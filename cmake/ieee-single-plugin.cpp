// GCC plugin that makes the R5900 target round single-precision constants to nearest.
//
// The upstream compiler gives the R5900 a single-precision format that truncates toward zero. The
// retail build rounded to nearest, so 0.2f became 0x3e4ccccd there but 0x3e4ccccc here. Once
// option processing has installed the truncating format, the plugin swaps in a copy that rounds to
// nearest. The copy retains the format's lack of NaNs and infinities. Instruction selection depends
// on the absence of both.
// This file is build support for the modern toolchain, not part of the reconstructed source.

#include "gcc-plugin.h"

#include "plugin-version.h"

#include "coretypes.h"
#include "real.h"
#include "tm.h"

int plugin_is_GPL_compatible;

namespace {

real_format g_roundingSingleFormat;

void restoreIeeeSingle(void *, void *) {
    g_roundingSingleFormat = *REAL_MODE_FORMAT(SFmode);
    g_roundingSingleFormat.round_towards_zero = false;
    REAL_MODE_FORMAT(SFmode) = &g_roundingSingleFormat;
}

} // namespace

int plugin_init(plugin_name_args *pluginInfo, plugin_gcc_version *version) {
    if (!plugin_default_version_check(version, &gcc_version)) {
        return 1;
    }
    register_callback(pluginInfo->base_name, PLUGIN_START_UNIT, restoreIeeeSingle, nullptr);
    return 0;
}
