"""Write an IRX module's name into its `.iopmod` header.

The ps2dev `srxfixup` leaves the name in the `.iopmod` header empty, while the IOP modules on the
disc carry it there. This reads the name from the module's `ModuleInfo` record, which the header
already points to, writes it into the header, and moves the rest of the file by the growth,
rounded to the alignment of the loadable segment.
"""

from __future__ import annotations

from pathlib import Path
from typing import NamedTuple
import argparse
import logging
import struct
import sys

log = logging.getLogger(__name__)

__all__ = ('IrxError', 'main')


class IrxError(Exception):
    """The file is not an IRX this script can rewrite."""


_ELF_MAGIC = b'\x7fELF'
_ELF_HEADER_FORMAT = '<16sHHIIIIIHHHHHH'
_SECTION_HEADER_FORMAT = '<10I'
_SECTION_HEADER_SIZE = 40
_PROGRAM_HEADER_FORMAT = '<8I'
_PROGRAM_HEADER_SIZE = 32
_SHT_PROGBITS = 1
_SHT_NOBITS = 8
_SHT_IOPMOD = 0x70000080
_PT_IOPMOD = 0x70000080
_PT_LOAD = 1
# The fixed part of the `.iopmod` record: the ModuleInfo address, the entry, the gp value, the
# text, data, and bss sizes, and the version.
_IOPMOD_FIXED_FORMAT = '<6IH'
_IOPMOD_FIXED_SIZE = struct.calcsize(_IOPMOD_FIXED_FORMAT)
_NAME_LIMIT = 256


class _Section(NamedTuple):
    """One section header."""

    index: int
    """Position in the section header table."""
    fields: list[int]
    """The ten header words, in file order."""


def _file_offset(sections: list[_Section], address: int) -> int:
    """
    Convert a module address inside a section with file contents to a file offset.

    Parameters
    ----------
    sections : list[_Section]
        Every section header.
    address : int
        Module address.

    Returns
    -------
    int
        The file offset.

    Raises
    ------
    IrxError
        No section with file contents covers the address.
    """
    for section in sections:
        sh_type, sh_addr, sh_offset, sh_size = (section.fields[1], section.fields[3],
                                                section.fields[4], section.fields[5])
        if sh_type == _SHT_PROGBITS and sh_addr <= address < sh_addr + sh_size:
            return sh_offset + address - sh_addr
    message = f'No section covers module address {address:#x}.'
    raise IrxError(message)


def name_irx(data: bytes) -> bytes:
    """
    Return the IRX with the module name written into its `.iopmod` header.

    Parameters
    ----------
    data : bytes
        The IRX file.

    Returns
    -------
    bytes
        The rewritten file, or the input when the header already names the module.

    Raises
    ------
    IrxError
        The file is not an IRX, or its layout cannot be rewritten.
    """
    if data[:4] != _ELF_MAGIC:
        message = 'Not an ELF file.'
        raise IrxError(message)
    header = list(struct.unpack_from(_ELF_HEADER_FORMAT, data))
    e_phoff, e_shoff, e_phnum, e_shnum = header[5], header[6], header[10], header[12]
    sections = [
        _Section(index,
                 list(struct.unpack_from(_SECTION_HEADER_FORMAT, data,
                                         e_shoff + index * _SECTION_HEADER_SIZE)))
        for index in range(e_shnum)
    ]
    iopmod = next((s for s in sections if s.fields[1] == _SHT_IOPMOD), None)
    if iopmod is None:
        message = 'No .iopmod section.'
        raise IrxError(message)
    iopmod_offset, iopmod_size = iopmod.fields[4], iopmod.fields[5]
    record = data[iopmod_offset:iopmod_offset + iopmod_size]
    if record[_IOPMOD_FIXED_SIZE:].split(b'\0', 1)[0]:
        return data
    module_info = struct.unpack_from(_IOPMOD_FIXED_FORMAT, record)[0]
    name_pointer = struct.unpack_from('<I', data, _file_offset(sections, module_info))[0]
    name_offset = _file_offset(sections, name_pointer)
    name = data[name_offset:name_offset + _NAME_LIMIT].split(b'\0', 1)[0]

    programs = [
        list(struct.unpack_from(_PROGRAM_HEADER_FORMAT, data, e_phoff + i * _PROGRAM_HEADER_SIZE))
        for i in range(e_phnum)
    ]
    load = next((p for p in programs if p[0] == _PT_LOAD), None)
    if load is None:
        message = 'No loadable segment.'
        raise IrxError(message)
    alignment = max(load[7], 1)
    old_rest = load[1]
    if old_rest < iopmod_offset + iopmod_size:
        message = 'The loadable segment overlaps the .iopmod record.'
        raise IrxError(message)
    new_record = record[:_IOPMOD_FIXED_SIZE] + name + b'\0'
    new_rest = -(-(iopmod_offset + len(new_record)) // alignment) * alignment
    delta = new_rest - old_rest

    out = bytearray(data[:iopmod_offset] + new_record)
    out += bytes(new_rest - len(out))
    out += data[old_rest:]
    for program in programs:
        if program[0] == _PT_IOPMOD:
            program[4] = len(new_record)
        elif program[1] >= old_rest:
            program[1] += delta
    for section in sections:
        if section.index == iopmod.index:
            section.fields[5] = len(new_record)
        elif section.fields[4] >= old_rest:
            section.fields[4] += delta
    if e_shoff >= old_rest:
        header[6] = e_shoff + delta
    struct.pack_into(_ELF_HEADER_FORMAT, out, 0, *header)
    for i, program in enumerate(programs):
        struct.pack_into(_PROGRAM_HEADER_FORMAT, out, e_phoff + i * _PROGRAM_HEADER_SIZE,
                         *program)
    for section in sections:
        struct.pack_into(_SECTION_HEADER_FORMAT, out,
                         header[6] + section.index * _SECTION_HEADER_SIZE, *section.fields)
    log.debug('Named the module `%s`.', name.decode('ascii', errors='replace'))
    return bytes(out)


def main(argv: list[str] | None = None) -> int:
    """
    Write the module name into an IRX's `.iopmod` header, in place.

    Parameters
    ----------
    argv : list[str] | None
        Arguments after the program name. The process arguments are used when omitted.

    Returns
    -------
    int
        The exit status, 0 on success and 1 on failure.
    """
    parser = argparse.ArgumentParser(
        description="Write an IRX module's name into its .iopmod header, in place.")
    parser.add_argument('irx', help='The IRX file to rewrite.', type=Path)
    parser.add_argument('--debug', action='store_true', help='Enable debug logging.')
    args = parser.parse_args(argv)
    logging.basicConfig(format='%(levelname)s: %(message)s',
                        level=logging.DEBUG if args.debug else logging.INFO)
    try:
        data = args.irx.read_bytes()
        named = name_irx(data)
        if named != data:
            args.irx.write_bytes(named)
    except (IrxError, OSError):
        log.exception('Naming the module failed.')
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
