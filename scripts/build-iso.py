"""Rebuild a FreQuency disc image with replacement binaries.

The entry point reads an original FreQuency image in cue, bin, or ISO form, exchanges the
`SCUS_971.25` executable and the `EZMIDI.IRX` module for fresh builds, and writes a plain
ISO. Every other byte retains its sector. A payload fitting its original extent overwrites
the extent. A larger payload moves to sectors appended after the volume. The directory
record and the volume size move with the payload. Nothing else shifts. The layout still
boots the console.
"""

from __future__ import annotations

from pathlib import Path
from typing import TYPE_CHECKING, NamedTuple, Self
import contextlib
import io
import json
import logging
import os
import re
import struct
import zipfile

import click
import requests

if TYPE_CHECKING:
    from collections.abc import Generator
    from types import TracebackType

log = logging.getLogger(__name__)

__all__ = ('ArtifactError', 'DiscImageError', 'main')


class ArtifactError(Exception):
    """Build artifacts cannot be fetched from GitHub."""


class DiscImageError(Exception):
    """The disc image cannot be read or rewritten."""


_DEFAULT_REPO = 'Tatsh/resonance'
_WORKFLOW_FILE = 'build.yml'
_GITHUB_API = 'https://api.github.com'
_USER_AGENT = 'resonance-build-iso'
_HTTP_TIMEOUT = 30
_SECTOR_DATA = 2048
_SECTOR_RAW = 2352
_MODE1_OFFSET = 16
_MODE2_OFFSET = 24
_PVD_SECTOR = 16
_PVD_MAGIC = b'CD001'
_RESONANCE_ARTIFACT = 'resonance'
_EZMIDI_ARTIFACT = 'ezmidi-irx'
_RESONANCE_MEMBER = 'resonance'
_EZMIDI_MEMBER = 'EZMIDI.IRX'
_TARGET_EXECUTABLE = 'SCUS_971.25'
_TARGET_MODULE = 'EZMIDI.IRX'
_ELF_MAGIC = b'\x7fELF'
_CUE_FILE_RE = re.compile(r'FILE\s+"(?P<name>[^"]+)"', re.IGNORECASE)
_CUE_TRACK_RE = re.compile(r'TRACK\s+(?P<number>\d+)\s+(?P<mode>\S+)', re.IGNORECASE)
_CUE_INDEX_RE = re.compile(r'INDEX\s+01\s+(?P<minute>\d+):(?P<second>\d+):(?P<frame>\d+)',
                           re.IGNORECASE)

type _Replacements = dict[str, bytes]


class _Geometry(NamedTuple):
    """Sector layout of a data track."""

    data_offset: int
    """Offset of the 2048 data bytes within a raw sector."""
    sector_size: int
    """Bytes per raw sector on the medium."""
    track_start: int
    """Absolute sector where the data track starts."""


class _Record(NamedTuple):
    """One ISO9660 directory record."""

    is_dir: bool
    """Whether the record describes a directory."""
    lba: int
    """First data sector, relative to the track start."""
    multi: bool
    """Whether the file continues in a further extent."""
    name: str
    """File identifier without the version suffix."""
    size: int
    """Data length in bytes."""


class _LocatedFile(NamedTuple):
    """A file found in the ISO9660 tree."""

    lba: int
    """First data sector, relative to the track start."""
    parent_lba: int
    """First sector of the containing directory."""
    parent_size: int
    """Length of the containing directory in bytes."""
    size: int
    """Data length in bytes."""


def _parse_cue(path: Path) -> tuple[Path, _Geometry]:
    """
    Locate the data track described by a cue sheet.

    Parameters
    ----------
    path : Path
        Cue sheet beside the binary track dump.

    Returns
    -------
    tuple[Path, _Geometry]
        Binary path and sector layout of track one.

    Raises
    ------
    DiscImageError
        The sheet lacks a binary track one with a data mode.
    """
    text = path.read_text(encoding='utf-8', errors='replace')
    bin_name: str | None = None
    track_mode: str | None = None
    track_start = 0
    for line in text.splitlines():
        if (found := _CUE_FILE_RE.search(line)) is not None:
            bin_name = found.group('name')
        elif (found := _CUE_TRACK_RE.search(line)) is not None:
            if int(found.group('number')) == 1:
                track_mode = found.group('mode')
        elif (found := _CUE_INDEX_RE.search(line)) is not None and track_mode is not None:
            minute, second, frame = (int(found.group(part)) for part in ('minute', 'second',
                                                                         'frame'))
            track_start = minute * 4500 + second * 75 + frame
            break
    if bin_name is None or track_mode is None:
        message = f'Cue sheet lacks a binary track one: {path}.'
        raise DiscImageError(message)
    candidate = _resolve_bin_path(path.parent, bin_name)
    match track_mode.upper():
        case 'MODE1/2352':
            offset = _MODE1_OFFSET
        case 'MODE2/2352':
            offset = _MODE2_OFFSET
        case _:
            message = f'Unsupported track mode: {track_mode}.'
            raise DiscImageError(message)
    geometry = _Geometry(data_offset=offset, sector_size=_SECTOR_RAW, track_start=track_start)
    return candidate, geometry


def _resolve_bin_path(directory: Path, bin_name: str) -> Path:
    """
    Locate the binary dump beside a cue sheet, ignoring case.

    Parameters
    ----------
    directory : Path
        Directory holding the cue sheet.
    bin_name : str
        File name from the sheet.

    Returns
    -------
    Path
        Existing binary path.

    Raises
    ------
    DiscImageError
        No file matches the name.
    """
    candidate = directory / bin_name
    if candidate.exists():
        return candidate
    lowered = bin_name.casefold()
    for entry in directory.iterdir():
        if entry.is_file() and entry.name.casefold() == lowered:
            return entry
    message = f'Binary {bin_name} missing beside {directory}.'
    raise DiscImageError(message)


def _detect_geometry(path: Path) -> tuple[Path, _Geometry]:
    """
    Resolve the sector layout for a cue, raw, or ISO image.

    Parameters
    ----------
    path : Path
        Disc image path.

    Returns
    -------
    tuple[Path, _Geometry]
        Readable binary path and its sector layout.

    Raises
    ------
    DiscImageError
        The suffix is unknown or the cue sheet cannot be parsed.
    """
    match path.suffix.lower():
        case '.cue':
            return _parse_cue(path)
        case '.bin':
            geometry = _Geometry(data_offset=_MODE2_OFFSET, sector_size=_SECTOR_RAW,
                                 track_start=0)
            return path, geometry
        case '.iso':
            geometry = _Geometry(data_offset=0, sector_size=_SECTOR_DATA, track_start=0)
            return path, geometry
        case _:
            message = f'Unsupported disc image suffix: {path.suffix}.'
            raise DiscImageError(message)


class _SourceImage:
    """Random access to the data track of a disc image."""

    def __enter__(self) -> Self:
        """
        Return the open image.

        Returns
        -------
        Self
            The open image.
        """
        return self

    def __exit__(self, exc_type: type[BaseException] | None, exc_value: BaseException | None,
                 traceback: TracebackType | None) -> None:
        """Close the image."""
        self.close()

    def __init__(self, path: Path) -> None:
        """
        Open an image and verify its primary volume descriptor.

        Parameters
        ----------
        path : Path
            Cue, raw, or ISO image path.

        Raises
        ------
        DiscImageError
            The layout is unknown or sector sixteen lacks the descriptor.
        """
        bin_path, geometry = _detect_geometry(path)
        self._data_offset = geometry.data_offset
        self._handle = bin_path.open('rb')
        self._sector_size = geometry.sector_size
        self._track_start = geometry.track_start
        descriptor = self.read_sector(_PVD_SECTOR)
        if descriptor[0] != 1 or descriptor[1:6] != _PVD_MAGIC:
            message = f'No primary volume descriptor in {path}.'
            raise DiscImageError(message)
        self._volume_sectors = struct.unpack('<I', descriptor[80:84])[0]

    def close(self) -> None:
        """Close the image."""
        self._handle.close()

    def find(self, name: str) -> _LocatedFile:
        """
        Locate a file by name anywhere in the tree.

        Parameters
        ----------
        name : str
            File name without the version suffix.

        Returns
        -------
        _LocatedFile
            Extent and containing directory of the match.

        Raises
        ------
        DiscImageError
            The file is missing or spans several extents.
        """
        descriptor = self.read_sector(_PVD_SECTOR)
        root_lba = struct.unpack('<I', descriptor[158:162])[0]
        root_size = struct.unpack('<I', descriptor[166:170])[0]
        stack = [(root_lba, root_size)]
        while stack:
            parent_lba, parent_size = stack.pop()
            for record in self._iter_records(parent_lba, parent_size):
                if record.name in {'\x00', '\x01'}:
                    continue
                if record.is_dir:
                    stack.append((record.lba, record.size))
                elif record.name.split(';')[0].upper() == name:
                    if record.multi:
                        message = f'File spans several extents: {name}.'
                        raise DiscImageError(message)
                    return _LocatedFile(lba=record.lba, parent_lba=parent_lba,
                                        parent_size=parent_size, size=record.size)
        message = f'File missing from the image: {name}.'
        raise DiscImageError(message)

    def read_sector(self, lba: int) -> bytes:
        """
        Read the data bytes of one sector.

        Parameters
        ----------
        lba : int
            Sector number relative to the track start.

        Returns
        -------
        bytes
            The 2048 data bytes.

        Raises
        ------
        DiscImageError
            The medium ends before the sector.
        """
        self._handle.seek((self._track_start + lba) * self._sector_size + self._data_offset)
        data = self._handle.read(_SECTOR_DATA)
        if len(data) != _SECTOR_DATA:
            message = f'Sector {lba} lies past the end of the medium.'
            raise DiscImageError(message)
        return data

    def volume_sectors(self) -> int:
        """
        Report the sector count of the volume.

        Returns
        -------
        int
            Sectors described by the primary volume descriptor.
        """
        return self._volume_sectors

    def _iter_records(self, lba: int, size: int) -> Generator[_Record]:
        """
        Yield the directory records of one extent.

        Parameters
        ----------
        lba : int
            First sector of the directory.
        size : int
            Length of the directory in bytes.

        Yields
        ------
        _Record
            Each record in order.
        """
        extent = b''.join(
            self.read_sector(lba + index)
            for index in range((size + _SECTOR_DATA - 1) // _SECTOR_DATA))
        offset = 0
        while offset < len(extent):
            if (length := extent[offset]) == 0:
                offset = (offset // _SECTOR_DATA + 1) * _SECTOR_DATA
                continue
            record = extent[offset:offset + length]
            flags = record[25]
            yield _Record(is_dir=bool(flags & 0x02), lba=struct.unpack('<I', record[2:6])[0],
                          multi=bool(flags & 0x80),
                          name=record[33:33 + record[32]].decode('ascii', errors='replace'),
                          size=struct.unpack('<I', record[10:14])[0])
            offset += length


def _request_headers(token: str | None) -> dict[str, str]:
    """
    Authorisation headers for the GitHub API.

    Parameters
    ----------
    token : str | None
        Bearer token, omitted when absent.

    Returns
    -------
    dict[str, str]
        Headers for a request.
    """
    headers = {'Accept': 'application/vnd.github+json', 'User-Agent': _USER_AGENT}
    if token is not None:
        headers['Authorization'] = f'Bearer {token}'
    return headers


def _read_url(url: str, token: str | None) -> bytes:
    """
    Fetch a URL.

    The library drops authorisation when a redirect leaves the API host. The presigned
    storage download requires anonymous access.

    Parameters
    ----------
    url : str
        Address to fetch.
    token : str | None
        Bearer token for the first request.

    Returns
    -------
    bytes
        Response body.

    Raises
    ------
    ArtifactError
        The request fails or the response reports an error.
    """
    try:
        response = requests.get(url, headers=_request_headers(token), timeout=_HTTP_TIMEOUT)
    except requests.RequestException as e:
        message = f'GitHub request failed for {url}: {e}.'
        raise ArtifactError(message) from e
    if not response.ok:
        message = f'GitHub responded with status {response.status_code} for {url}.'
        raise ArtifactError(message)
    return response.content


def _api_json(url: str, token: str | None) -> object:
    """
    Decode a JSON document from the GitHub API.

    Parameters
    ----------
    url : str
        Address to fetch.
    token : str | None
        Bearer token for the request.

    Returns
    -------
    object
        Decoded document with unknown shape.

    Raises
    ------
    ArtifactError
        The request fails or the body is not JSON.
    """
    try:
        return json.loads(_read_url(url, token))
    except ValueError as e:
        message = f'GitHub returned invalid JSON for {url}.'
        raise ArtifactError(message) from e


def _latest_successful_run(repo: str, token: str | None) -> int:
    """
    Identify the latest successful build run.

    Parameters
    ----------
    repo : str
        Repository in owner and name form.
    token : str | None
        Bearer token for the request.

    Returns
    -------
    int
        Run identifier.

    Raises
    ------
    ArtifactError
        No successful run exists.
    """
    url = (f'{_GITHUB_API}/repos/{repo}/actions/workflows/{_WORKFLOW_FILE}/runs'
           '?status=completed&per_page=20')
    data = _api_json(url, token)
    runs = data['workflow_runs'] if isinstance(data, dict) else []
    for run in runs:
        if isinstance(run, dict) and run.get('conclusion') == 'success':
            return int(run['id'])
    message = f'No successful runs exist for {repo}.'
    raise ArtifactError(message)


def _download_artifact(repo: str, run_id: int, artifact: str, token: str) -> bytes:
    """
    Fetch one artifact archive from a run.

    Parameters
    ----------
    repo : str
        Repository in owner and name form.
    run_id : int
        Actions run holding the artifact.
    artifact : str
        Artifact name.
    token : str
        Bearer token for the download.

    Returns
    -------
    bytes
        Archive bytes.

    Raises
    ------
    ArtifactError
        The artifact is missing or expired.
    """
    url = f'{_GITHUB_API}/repos/{repo}/actions/runs/{run_id}/artifacts'
    data = _api_json(url, token)
    items = data['artifacts'] if isinstance(data, dict) else []
    for item in items:
        if isinstance(item, dict) and item.get('name') == artifact:
            if item.get('expired', False):
                message = f'Artifact {artifact} expired.'
                raise ArtifactError(message)
            return _read_url(str(item['archive_download_url']), token)
    message = f'Artifact {artifact} missing from run {run_id}.'
    raise ArtifactError(message)


def _extract_member(data: bytes, member: str) -> bytes:
    """
    Pull one file from an artifact archive.

    Parameters
    ----------
    data : bytes
        Archive bytes.
    member : str
        File name to find by base name.

    Returns
    -------
    bytes
        Member contents.

    Raises
    ------
    ArtifactError
        The archive is invalid or lacks the member.
    """
    try:
        with zipfile.ZipFile(io.BytesIO(data)) as archive:
            names = archive.namelist()
            for info in archive.infolist():
                if Path(info.filename).name == member:
                    return archive.read(info.filename)
            if len(names) == 1 and not names[0].endswith('/'):
                return archive.read(names[0])
    except zipfile.BadZipFile as e:
        message = 'Artifact payload is not a zip archive.'
        raise ArtifactError(message) from e
    message = f'Member {member} missing from the archive.'
    raise ArtifactError(message)


def _patch_record(sector: bytes,
                  old_lba: int,
                  name: str,
                  new_lba: int,
                  new_size: int) -> bytes:
    """
    Rewrite the extent fields of one directory record.

    Parameters
    ----------
    sector : bytes
        Directory sector holding the record.
    old_lba : int
        Extent to match.
    name : str
        File name without the version suffix.
    new_lba : int
        Replacement extent.
    new_size : int
        Replacement length in bytes.

    Returns
    -------
    bytes
        Sector with the patched record.

    Raises
    ------
    DiscImageError
        The record is absent from the sector.
    """
    patched = bytearray(sector)
    offset = 0
    while offset < len(patched):
        length = patched[offset]
        if length == 0:
            break
        entry_lba = struct.unpack('<I', patched[offset + 2:offset + 6])[0]
        entry_name = bytes(patched[offset + 33:offset + 33 + patched[offset + 32]])
        decoded = entry_name.decode('ascii', errors='replace').split(';')[0].upper()
        if entry_lba == old_lba and decoded == name:
            struct.pack_into('<I', patched, offset + 2, new_lba)
            struct.pack_into('>I', patched, offset + 6, new_lba)
            struct.pack_into('<I', patched, offset + 10, new_size)
            struct.pack_into('>I', patched, offset + 14, new_size)
            return bytes(patched)
        offset += length
    message = f'Record vanished from its directory: {name}.'
    raise DiscImageError(message)


def _write_iso(source: _SourceImage, replacements: _Replacements, output: Path) -> int:
    """
    Stream the output ISO with replaced extents.

    A payload fitting its original extent overwrites the extent with zero padding, and the
    metadata stays untouched. A larger payload moves to sectors appended after the volume,
    and the directory record and volume size move with the payload. Every other sector is
    copied verbatim.

    Parameters
    ----------
    source : _SourceImage
        Open original image.
    replacements : _Replacements
        Payloads by target file name.
    output : Path
        Destination ISO path.

    Returns
    -------
    int
        Sectors in the output image.
    """
    volume_sectors = source.volume_sectors()
    overlays: dict[int, bytes] = {}
    written: dict[str, int] = {}
    append_lba = volume_sectors
    for target, payload in replacements.items():
        if not payload.startswith(_ELF_MAGIC):
            log.warning('Payload for `%s` lacks an ELF header.', target)
        found = source.find(target)
        if len(payload) <= found.size:
            padded = payload + b'\x00' * (found.size - len(payload))
            for index in range((found.size + _SECTOR_DATA - 1) // _SECTOR_DATA):
                start, end = index * _SECTOR_DATA, min((index + 1) * _SECTOR_DATA, found.size)
                chunk = padded[start:end]
                if len(chunk) < _SECTOR_DATA:
                    chunk += source.read_sector(found.lba + index)[len(chunk):]
                overlays[found.lba + index] = chunk
            written[target] = found.lba
            log.info('Replaced %s in place (%d bytes).', target, len(payload))
            continue
        sectors = (len(payload) + _SECTOR_DATA - 1) // _SECTOR_DATA
        blob = payload + b'\x00' * (sectors * _SECTOR_DATA - len(payload))
        for index in range(sectors):
            overlays[append_lba + index] = blob[index * _SECTOR_DATA:(index + 1) * _SECTOR_DATA]
        parent_sectors = (found.parent_size + _SECTOR_DATA - 1) // _SECTOR_DATA
        for index in range(parent_sectors):
            parent_lba = found.parent_lba + index
            parent = overlays.get(parent_lba)
            if parent is None:
                parent = source.read_sector(parent_lba)
            overlays[parent_lba] = _patch_record(parent, found.lba, target, append_lba,
                                                 len(payload))
        written[target] = append_lba
        log.info('Relocated %s to sector %d (%d bytes).', target, append_lba, len(payload))
        append_lba += sectors
    if append_lba != volume_sectors:
        descriptor = bytearray(source.read_sector(_PVD_SECTOR))
        struct.pack_into('<I', descriptor, 80, append_lba)
        struct.pack_into('>I', descriptor, 84, append_lba)
        overlays[_PVD_SECTOR] = bytes(descriptor)
    with output.open('wb') as handle:
        for lba in range(append_lba):
            chunk = overlays.get(lba)
            handle.write(chunk if chunk is not None else source.read_sector(lba))
    _verify_output(output, written, replacements)
    return append_lba


def _verify_output(output: Path, written: dict[str, int], replacements: _Replacements) -> None:
    """
    Confirm the replaced extents open with the payload prefix.

    Parameters
    ----------
    output : Path
        Written ISO path.
    written : dict[str, int]
        Output sector by target file name.
    replacements : _Replacements
        Payloads by target file name.

    Raises
    ------
    DiscImageError
        A prefix mismatches.
    """
    with output.open('rb') as handle:
        for target, lba in written.items():
            handle.seek(lba * _SECTOR_DATA)
            if handle.read(4) != replacements[target][:4]:
                message = f'Verification failed for {target}.'
                raise DiscImageError(message)


class _Options(NamedTuple):
    """Command line selections."""

    debug: bool
    """Enable debug logging."""
    ezmidi_irx: Path | None
    """Local module file, bypassing the artifact download."""
    input_image: Path
    """Original image in cue, bin, or ISO form."""
    output_iso: Path
    """Destination ISO path."""
    overwrite: bool
    """Replace the output file when present."""
    repo: str
    """Repository holding the build artifacts."""
    resonance_bin: Path | None
    """Local executable file, bypassing the artifact download."""
    run_id: int | None
    """Actions run to fetch. The latest successful run is used when omitted."""
    token: str | None
    """GitHub token. GITHUB_TOKEN or GH_TOKEN provides the value when the flag is absent."""


def _run(options: _Options) -> int:
    """
    Build the ISO from resolved selections.

    Parameters
    ----------
    options : _Options
        Command line selections.

    Returns
    -------
    int
        Sectors in the output image.

    Raises
    ------
    DiscImageError
        The image cannot be read or rewritten.
    """
    if options.output_iso.exists() and not options.overwrite:
        message = f'Output exists: {options.output_iso}.'
        raise DiscImageError(message)
    if options.output_iso.resolve() == options.input_image.resolve():
        message = 'Output must differ from the input.'
        raise DiscImageError(message)
    replacements = _resolve_payloads(options)
    with _SourceImage(options.input_image) as source:
        return _write_iso(source, replacements, options.output_iso)


def _resolve_payloads(options: _Options) -> _Replacements:
    """
    Fetch or read both replacement payloads.

    Parameters
    ----------
    options : _Options
        Command line selections.

    Returns
    -------
    _Replacements
        Payloads by target file name.

    Raises
    ------
    ArtifactError
        Downloads fail or artifacts are unusable.
    """
    effective = options.token or os.environ.get('GITHUB_TOKEN') or os.environ.get('GH_TOKEN')
    if options.resonance_bin is None or options.ezmidi_irx is None:
        if effective is None:
            message = 'A token is required for downloads. Set GITHUB_TOKEN.'
            raise ArtifactError(message)
        resolved = (options.run_id if options.run_id is not None
                    else _latest_successful_run(options.repo, effective))
    else:
        resolved = 0
    if options.resonance_bin is not None:
        executable = options.resonance_bin.read_bytes()
    else:
        archive = _download_artifact(options.repo, resolved, _RESONANCE_ARTIFACT,
                                     str(effective))
        executable = _extract_member(archive, _RESONANCE_MEMBER)
    if options.ezmidi_irx is not None:
        module = options.ezmidi_irx.read_bytes()
    else:
        archive = _download_artifact(options.repo, resolved, _EZMIDI_ARTIFACT, str(effective))
        module = _extract_member(archive, _EZMIDI_MEMBER)
    return {_TARGET_EXECUTABLE: executable, _TARGET_MODULE: module}


@click.command()
@click.argument('input_image', type=click.Path(dir_okay=False, exists=True, path_type=Path))
@click.argument('output_iso', type=click.Path(dir_okay=False, path_type=Path))
@click.option('--debug', default=False, help='Enable debug logging.', is_flag=True,
              show_default=True)
@click.option('--ezmidi-irx', default=None,
              help='Local module file, bypassing the artifact download.',
              type=click.Path(dir_okay=False, exists=True, path_type=Path))
@click.option('--overwrite', default=False, help='Replace the output file when present.',
              is_flag=True, show_default=True)
@click.option('--repo', default=_DEFAULT_REPO, help='Repository holding the build artifacts.',
              show_default=True)
@click.option('--resonance-bin', default=None,
              help='Local executable file, bypassing the artifact download.',
              type=click.Path(dir_okay=False, exists=True, path_type=Path))
@click.option('--run-id', default=None, help='Actions run to fetch. The latest successful run '
              'is used when omitted.', type=int)
@click.option('--token', default=None, help='GitHub token. GITHUB_TOKEN or GH_TOKEN provides '
              'the value when the flag is absent.')
def main(input_image: Path,  # ruff: ignore[too-many-arguments]
         output_iso: Path,
         *,
         debug: bool,
         ezmidi_irx: Path | None,
         overwrite: bool,
         repo: str,
         resonance_bin: Path | None,
         run_id: int | None,
         token: str | None) -> None:
    """
    Rebuild a FreQuency ISO with replacement binaries.

    Parameters
    ----------
    input_image : Path
        Original image in cue, bin, or ISO form.
    output_iso : Path
        Destination ISO path.
    debug : bool
        Enable debug logging.
    ezmidi_irx : Path | None
        Local module file, bypassing the artifact download.
    overwrite : bool
        Replace the output file when present.
    repo : str
        Repository holding the build artifacts.
    resonance_bin : Path | None
        Local executable file, bypassing the artifact download.
    run_id : int | None
        Actions run to fetch. The latest successful run is used when omitted.
    token : str | None
        GitHub token. GITHUB_TOKEN or GH_TOKEN provides the value when the flag is absent.

    Raises
    ------
    click.Abort
        Any failure, chained from the cause.
    """
    logging.basicConfig(format='%(levelname)s: %(message)s',
                        level=logging.DEBUG if debug else logging.INFO)
    options = _Options(debug=debug, ezmidi_irx=ezmidi_irx, input_image=input_image,
                       output_iso=output_iso, overwrite=overwrite, repo=repo,
                       resonance_bin=resonance_bin, run_id=run_id, token=token)
    existed = output_iso.exists()
    try:
        sectors = _run(options)
    except (ArtifactError, DiscImageError, OSError) as e:
        if not existed:
            with contextlib.suppress(OSError):
                output_iso.unlink(missing_ok=True)
        log.exception('ISO build failed.')
        raise click.Abort from e
    click.echo(f'Wrote {options.output_iso} ({sectors} sectors).')


if __name__ == '__main__':
    main()
