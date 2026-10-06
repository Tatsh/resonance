// GCC plugin that makes the R5900 target round single-precision constants to nearest.
//
// The upstream compiler gives the R5900 a single-precision format that truncates toward zero. The
// retail build rounded to nearest, so 0.2f became 0x3e4ccccd there but 0x3e4ccccc here. Once
// option processing has installed the truncating format, the plugin swaps in a copy that rounds to
// nearest. The copy retains the format's lack of NaNs and infinities. Instruction selection depends
// on the absence of both.
//
// The plugin also negates a double the way the retail compiler did. That compiler computed -x as
// 0.0 - x through the soft-float subtract (float_neg at 0x0047b788, unpack_double at 0x006324a0).
// Negating a zero therefore gave positive zero. The upstream compiler flips the sign bit instead. A
// late GIMPLE pass rewrites each double negation as the subtraction. Later passes retain the
// subtraction because signed zeros are honoured.
// This file is build support for the modern toolchain, not part of the reconstructed source.

// The compiler's internal headers depend on one another and only build in this order.
// clang-format off
#include "gcc-plugin.h"
#include "plugin-version.h"
#include "coretypes.h"
#include "tm.h"
#include "real.h"
#include "backend.h"
#include "tree.h"
#include "gimple.h"
#include "gimple-iterator.h"
#include "gimple-ssa.h"
#include "tree-pass.h"
#include "context.h"
// clang-format on

int plugin_is_GPL_compatible;

namespace {

real_format g_roundingSingleFormat;

void restoreIeeeSingle(void *, void *) {
    g_roundingSingleFormat = *REAL_MODE_FORMAT(SFmode);
    g_roundingSingleFormat.round_towards_zero = false;
    REAL_MODE_FORMAT(SFmode) = &g_roundingSingleFormat;
}

const pass_data kDoubleNegationPassData = {
    GIMPLE_PASS,
    "retail_double_negation",
    OPTGROUP_NONE,
    TV_NONE,
    PROP_gimple_any,
    0,
    0,
    0,
    TODO_update_ssa,
};

class DoubleNegationPass : public gimple_opt_pass {
public:
    explicit DoubleNegationPass(gcc::context *pContext)
        : gimple_opt_pass(kDoubleNegationPassData, pContext) {
    }

    unsigned int execute(function *pFunction) override {
        basic_block block;
        FOR_EACH_BB_FN(block, pFunction) {
            for (auto gsi = gsi_start_bb(block); !gsi_end_p(gsi); gsi_next(&gsi)) {
                gimple *pStatement = gsi_stmt(gsi);
                if (!is_gimple_assign(pStatement) ||
                    gimple_assign_rhs_code(pStatement) != NEGATE_EXPR) {
                    continue;
                }
                const tree type = TREE_TYPE(gimple_assign_lhs(pStatement));
                if (!SCALAR_FLOAT_TYPE_P(type) || TYPE_MODE(type) != DFmode) {
                    continue;
                }
                const tree operand = gimple_assign_rhs1(pStatement);
                gimple_assign_set_rhs_with_ops(
                    &gsi, MINUS_EXPR, build_real(type, dconst0), operand);
                update_stmt(gsi_stmt(gsi));
            }
        }
        return 0;
    }
};

} // namespace

int plugin_init(plugin_name_args *pluginInfo, plugin_gcc_version *version) {
    if (!plugin_default_version_check(version, &gcc_version)) {
        return 1;
    }
    register_callback(pluginInfo->base_name, PLUGIN_START_UNIT, restoreIeeeSingle, nullptr);
    register_pass_info passInfo;
    passInfo.pass = new DoubleNegationPass(g);
    passInfo.reference_pass_name = "optimized";
    passInfo.ref_pass_instance_number = 1;
    passInfo.pos_op = PASS_POS_INSERT_AFTER;
    register_callback(pluginInfo->base_name, PLUGIN_PASS_MANAGER_SETUP, nullptr, &passInfo);
    return 0;
}
