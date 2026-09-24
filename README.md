# FreQuency

<!-- WISWA-GENERATED-README:START -->

[![C++](https://img.shields.io/badge/C++-00599C?logo=c%2B%2B)](https://isocpp.org)
[![GitHub tag (with filter)](https://img.shields.io/github/v/tag/Tatsh/resonance)](https://github.com/Tatsh/resonance/tags)
[![License](https://img.shields.io/github/license/Tatsh/resonance)](https://github.com/Tatsh/resonance/blob/master/LICENSE.txt)
[![GitHub commits since latest release (by SemVer including pre-releases)](https://img.shields.io/github/commits-since/Tatsh/resonance/v0.0.0/master)](https://github.com/Tatsh/resonance/compare/v0.0.0...master)
[![Dependabot](https://img.shields.io/badge/Dependabot-enabled-blue?logo=dependabot)](https://github.com/dependabot)
[![pages-build-deployment](https://github.com/Tatsh/resonance/actions/workflows/pages/pages-build-deployment/badge.svg)](https://tatsh.github.io/resonance/)
[![Stargazers](https://img.shields.io/github/stars/Tatsh/resonance?logo=github&style=flat)](https://github.com/Tatsh/resonance/stargazers)
[![pre-commit.ci status](https://results.pre-commit.ci/badge/github/Tatsh/resonance/master.svg)](https://results.pre-commit.ci/latest/github/Tatsh/resonance/master)
[![CMake](https://img.shields.io/badge/CMake-6E6E6E?logo=cmake)](https://cmake.org/)
[![Prettier](https://img.shields.io/badge/Prettier-black?logo=prettier)](https://prettier.io/)

[![@Tatsh](https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fpublic.api.bsky.app%2Fxrpc%2Fapp.bsky.actor.getProfile%2F%3Factor=did%3Aplc%3Auq42idtvuccnmtl57nsucz72&query=%24.followersCount&label=Follow+%40Tatsh&logo=bluesky&style=social)](https://bsky.app/profile/Tatsh.bsky.social)
[![Buy Me A Coffee](https://img.shields.io/badge/Buy%20Me%20a%20Coffee-Tatsh-black?logo=buymeacoffee)](https://buymeacoffee.com/Tatsh)
[![Libera.Chat](https://img.shields.io/badge/Libera.Chat-Tatsh-black?logo=liberadotchat)](irc://irc.libera.chat/Tatsh)
[![Mastodon Follow](https://img.shields.io/mastodon/follow/109370961877277568?domain=hostux.social&style=social)](https://hostux.social/@Tatsh)
[![Patreon](https://img.shields.io/badge/Patreon-Tatsh2-F96854?logo=patreon)](https://www.patreon.com/Tatsh2)

<!-- WISWA-GENERATED-README:STOP -->

Reconstructed source code for _FreQuency_, the 2001 PlayStation 2 rhythm game developed by Harmonix
Music Systems and published by Sony Computer Entertainment.

## Status

This is an active, partial reconstruction. It does not build a playable game yet, and it is not a
port. Over nine tenths of the game's routines are accounted for and nearly nine tenths have a
body, with the current figures and how they are measured in [PROGRESS.md](PROGRESS.md).

| Area                               | State                                        |
| ---------------------------------- | -------------------------------------------- |
| Start-up and the application shell | Recovered                                    |
| Memory, archives, and file loading | Recovered                                    |
| Renderer object model and streams  | Recovered                                    |
| Meshes, materials, and textures    | Recovered                                    |
| Art and canvas library             | Recovered apart from two routines            |
| Game messages and packets          | Recovered                                    |
| Memory card                        | Recovered                                    |
| Front end screens and the metagame | Recovered                                    |
| Gameplay                           | Recovered                                    |
| Audio and the synthesiser          | Recovered                                    |
| Embedded Python interpreter        | Characterised, with its differences recorded |
| Networking                         | Packet records only                          |

Recovered means every routine the game's code defines has a body or a recorded reason for
having none. Cross-reviews against the disassembly continue to correct bodies in every area.

## Layout

| Path        | Contents                                 |
| ----------- | ---------------------------------------- |
| `include/`  | Headers, grouped by subsystem            |
| `src/`      | Implementations, mirroring `include/`    |
| `3rdparty/` | Third-party code the game linked against |

Within those, `rnd` is the renderer and `rndartt` its art and canvas library, `os` the memory and
file layer, `app` the application shell, `msg` the message and packet classes, `math` the vector and
transform types, `gfx` the display device, `sch` the command scheduler, `synth` and `mid` the audio
path, `game` and `gs` the gameplay classes, `met` the front end screens and the metagame, and
`memcard` the save and load tasks. `src/python` records the differences between the game's embedded
Python interpreter and the public release it was built from.

A class whose name begins `Ps` is the PlayStation 2 implementation of the portable class above it.

[PROGRESS.md](PROGRESS.md) records how much is recovered, how the figures are measured, and the
conventions the source follows.

## Building

CI compiles every buildable source with the Emotion Engine cross compiler in the ps2dev container
and archives one static library per subsystem. The libraries are not linked yet, and the embedded
interpreter and its C++ binding are not built.

## Provenance and licence

This is an independent reverse-engineering effort for preservation and study. It includes no code
and no assets copied from the game. _FreQuency_ and its assets remain the property of their
respective rights holders, and this project is not affiliated with or endorsed by Harmonix or
Sony Interactive Entertainment.

Third-party code the game linked against, including the Python interpreter and its C++ binding
layer, remains under its licence and is identified as such under `3rdparty/` and `src/python/`.
