"""Write the header with the credits avatar of patched builds.

The header defines the credit text and a square RGBA texture of a GitHub avatar, with alpha already
on the GS scale, where 0x80 is opaque. The avatar is fetched from GitHub and converted with
ImageMagick. When it cannot be fetched or converted, the texture is blank.
"""

from __future__ import annotations

from pathlib import Path
from typing import TYPE_CHECKING
import argparse
import logging
import subprocess as sp
import sys
import urllib.error
import urllib.request

if TYPE_CHECKING:
    from collections.abc import Sequence

log = logging.getLogger(__name__)

__all__ = ('main',)

_AVATAR_URL = 'https://github.com/{user}.png?size={size}'
_HTTP_TIMEOUT = 15
_GS_OPAQUE = 0x80
_BYTES_PER_LINE = 16


def _fetch_avatar(user: str, size: int, magick: str) -> bytes | None:
    """
    Fetch a GitHub avatar and convert it to RGBA texels.

    Parameters
    ----------
    user : str
        GitHub user name.
    size : int
        Width and height of the texture in pixels.
    magick : str
        The ImageMagick command, or an empty string when ImageMagick is not installed.

    Returns
    -------
    bytes | None
        The texels with GS alpha, or ``None`` when the avatar cannot be fetched or converted.
    """
    if not magick:
        log.warning('ImageMagick is not installed. The credits avatar is blank.')
        return None
    url = _AVATAR_URL.format(size=size, user=user)
    try:
        with urllib.request.urlopen(  # ruff: ignore[suspicious-url-open-usage]
                url, timeout=_HTTP_TIMEOUT) as response:
            data = response.read()
    except (OSError, urllib.error.URLError):
        log.warning('Cannot fetch %s. The credits avatar is blank.', url)
        return None
    try:
        texels = bytearray(
            sp.run((magick, '-', '-resize', f'{size}x{size}!', '-depth', '8', 'rgba:-'),
                   capture_output=True,
                   check=True,
                   input=data).stdout)
    except (OSError, sp.CalledProcessError):
        log.warning('Cannot convert the avatar from %s. The credits avatar is blank.', url)
        return None
    if len(texels) != size * size * 4:
        log.warning('The converted avatar has the wrong size. The credits avatar is blank.')
        return None
    for alpha in range(3, len(texels), 4):
        texels[alpha] = (texels[alpha] + 1) >> 1
    return bytes(texels)


def _c_string(text: str) -> str:
    """
    Quote text as a C string literal.

    Parameters
    ----------
    text : str
        The text.

    Returns
    -------
    str
        The literal, with escapes for backslashes, quotes, and newlines.
    """
    return '"' + text.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n') + '"'


def _header(text: str, size: int, texels: bytes | None) -> str:
    """
    Compose the header.

    Parameters
    ----------
    text : str
        The credit text.
    size : int
        Width and height of the texture in pixels.
    texels : bytes | None
        The texels, or ``None`` for a blank texture.

    Returns
    -------
    str
        The header text.
    """
    if texels is None:
        texels = bytes([0, 0, 0, _GS_OPAQUE]) * (size * size)
    lines = [
        '#pragma once', '', '#include <stdint.h>', '',
        f'#define RESONANCE_CREDITS_TEXT {_c_string(text)}',
        f'#define RESONANCE_CREDITS_AVATAR_SIZE {size}', '',
        '// The texels are RGBA, with alpha on the GS scale.',
        'alignas(16) static const uint8_t kCreditsAvatarTexels[] = {'
    ]
    lines += [
        '    ' + ', '.join(f'0x{b:02x}' for b in texels[i:i + _BYTES_PER_LINE]) + ','
        for i in range(0, len(texels), _BYTES_PER_LINE)
    ]
    lines += ['};', '']
    return '\n'.join(lines)


def main(argv: Sequence[str] | None = None) -> int:
    """
    Write the credits avatar header.

    Parameters
    ----------
    argv : Sequence[str] | None
        Arguments after the program name. The process arguments are used when omitted.

    Returns
    -------
    int
        The exit status, 0 on success and 1 on failure.
    """
    parser = argparse.ArgumentParser(description='Write the credits avatar header.')
    parser.add_argument('output', help='The header to write.', type=Path)
    parser.add_argument('--magick', default='magick',
                        help='The ImageMagick command, or an empty string for a blank avatar. '
                        'Defaults to %(default)s.')
    parser.add_argument('--size',
                        default=128,
                        help='Texture size in pixels. Defaults to %(default)s.',
                        type=int)
    parser.add_argument('--text', required=True, help='The credit text. \\n breaks a line.')
    parser.add_argument('--user', required=True, help='The GitHub user whose avatar is used.')
    args = parser.parse_args(argv)
    logging.basicConfig(format='%(levelname)s: %(message)s', level=logging.INFO)
    text = args.text.replace('\\n', '\n')
    try:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(
            _header(text, args.size, _fetch_avatar(args.user, args.size, args.magick)),
            encoding='utf-8')
    except OSError:
        log.exception('Writing the credits avatar header failed.')
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
