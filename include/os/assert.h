#pragma once

/**
 * Report a failed check and stop the machine.
 *
 * The routine is newlib's own `__assert`. Its report goes to `stderr`, taken from the reentrancy
 * structure at 0x007819cc, through the format `assertion "%s" failed: file "%s", line %d`, which
 * is that library's text verbatim. It then tail-calls abort, which raises signal 6, calls exit, and
 * spins forever.
 *
 * Every call site in the binary falls through into the code the check was protecting. That is
 * because the declaration the game compiled against did not mark the routine as never returning,
 * and not because the routine returns. It does not return.
 *
 * @param pszFile The reporting file, from `__FILE__`.
 * @param nLine The reporting line, from `__LINE__`.
 * @param pszExpression The source text of the failed expression.
 * @ghidraAddress 0x0055c548
 */
void HxAssertFailed(const char *pszFile, int nLine, const char *pszExpression);

// Every assert in the shipped code expands to this shape, which is the shape of newlib's own
// assert() macro. The expression text, the file, and the line all survive in .rodata, which is what
// fixes the line numbers recorded against the reconstructed routines.
#define HX_ASSERT(expression)                                                                      \
    if (!(expression)) {                                                                           \
        HxAssertFailed(__FILE__, __LINE__, #expression);                                           \
    }
