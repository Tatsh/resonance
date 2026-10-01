"""Rebuild a FreQuency disc image with replacement binaries.

The entry point reads an original FreQuency image in cue, bin, or ISO form, exchanges the
`SCUS_971.25` executable and the `EZMIDI.IRX` module for fresh builds, and writes a raw
MODE2/2352 bin and its cue sheet like the original CD. An ISO input is the original's data
sectors without their Mode 2 framing. Burned as it is, an ISO makes a Mode 1 disc, and the
console's CD driver cannot read files from one.

Every output sector is encoded afresh as a Mode 2 Form 1 data sector from its 2048 data bytes,
and a two-second postgap follows the volume. A payload fitting its original extent overwrites
the extent. A larger payload moves to sectors appended after the volume. The directory record
and the volume size move with the payload. Every other data byte retains its sector.
"""

from __future__ import annotations

from pathlib import Path
from typing import TYPE_CHECKING, NamedTuple, Self
import argparse
import contextlib
import io
import json
import logging
import operator
import os
import re
import struct
import sys
import zipfile

import requests

if TYPE_CHECKING:
    from collections.abc import Generator, Sequence
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
_USER_AGENT = 'resonance-build-image'
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

_SYNC = b'\x00' + b'\xff' * 10 + b'\x00'
_MODE2 = 2
_LEAD_IN_SECTORS = 150
_SECTORS_PER_SECOND = 75
_SECONDS_PER_MINUTE = 60
_POSTGAP_SECTORS = 150
_ECC_P_COLUMNS = 86
_ECC_P_ROWS = 24
_ECC_Q_DIAGONALS = 52
_ECC_Q_ROWS = 43
_ECC_Q_STRIDE = 88
_ECC_POLYNOMIAL = 0x11D
_EDC_POLYNOMIAL = 0xD8018001
_DATA_SUBHEADER = bytes((0, 0, 8, 0)) * 2
_CUE_TEMPLATE = 'FILE "{name}" BINARY\n  TRACK 01 MODE2/2352\n    INDEX 01 00:00:00\n'

type _Replacements = dict[str, bytes]


def _build_ecc_tables() -> tuple[bytes, bytes]:
    forward = bytearray(256)
    backward = bytearray(256)
    for value in range(256):
        doubled = ((value << 1) ^ (_ECC_POLYNOMIAL if value & 0x80 else 0)) & 0xFF
        forward[value] = doubled
        backward[value ^ doubled] = value
    return bytes(forward), bytes(backward)


def _build_edc_table() -> tuple[int, ...]:
    table = []
    for value in range(256):
        edc = value
        for _ in range(8):
            edc = (edc >> 1) ^ (_EDC_POLYNOMIAL if edc & 1 else 0)
        table.append(edc)
    return tuple(table)


_ECC_FORWARD, _ECC_BACKWARD = _build_ecc_tables()
_EDC_TABLE = _build_edc_table()
# Each row gathers one byte per parity column. A row of XORs then computes every column at once.
_ECC_P_ROW_GETTERS = tuple(
    operator.itemgetter(*range(row * _ECC_P_COLUMNS, (row + 1) * _ECC_P_COLUMNS))
    for row in range(_ECC_P_ROWS))
_ECC_Q_ROW_GETTERS = tuple(
    operator.itemgetter(*(((diagonal >> 1) * _ECC_P_COLUMNS + (diagonal & 1) + row * _ECC_Q_STRIDE)
                          % (_ECC_Q_DIAGONALS * _ECC_Q_ROWS)
                          for diagonal in range(_ECC_Q_DIAGONALS)))
    for row in range(_ECC_Q_ROWS))


def _xor(left: bytes, right: bytes) -> bytes:
    return (int.from_bytes(left) ^ int.from_bytes(right)).to_bytes(len(left))


def _ecc_parity(data: bytes, getters: tuple[operator.itemgetter[int], ...]) -> bytes:
    width = len(getters[0](data))
    first = second = bytes(width)
    for getter in getters:
        row = bytes(getter(data))
        first = _xor(first, row).translate(_ECC_FORWARD)
        second = _xor(second, row)
    first = _xor(first.translate(_ECC_FORWARD), second).translate(_ECC_BACKWARD)
    return first + _xor(first, second)


def _bcd(value: int) -> int:
    return ((value // 10) << 4) | (value % 10)


def _encode_mode2_form1(lba: int, subheader: bytes, data: bytes) -> bytes:
    """
    Encode one CD-ROM XA Mode 2 Form 1 sector.

    Parameters
    ----------
    lba : int
        Sector number relative to the track start.
    subheader : bytes
        The eight subheader bytes.
    data : bytes
        The 2048 data bytes.

    Returns
    -------
    bytes
        The 2352-byte raw sector with its EDC and both ECC parity blocks.
    """
    address = lba + _LEAD_IN_SECTORS
    header = bytes((_bcd(address // (_SECTORS_PER_SECOND * _SECONDS_PER_MINUTE)),
                    _bcd(address // _SECTORS_PER_SECOND % _SECONDS_PER_MINUTE),
                    _bcd(address % _SECTORS_PER_SECOND), _MODE2))
    edc = 0
    for value in subheader + data:
        edc = (edc >> 8) ^ _EDC_TABLE[(edc ^ value) & 0xFF]
    # Form 1 parity treats the header as zero.
    protected = bytes(4) + subheader + data + edc.to_bytes(4, 'little')
    parity_p = _ecc_parity(protected, _ECC_P_ROW_GETTERS)
    parity_q = _ecc_parity(protected + parity_p, _ECC_Q_ROW_GETTERS)
    return _SYNC + header + protected[4:] + parity_p + parity_q


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


class _Plan(NamedTuple):
    """Sectors that differ from the original image."""

    overlays: dict[int, bytes]
    """Replacement data bytes by sector."""
    volume_sectors: int
    """Sectors in the output volume."""
    written: dict[str, int]
    """Output sector by target file name."""


def _plan_overlays(source: _SourceImage, replacements: _Replacements) -> _Plan:
    """
    Place each payload and collect the sectors that change.

    A payload fitting its original extent overwrites the extent with zero padding, and the
    metadata stays untouched. A larger payload moves to sectors appended after the volume,
    and the directory record and volume size move with the payload.

    Parameters
    ----------
    source : _SourceImage
        Open original image.
    replacements : _Replacements
        Payloads by target file name.

    Returns
    -------
    _Plan
        Changed sectors, the output volume size, and where each payload starts.
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
    return _Plan(overlays=overlays, volume_sectors=append_lba, written=written)


def _write_image(source: _SourceImage, replacements: _Replacements, output: Path) -> int:
    """
    Encode the raw MODE2/2352 image and write its cue sheet.

    Parameters
    ----------
    source : _SourceImage
        Open original image.
    replacements : _Replacements
        Payloads by target file name.
    output : Path
        Destination cue sheet path. The image is written beside it with a `.bin` suffix.

    Returns
    -------
    int
        Sectors in the output image, including the postgap.
    """
    plan = _plan_overlays(source, replacements)
    image = output.with_suffix('.bin')
    with image.open('wb') as handle:
        for lba in range(plan.volume_sectors):
            if (data := plan.overlays.get(lba)) is None:
                data = source.read_sector(lba)
            handle.write(_encode_mode2_form1(lba, _DATA_SUBHEADER, data))
        for lba in range(plan.volume_sectors, plan.volume_sectors + _POSTGAP_SECTORS):
            handle.write(_encode_mode2_form1(lba, _DATA_SUBHEADER, bytes(_SECTOR_DATA)))
    output.write_text(_CUE_TEMPLATE.format(name=image.name), encoding='utf-8')
    _verify_output(image, plan.written, replacements)
    return plan.volume_sectors + _POSTGAP_SECTORS


def _verify_output(image: Path, written: dict[str, int], replacements: _Replacements) -> None:
    """
    Confirm the replaced extents open with the payload prefix.

    Parameters
    ----------
    image : Path
        Written raw image path.
    written : dict[str, int]
        Output sector by target file name.
    replacements : _Replacements
        Payloads by target file name.

    Raises
    ------
    DiscImageError
        A prefix mismatches.
    """
    with image.open('rb') as handle:
        for target, lba in written.items():
            handle.seek(lba * _SECTOR_RAW + _MODE2_OFFSET)
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
    output_cue: Path
    """Destination cue sheet path. The bin is written beside it."""
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
    Build the image from resolved selections.

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
        The output is not a cue sheet, or the image cannot be read or rewritten.
    """
    if options.output_cue.suffix.lower() != '.cue':
        message = f'Output must be a cue sheet: {options.output_cue}.'
        raise DiscImageError(message)
    image = options.output_cue.with_suffix('.bin')
    if not options.overwrite and (options.output_cue.exists() or image.exists()):
        message = f'Output exists: {options.output_cue}.'
        raise DiscImageError(message)
    if image.resolve() == options.input_image.resolve() or (options.output_cue.resolve()
                                                            == options.input_image.resolve()):
        message = 'Output must differ from the input.'
        raise DiscImageError(message)
    replacements = _resolve_payloads(options)
    with _SourceImage(options.input_image) as source:
        return _write_image(source, replacements, options.output_cue)


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


def _existing_file(value: str) -> Path:
    """
    Convert an argument to the path of an existing file.

    Parameters
    ----------
    value : str
        Argument text.

    Returns
    -------
    Path
        The file path.

    Raises
    ------
    argparse.ArgumentTypeError
        No file exists at the path.
    """
    path = Path(value)
    if not path.is_file():
        message = f'No such file: {value}.'
        raise argparse.ArgumentTypeError(message)
    return path


def _build_parser() -> argparse.ArgumentParser:
    """
    Describe the command line.

    Returns
    -------
    argparse.ArgumentParser
        The parser.
    """
    parser = argparse.ArgumentParser(
        description='Rebuild a FreQuency CD image with replacement binaries. The raw MODE2/2352 '
        'bin is written beside the cue sheet, with the same name and a .bin suffix.')
    parser.add_argument('input_image', help='Original image in cue, bin, or ISO form.',
                        type=_existing_file)
    parser.add_argument('output_cue', help='Destination cue sheet path.', type=Path)
    parser.add_argument('--debug', action='store_true', help='Enable debug logging.')
    parser.add_argument('--ezmidi-irx', help='Local module file, bypassing the artifact download.',
                        type=_existing_file)
    parser.add_argument('--overwrite', action='store_true',
                        help='Replace the output files when present.')
    parser.add_argument('--repo', default=_DEFAULT_REPO,
                        help='Repository holding the build artifacts (default: %(default)s).')
    parser.add_argument('--resonance-bin',
                        help='Local executable file, bypassing the artifact download.',
                        type=_existing_file)
    parser.add_argument('--run-id', help='Actions run to fetch. The latest successful run is used '
                        'when omitted.', type=int)
    parser.add_argument('--token', help='GitHub token. GITHUB_TOKEN or GH_TOKEN provides the value '
                        'when the flag is absent.')
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    """
    Rebuild a FreQuency CD image with replacement binaries.

    Parameters
    ----------
    argv : Sequence[str] | None
        Arguments after the program name. The process arguments are used when omitted.

    Returns
    -------
    int
        The exit status, 0 on success and 1 on failure.
    """
    args = _build_parser().parse_args(argv)
    logging.basicConfig(format='%(levelname)s: %(message)s',
                        level=logging.DEBUG if args.debug else logging.INFO)
    options = _Options(debug=args.debug, ezmidi_irx=args.ezmidi_irx, input_image=args.input_image,
                       output_cue=args.output_cue, overwrite=args.overwrite, repo=args.repo,
                       resonance_bin=args.resonance_bin, run_id=args.run_id, token=args.token)
    outputs = (options.output_cue, options.output_cue.with_suffix('.bin'))
    existed = {path for path in outputs if path.exists()}
    try:
        sectors = _run(options)
    except (ArtifactError, DiscImageError, OSError):
        for path in outputs:
            if path not in existed:
                with contextlib.suppress(OSError):
                    path.unlink(missing_ok=True)
        log.exception('Image build failed.')
        return 1
    print(f'Wrote {options.output_cue} ({sectors} sectors).')  # ruff: ignore[print]
    return 0


if __name__ == '__main__':
    sys.exit(main())
