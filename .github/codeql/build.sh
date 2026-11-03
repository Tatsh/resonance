#!/usr/bin/env bash
set -euo pipefail

case "$CODEQL_COMPONENT" in
  game)
    cmake --build build-codeql --target archives --parallel 4
    ;;
  iop)
    cmake --build build-codeql --target ezmidi_irx --parallel 4
    ;;
  tools)
    cmake -S tools -B build-codeql-tools -G Ninja -DCMAKE_BUILD_TYPE=Release
    cmake --build build-codeql-tools --parallel 4
    ;;
  plugin)
    plugin_dir=$(mips64r5900el-ps2-elf-gcc -print-file-name=plugin)
    c++ -shared -fPIC -fno-rtti -O2 "-I$plugin_dir/include" \
      cmake/ieee-single-plugin.cpp -o "$RUNNER_TEMP/ieee-single-plugin.so"
    ;;
  *)
    printf 'The CodeQL component %s is unknown.\n' "$CODEQL_COMPONENT" >&2
    exit 1
    ;;
esac
