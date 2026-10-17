<!-- markdownlint-configure-file {"MD024": { "siblings_only": true } } -->

# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.1/), and this project
adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [unreleased]

## [1.0.1] - 2026-10-17

### Changed

- A Debug build's disc image includes the executable with debug information.
  `RESONANCE_IMAGE_STRIP` now defaults to `OFF` when `CMAKE_BUILD_TYPE` is `Debug`.
- `3rdparty/README.md` records upstream archive URL, SHA-256, imported files, and local changes
  of each vendored release.

### Fixed

- `build-image` gives a replaced file the size of its payload and the modification time of its
  input. Parent directories and the primary volume descriptor are raised to the input modification
  time when older, and the verify step checks each directory record.
- The vendored Python interpreter builds without warnings.

## [1.0.0] - 2026-10-06

First release.

[unreleased]: https://github.com/Tatsh/resonance/compare/v1.0.1...HEAD
[1.0.1]: https://github.com/Tatsh/resonance/compare/v1.0.0...v1.0.1
[1.0.0]: https://github.com/Tatsh/resonance/releases/tag/v1.0.0
