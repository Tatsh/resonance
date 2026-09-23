#include "script/configquery.h"

#include <stdarg.h>
#include <vector>

#include "os/hxstr.h"
#include "os/log.h"
#include "script/cxx/config.h"
#include "script/cxx/exception.h"
#include "script/cxx/int.h"
#include "script/cxx/seqbase.h"
#include "script/cxx/string.h"
#include "script/scripteval.h"
#include "script/scripttemplatemap.h"

namespace {

// The text Fatal() reports a failed query with.
inline const char *ExpressionText(const HxStr &expression) {
    return expression.mStr != nullptr ? expression.mStr : g_szEmptyString;
}

} // namespace

// 0x00509110
int QueryConfigValue(int nEventCode, ...) {
    va_list args;
    va_start(args, nEventCode);
    const HxStr expression = FormatMessage(GetScriptTemplate(nEventCode), args);
    va_end(args);

    try {
        return static_cast<long>(Py::Int(EvalScriptExpression(expression)));
    } catch (Py::Exception &) {
        Fatal("%s does not evaluate to an int", ExpressionText(expression));
        PyErr_Clear();
    }
    return 0;
}

// 0x005093e0
int QueryConfigFlag(int nEventCode, ...) {
    va_list args;
    va_start(args, nEventCode);
    const HxStr expression = FormatMessage(GetScriptTemplate(nEventCode), args);
    va_end(args);

    try {
        return (static_cast<long>(Py::Int(EvalScriptExpression(expression))) != 0) ? 1 : 0;
    } catch (Py::Exception &) {
        Fatal("%s does not evaluate to an int", ExpressionText(expression));
        PyErr_Clear();
    }
    return 0;
}

// 0x005096d0
HxStr *QueryConfigString(HxStr *pResult, int nEventCode, ...) {
    va_list args;
    va_start(args, nEventCode);
    const HxStr expression = FormatMessage(GetScriptTemplate(nEventCode), args);
    va_end(args);

    try {
        *pResult = static_cast<HxStr>(Py::String(EvalScriptExpression(expression)));
    } catch (Py::Exception &) {
        Fatal("%s does not evaluate to a HxStr", ExpressionText(expression));
        PyErr_Clear();
        *pResult = "";
    }
    return pResult;
}

// 0x00509b78
void QueryConfigStrings(std::vector<HxStr> *pResult, int nEventCode, ...) {
    va_list args;
    va_start(args, nEventCode);
    const HxStr expression = FormatMessage(GetScriptTemplate(nEventCode), args);
    va_end(args);

    try {
        Py::Sequence sequence(EvalScriptExpression(expression));
        pResult->clear();
        for (int i = 0; i < sequence.length(); ++i) {
            pResult->push_back(static_cast<HxStr>(Py::String(sequence.getItem(i))));
        }
    } catch (Py::Exception &) {
        Fatal("%s does not evaluate to a sequence", ExpressionText(expression));
        PyErr_Clear();
    }
}

// 0x0050a1a0
void QueryConfigVector(std::vector<int> *pResult, int nEventCode, ...) {
    va_list args;
    va_start(args, nEventCode);
    const HxStr expression = FormatMessage(GetScriptTemplate(nEventCode), args);
    va_end(args);

    try {
        Py::Sequence sequence(EvalScriptExpression(expression));
        pResult->clear();
        for (int i = 0; i < sequence.length(); ++i) {
            pResult->push_back(static_cast<long>(Py::Int(sequence.getItem(i))));
        }
    } catch (Py::Exception &) {
        Fatal("%s does not evaluate to a sequence", ExpressionText(expression));
        PyErr_Clear();
    }
}
