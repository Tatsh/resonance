#pragma once

#include "met/metstringid.h"
#include "os/hxstr.h"
#include "script/configquery.h"
#include "script/scripttemplatemap.h"

#ifdef VIDEO_STANDARD_PAL
/**
 * Front-end text of the current language, by string identifier.
 *
 * LoadMetStrings() fills it, and GetMetString() reads it. The European release added it, in place
 * of the `get_met_string()` script function the North American release evaluates for each text.
 * The name is inferred.
 *
 * @ghidraAddress PAL: 0x00710a38
 */
extern ScriptTemplateMap g_metStrings;

/**
 * Load the front-end text of the current language into g_metStrings.
 *
 * The file is `metagame/strings/gamestrings_<suffix>.txt` under the frequency root, with the suffix
 * `fr`, `ita`, `sp`, `ger`, or `en` chosen by GetLanguage(). It is read a line at a time. A line
 * starting with `//` is a comment. Text in braces opens a section, and the brace that closes it
 * rounds the next identifier up to the next multiple of 100. Each entry is a key, a comma, and a
 * quoted value, and the value is stored under the next identifier. The key text is discarded. A
 * value accepts the escapes `\\`, `\n`, `\'`, and `\"`. MetRenderer's constructor is the one
 * caller. The name is inferred.
 *
 * @ghidraAddress PAL: 0x003f9bb0
 */
void LoadMetStrings();

/**
 * Look a front-end text up by identifier.
 *
 * @param nId The identifier.
 * @return A copy of the text, or an empty string when none is loaded under the identifier.
 * @ghidraAddress PAL: 0x003fb7b0
 */
HxStr GetMetString(int nId);
#endif

/**
 * A text the North American release writes into the executable.
 *
 * The European release looks the same text up in the current language instead. The helper picks
 * the release's behaviour. A call site gives both forms once.
 *
 * @param nId The identifier the European release looks up.
 * @param pszText The text the North American release uses.
 * @return The text.
 */
inline HxStr MetText([[maybe_unused]] MetStringId nId, [[maybe_unused]] const char *pszText) {
#ifdef VIDEO_STANDARD_PAL
    return GetMetString(nId);
#else
    return HxStr(pszText);
#endif
}

/**
 * A text the North American release reads through a configuration query.
 *
 * The North American release evaluates `get_met_string('<key>')` through QueryConfigString(), and
 * the European release looks the text up in the current language instead.
 *
 * @param nId The identifier the European release looks up.
 * @param nConfigCode The configuration code the North American release queries.
 * @param pszKey The key the North American release queries.
 * @return The text.
 */
inline HxStr MetConfigText([[maybe_unused]] MetStringId nId,
                           [[maybe_unused]] int nConfigCode,
                           [[maybe_unused]] const char *pszKey) {
#ifdef VIDEO_STANDARD_PAL
    return GetMetString(nId);
#else
    return QueryConfigString(nConfigCode, pszKey);
#endif
}
