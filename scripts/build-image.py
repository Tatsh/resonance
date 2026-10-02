"""Rebuild a FreQuency disc image with replacement binaries.

The entry point reads an original FreQuency image in cue, bin, or ISO form, exchanges the
executable that `SYSTEM.CNF` boots (`SCUS_971.25` in North America, `SCES_507.91` in Europe) and
the `EZMIDI.IRX` module for fresh builds, and writes a raw
MODE2/2352 bin and its cue sheet like the original CD. An ISO input is the original's data
sectors without their Mode 2 framing. Burned as it is, an ISO makes a Mode 1 disc, and the
console's CD driver cannot read files from one.

The input may instead be the disc root, the directory with `SYSTEM.CNF`. An ISO9660 volume is
then built from the files of the disc root, with the identifiers of the original disc. A directory
does not include the boot logo that the original disc stores in its first 12 sectors. The 12 logo
sectors come from a separate file or stay zero.

Every output sector is encoded afresh as a Mode 2 Form 1 data sector from its 2048 data bytes,
and a two-second postgap follows the volume. A payload fitting its original extent overwrites
the extent. A larger payload moves to sectors appended after the volume. The directory record
and the volume size move with the payload. Every other data byte retains its sector.
"""

from __future__ import annotations

from pathlib import Path
from typing import TYPE_CHECKING, BinaryIO, NamedTuple, Self
import argparse
import bisect
import contextlib
import datetime
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
_EZMIDI_ARTIFACT = 'release-assets'
_RESONANCE_MEMBER = 'resonance'
_EZMIDI_MEMBER = 'EZMIDI.IRX'
_KNOWN_EXECUTABLES = {'SCES_507.91': 'PAL', 'SCUS_971.25': 'NTSC-U/C'}
_PAL_EXECUTABLE = 'SCES_507.91'
_TARGET_MODULE = 'EZMIDI.IRX'
_BOOT2_RE = re.compile(r'^\s*BOOT2\s*=\s*cdrom0:\\(?P<name>[^;\s]+)', re.IGNORECASE | re.MULTILINE)
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
# The first 12 sectors of a PlayStation 2 disc store the encrypted boot logo the console checks.
_LOGO_SECTORS = 12
_PATH_TABLE_LBA = 18
_SYSTEM_CNF = 'SYSTEM.CNF'
_SYSTEM_IDENTIFIER = b'PLAYSTATION'
_ISO_NAME_MAX = 30
_ISO_NAME_RE = re.compile(r'[A-Z0-9_]+(?:\.[A-Z0-9_]*)?')
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


def _both_endian(value: int, width: int) -> bytes:
    return value.to_bytes(width, 'little') + value.to_bytes(width, 'big')


def _sectors_for(size: int) -> int:
    return (size + _SECTOR_DATA - 1) // _SECTOR_DATA


def _record_date(stamp: float) -> bytes:
    moment = datetime.datetime.fromtimestamp(stamp, tz=datetime.UTC)
    return bytes((moment.year - 1900, moment.month, moment.day, moment.hour, moment.minute,
                  moment.second, 0))


def _volume_date(stamp: float) -> bytes:
    moment = datetime.datetime.fromtimestamp(stamp, tz=datetime.UTC)
    return moment.strftime('%Y%m%d%H%M%S00').encode() + b'\x00'


def _iso_name(path: Path) -> str:
    """
    Convert a file or directory name to its ISO9660 identifier.

    Parameters
    ----------
    path : Path
        Entry in the disc root.

    Returns
    -------
    str
        Upper-case identifier, with the `;1` version suffix for a file.

    Raises
    ------
    DiscImageError
        The name has characters ISO9660 does not allow or is too long.
    """
    name = path.name.upper()
    if not _ISO_NAME_RE.fullmatch(name) or len(name) > _ISO_NAME_MAX:
        message = f'Name not valid on an ISO9660 disc: {path}.'
        raise DiscImageError(message)
    return name if path.is_dir() else f'{name};1'


class _DirectoryEntry(NamedTuple):
    """One file or directory placed in the volume."""

    iso_name: str
    """Identifier in its parent directory."""
    lba: int
    """First sector."""
    mtime: float
    """Modification time of the source."""
    path: Path
    """Source path."""
    size: int
    """Data length in bytes. A directory's length is its record extent."""


class _DirectoryVolume:
    """An ISO9660 volume built from the files of a disc root directory."""

    def __enter__(self) -> Self:
        """
        Return the open volume.

        Returns
        -------
        Self
            The open volume.
        """
        return self

    def __exit__(self, exc_type: type[BaseException] | None, exc_value: BaseException | None,
                 traceback: TracebackType | None) -> None:
        """Close the open source files."""
        self.close()

    def __init__(self, root: Path, system_area: bytes | None) -> None:
        """
        Build the volume layout.

        The primary volume descriptor, the terminator, and the path tables take the sectors the
        original disc gives them. The directories follow in path table order, and then the files
        in the same order. Metadata sectors are built in memory, and file sectors are read from
        the source files on demand.

        Parameters
        ----------
        root : Path
            Disc root directory with `SYSTEM.CNF`.
        system_area : bytes | None
            The 12 sectors of boot logo data. The sectors are zero when omitted.

        Raises
        ------
        DiscImageError
            The directory is not a disc root, a name is not valid, or the system area has the
            wrong size.
        """
        if not any(entry.name.upper() == _SYSTEM_CNF for entry in root.iterdir()):
            message = f'{root} lacks {_SYSTEM_CNF} and is not the root of a PlayStation 2 disc.'
            raise DiscImageError(message)
        if system_area is not None and len(system_area) != _LOGO_SECTORS * _SECTOR_DATA:
            message = f'The system area must be {_LOGO_SECTORS * _SECTOR_DATA} bytes.'
            raise DiscImageError(message)
        if system_area is None:
            log.warning('No system area given. The boot logo sectors are zero, and a console may '
                        'refuse the disc.')
        self._handles: dict[Path, BinaryIO] = {}
        self._system_area = system_area or bytes(_LOGO_SECTORS * _SECTOR_DATA)
        # Breadth-first order is path table order. Children sort by identifier.
        directories = [root]
        parents = [0]
        children: dict[Path, list[Path]] = {}
        index = 0
        while index < len(directories):
            listing = sorted(directories[index].iterdir(), key=_iso_name)
            children[directories[index]] = listing
            for entry in listing:
                if entry.is_dir():
                    directories.append(entry)
                    parents.append(index + 1)
            index += 1
        path_table = sum(8 + len(self._dir_identifier(directory, root)) + (
            len(self._dir_identifier(directory, root)) & 1) for directory in directories)
        table_sectors = _sectors_for(path_table)
        next_lba = _PATH_TABLE_LBA + 4 * table_sectors
        self._dirs: dict[Path, _DirectoryEntry] = {}
        for directory in directories:
            extent = self._extent_size(children[directory])
            self._dirs[directory] = _DirectoryEntry(
                iso_name=_iso_name(directory) if directory != root else '\x00', lba=next_lba,
                mtime=directory.stat().st_mtime, path=directory, size=extent)
            next_lba += _sectors_for(extent)
        self._files: list[_DirectoryEntry] = []
        self._parent_of: dict[str, Path] = {}
        for directory in directories:
            for entry in children[directory]:
                if entry.is_dir():
                    continue
                stat = entry.stat()
                placed = _DirectoryEntry(iso_name=_iso_name(entry), lba=next_lba,
                                         mtime=stat.st_mtime, path=entry, size=stat.st_size)
                self._files.append(placed)
                self._parent_of.setdefault(placed.iso_name.split(';')[0], directory)
                next_lba += _sectors_for(stat.st_size)
        self._file_starts = [entry.lba for entry in self._files]
        self._volume_sectors = next_lba
        self._metadata: dict[int, bytes] = {}
        newest = max(entry.mtime for entry in (*self._dirs.values(), *self._files))
        self._write_descriptors(root, path_table, table_sectors, newest)
        self._write_path_tables(root, directories, parents, table_sectors)
        for directory in directories:
            self._write_directory(root, directory, children[directory])

    def close(self) -> None:
        """Close the open source files."""
        for handle in self._handles.values():
            handle.close()
        self._handles.clear()

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
            Extent and parent directory of the match.

        Raises
        ------
        DiscImageError
            The file is missing.
        """
        for entry in self._files:
            if entry.iso_name.split(';')[0] == name:
                parent = self._dirs[self._parent_of[name]]
                return _LocatedFile(lba=entry.lba, parent_lba=parent.lba,
                                    parent_size=parent.size, size=entry.size)
        message = f'File missing from the directory: {name}.'
        raise DiscImageError(message)

    def read_sector(self, lba: int) -> bytes:
        """
        Produce the data bytes of one sector.

        Parameters
        ----------
        lba : int
            Sector number from the start of the volume.

        Returns
        -------
        bytes
            The 2048 data bytes.
        """
        if lba < _LOGO_SECTORS:
            return self._system_area[lba * _SECTOR_DATA:(lba + 1) * _SECTOR_DATA]
        if (data := self._metadata.get(lba)) is not None:
            return data
        position = bisect.bisect_right(self._file_starts, lba) - 1
        if position >= 0:
            entry = self._files[position]
            offset = (lba - entry.lba) * _SECTOR_DATA
            if offset < entry.size:
                if (handle := self._handles.get(entry.path)) is None:
                    handle = self._handles[entry.path] = entry.path.open('rb')
                handle.seek(offset)
                return handle.read(_SECTOR_DATA).ljust(_SECTOR_DATA, b'\x00')
        return bytes(_SECTOR_DATA)

    def volume_sectors(self) -> int:
        """
        Report the sector count of the volume.

        Returns
        -------
        int
            Sectors in the volume.
        """
        return self._volume_sectors

    @staticmethod
    def _dir_identifier(directory: Path, root: Path) -> bytes:
        return b'\x00' if directory == root else _iso_name(directory).encode()

    @staticmethod
    def _extent_size(listing: Sequence[Path]) -> int:
        # The two self and parent records come first. A record never crosses a sector boundary.
        used = 0
        for name_length in (1, 1, *(len(_iso_name(entry)) for entry in listing)):
            length = 33 + name_length + (1 - name_length % 2)
            if used % _SECTOR_DATA + length > _SECTOR_DATA:
                used = (used // _SECTOR_DATA + 1) * _SECTOR_DATA
            used += length
        return _sectors_for(used) * _SECTOR_DATA

    def _record(self, identifier: bytes, entry: _DirectoryEntry, *, is_dir: bool) -> bytes:
        length = 33 + len(identifier) + (1 - len(identifier) % 2)
        record = bytearray(length)
        record[0] = length
        record[2:10] = _both_endian(entry.lba, 4)
        record[10:18] = _both_endian(entry.size, 4)
        record[18:25] = _record_date(entry.mtime)
        record[25] = 0x02 if is_dir else 0x00
        record[28:32] = _both_endian(1, 2)
        record[32] = len(identifier)
        record[33:33 + len(identifier)] = identifier
        return bytes(record)

    def _write_descriptors(self, root: Path, path_table: int, table_sectors: int,
                           newest: float) -> None:
        descriptor = bytearray(_SECTOR_DATA)
        descriptor[0] = 1
        descriptor[1:6] = _PVD_MAGIC
        descriptor[6] = 1
        descriptor[8:40] = _SYSTEM_IDENTIFIER.ljust(32)
        descriptor[40:72] = b' ' * 32
        descriptor[80:88] = _both_endian(self._volume_sectors, 4)
        descriptor[120:124] = _both_endian(1, 2)
        descriptor[124:128] = _both_endian(1, 2)
        descriptor[128:132] = _both_endian(_SECTOR_DATA, 2)
        descriptor[132:140] = _both_endian(path_table, 4)
        for copy in range(2):
            little = _PATH_TABLE_LBA + copy * table_sectors
            big = _PATH_TABLE_LBA + (2 + copy) * table_sectors
            struct.pack_into('<I', descriptor, 140 + copy * 4, little)
            struct.pack_into('>I', descriptor, 148 + copy * 4, big)
        descriptor[156:190] = self._record(b'\x00', self._dirs[root], is_dir=True)
        descriptor[190:702] = b' ' * 512
        descriptor[574:702] = _SYSTEM_IDENTIFIER.ljust(128)
        descriptor[702:813] = b' ' * 111
        descriptor[813:830] = _volume_date(newest)
        for start in (830, 847, 864):
            descriptor[start:start + 17] = b'0' * 16 + b'\x00'
        descriptor[881] = 1
        self._metadata[_PVD_SECTOR] = bytes(descriptor)
        terminator = bytearray(_SECTOR_DATA)
        terminator[0] = 0xFF
        terminator[1:6] = _PVD_MAGIC
        terminator[6] = 1
        self._metadata[_PVD_SECTOR + 1] = bytes(terminator)

    def _write_path_tables(self, root: Path, directories: Sequence[Path], parents: Sequence[int],
                           table_sectors: int) -> None:
        for copy, order in enumerate(('<', '<', '>', '>')):
            table = bytearray()
            for directory, parent in zip(directories, parents, strict=True):
                identifier = self._dir_identifier(directory, root)
                table += bytes((len(identifier), 0))
                table += struct.pack(f'{order}IH', self._dirs[directory].lba, max(parent, 1))
                table += identifier + b'\x00' * (len(identifier) & 1)
            padded = bytes(table).ljust(table_sectors * _SECTOR_DATA, b'\x00')
            for index in range(table_sectors):
                self._metadata[_PATH_TABLE_LBA + copy * table_sectors + index] = padded[
                    index * _SECTOR_DATA:(index + 1) * _SECTOR_DATA]

    def _write_directory(self, root: Path, directory: Path, listing: Sequence[Path]) -> None:
        own = self._dirs[directory]
        parent = self._dirs[directory.parent if directory != root else root]
        records = [self._record(b'\x00', own, is_dir=True),
                   self._record(b'\x01', parent, is_dir=True)]
        by_path = {entry.path: entry for entry in self._files}
        for entry in listing:
            if entry.is_dir():
                records.append(self._record(_iso_name(entry).encode(), self._dirs[entry],
                                            is_dir=True))
            else:
                placed = by_path[entry]
                records.append(self._record(placed.iso_name.encode(), placed, is_dir=False))
        extent = bytearray(own.size)
        used = 0
        for record in records:
            if used % _SECTOR_DATA + len(record) > _SECTOR_DATA:
                used = (used // _SECTOR_DATA + 1) * _SECTOR_DATA
            extent[used:used + len(record)] = record
            used += len(record)
        for index in range(_sectors_for(own.size)):
            self._metadata[own.lba + index] = bytes(
                extent[index * _SECTOR_DATA:(index + 1) * _SECTOR_DATA])


type _Volume = _SourceImage | _DirectoryVolume


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


def _plan_overlays(source: _Volume, replacements: _Replacements) -> _Plan:
    """
    Place each payload and collect the sectors that change.

    A payload fitting its original extent overwrites the extent with zero padding, and the
    metadata stays untouched. A larger payload moves to sectors appended after the volume,
    and the directory record and volume size move with the payload.

    Parameters
    ----------
    source : _Volume
        Open original image or disc root.
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


def _write_image(source: _Volume, replacements: _Replacements, output: Path) -> int:
    """
    Encode the raw MODE2/2352 image and write its cue sheet.

    Parameters
    ----------
    source : _Volume
        Open original image or disc root.
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
    """Original image in cue, bin, or ISO form, or the disc root directory."""
    output_cue: Path
    """Destination cue sheet path. The bin is written beside it."""
    overwrite: bool
    """Replace the output file when present."""
    pal: bool
    """The executable is a PAL build. A PAL build pairs only with the European disc."""
    repo: str
    """Repository holding the build artifacts."""
    resonance_bin: Path | None
    """Local executable file, bypassing the artifact download."""
    run_id: int | None
    """Actions run to fetch. The latest successful run is used when omitted."""
    system_area: Path | None
    """Boot logo sectors for a disc root input."""
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
    if options.input_image.is_dir() and image.resolve().is_relative_to(
            options.input_image.resolve()):
        message = 'Output must lie outside the disc root.'
        raise DiscImageError(message)
    executable, module = _resolve_payloads(options)
    source: _Volume
    if options.input_image.is_dir():
        system_area = (options.system_area.read_bytes()
                       if options.system_area is not None else None)
        source = _DirectoryVolume(options.input_image, system_area)
    else:
        source = _SourceImage(options.input_image)
    with source:
        target = _boot_executable(source)
        if options.pal != (target == _PAL_EXECUTABLE):
            message = (f'The original boots {target}. A PAL build requires the European disc, '
                       'and an NTSC build requires the North American disc.')
            raise DiscImageError(message)
        log.info('Original is the %s release (%s).', _KNOWN_EXECUTABLES[target], target)
        return _write_image(source, {target: executable, _TARGET_MODULE: module},
                            options.output_cue)


def _boot_executable(source: _Volume) -> str:
    """
    Read the name of the executable the disc boots.

    Parameters
    ----------
    source : _Volume
        Open original image or disc root.

    Returns
    -------
    str
        File name from the `BOOT2` line of `SYSTEM.CNF`, without the version suffix.

    Raises
    ------
    DiscImageError
        `SYSTEM.CNF` does not have a `BOOT2` line, or the line identifies an unknown executable.
    """
    found = source.find(_SYSTEM_CNF)
    data = b''.join(source.read_sector(found.lba + index)
                    for index in range(_sectors_for(found.size)))[:found.size]
    return _parse_boot2(data)


def _identify(input_image: Path) -> str:
    """
    Read the name of the executable an original disc boots, without building anything.

    Parameters
    ----------
    input_image : Path
        Original image in cue, bin, or ISO form, or the disc root directory.

    Returns
    -------
    str
        File name from the `BOOT2` line of `SYSTEM.CNF`, without the version suffix.

    Raises
    ------
    DiscImageError
        The disc cannot be read, or `SYSTEM.CNF` does not boot a known release.
    """
    if input_image.is_dir():
        for entry in input_image.iterdir():
            if entry.name.upper() == _SYSTEM_CNF:
                return _parse_boot2(entry.read_bytes())
        message = f'{input_image} lacks {_SYSTEM_CNF} and is not the root of a PlayStation 2 disc.'
        raise DiscImageError(message)
    with _SourceImage(input_image) as source:
        return _boot_executable(source)


def _parse_boot2(data: bytes) -> str:
    """
    Read the executable name from the text of `SYSTEM.CNF`.

    Parameters
    ----------
    data : bytes
        Contents of `SYSTEM.CNF`.

    Returns
    -------
    str
        File name from the `BOOT2` line, without the version suffix.

    Raises
    ------
    DiscImageError
        The text does not have a `BOOT2` line, or the line identifies an unknown executable.
    """
    if (match := _BOOT2_RE.search(data.decode('ascii', 'replace'))) is None:
        message = f'{_SYSTEM_CNF} has no BOOT2 line.'
        raise DiscImageError(message)
    name = match['name'].upper()
    if name not in _KNOWN_EXECUTABLES:
        message = f'{_SYSTEM_CNF} boots {name}. No known FreQuency release boots it.'
        raise DiscImageError(message)
    return name


def _resolve_payloads(options: _Options) -> tuple[bytes, bytes]:
    """
    Fetch or read both replacement payloads.

    Parameters
    ----------
    options : _Options
        Command line selections.

    Returns
    -------
    tuple[bytes, bytes]
        The executable and the module.

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
    return executable, module


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


def _existing_path(value: str) -> Path:
    """
    Convert an argument to the path of an existing file or directory.

    Parameters
    ----------
    value : str
        Argument text.

    Returns
    -------
    Path
        The path.

    Raises
    ------
    argparse.ArgumentTypeError
        The path does not exist.
    """
    path = Path(value)
    if not path.exists():
        message = f'No such file or directory: {value}.'
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
    parser.add_argument('input_image',
                        help='Original image in cue, bin, or ISO form, or the disc root directory '
                        '(the directory with SYSTEM.CNF).',
                        type=_existing_path)
    parser.add_argument('output_cue', help='Destination cue sheet path. Required unless '
                        '--identify is given.', nargs='?', type=Path)
    parser.add_argument('--identify', action='store_true',
                        help='Print the name of the executable the original boots, and exit.')
    parser.add_argument('--pal', action='store_true',
                        help='The executable is a PAL build. The European disc (SCES_507.91) '
                        'requires the flag, and the North American disc refuses it.')
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
    parser.add_argument('--system-area',
                        help='The first 12 sectors of the original disc (24576 bytes, the boot '
                        'logo), for a disc root input. The sectors are zero otherwise.',
                        type=_existing_file)
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
    parser = _build_parser()
    args = parser.parse_args(argv)
    logging.basicConfig(format='%(levelname)s: %(message)s',
                        level=logging.DEBUG if args.debug else logging.INFO)
    if args.identify:
        try:
            print(_identify(args.input_image))  # ruff: ignore[print]
        except (DiscImageError, OSError):
            log.exception('Cannot identify the original.')
            return 1
        return 0
    if args.output_cue is None:
        parser.error('the output cue sheet is required')
    options = _Options(debug=args.debug, ezmidi_irx=args.ezmidi_irx, input_image=args.input_image,
                       output_cue=args.output_cue, overwrite=args.overwrite, pal=args.pal,
                       repo=args.repo, resonance_bin=args.resonance_bin, run_id=args.run_id,
                       system_area=args.system_area, token=args.token)
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
