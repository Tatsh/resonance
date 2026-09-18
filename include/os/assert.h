#pragma once

/**
 * Report a failed check and continue.
 *
 * The routine does not stop execution. Every call site in the binary falls
 * through into the code the check was protecting, so a failed check produces a
 * report and then whatever the unguarded code does.
 *
 * @param pszFile The reporting file, from `__FILE__`.
 * @param nLine The reporting line, from `__LINE__`.
 * @param pszExpression The source text of the failed expression.
 * @ghidraAddress 0x0055c548
 */
void HxAssertFailed(const char *pszFile, int nLine, const char *pszExpression);

// Every assert in the shipped code expands to this shape. The expression text,
// the file, and the line all survive in .rodata, which is what fixes the line
// numbers recorded against the reconstructed routines.
#define HX_ASSERT(expression)                                                                      \
    if (!(expression)) {                                                                           \
        HxAssertFailed(__FILE__, __LINE__, #expression);                                           \
    }
