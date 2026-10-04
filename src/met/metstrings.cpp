#include "met/metstrings.h"

#ifdef VIDEO_STANDARD_PAL
#include <libscf.h>
#include <stdio.h>

#include "os/hostmode.h"

namespace {

// Bytes of one line fgets() reads.
constexpr int kLineSize = 1024;
// Identifiers of each section start at a multiple of this stride.
constexpr int kSectionStride = 100;

// Parser state bits.
enum MetStringsState {
    kStateInSection = 1, // Between the braces of a section name.
    kStateInKey = 2,     // In the key before the comma.
    kStateInValue = 4,   // Between the quotes of a value.
};

} // namespace

// PAL: 0x00710a38
ScriptTemplateMap g_metStrings;

void LoadMetStrings() {
    HxStr path = GetFreqRoot() + "metagame/strings/";
    switch (GetLanguage()) {
    case SCE_FRENCH_LANGUAGE:
        path += HxStr("gamestrings_fr.txt");
        break;
    case SCE_ITALIAN_LANGUAGE:
        path += HxStr("gamestrings_ita.txt");
        break;
    case SCE_SPANISH_LANGUAGE:
        path += HxStr("gamestrings_sp.txt");
        break;
    case SCE_GERMAN_LANGUAGE:
        path += HxStr("gamestrings_ger.txt");
        break;
    default:
        path += HxStr("gamestrings_en.txt");
        break;
    }
    HxStr text("");
    unsigned nState = 0;
    int nId = 0;
    int nBase = 0;
    FILE *pFile = fopen(path.mStr != nullptr ? path.mStr : g_szEmptyString, "r");
    char line[kLineSize];
    // Yes, the binary never closes the file.
    while (fgets(line, kLineSize, pFile) != nullptr) {
        for (char *p = line; *p != '\n'; ++p) {
            if (nState == 0) {
                if (p[0] == '/' && p[1] == '/') {
                    break;
                }
                if (*p == '{') {
                    nState |= kStateInSection;
                } else if (*p == '"') {
                    nState |= kStateInValue;
                } else if (*p != ' ' && *p != '\t' && *p != '\r') {
                    nState |= kStateInKey;
                }
                continue;
            }
            if (*p == '}') {
                nState &= ~kStateInSection;
                while (nId >= nBase) {
                    nBase += kSectionStride;
                }
                nId = nBase;
                continue;
            }
            if (*p == '"' && p != line && p[-1] != '\\') {
                nState &= ~kStateInValue;
                g_metStrings.Add(nId, text);
                ++nId;
                continue;
            }
            if (*p == ',' && nState == kStateInKey) {
                // The key's characters are discarded here, and the value starts empty.
                text = "";
                nState = 0;
                continue;
            }
            if (*p == '\\') {
                switch (p[1]) {
                case '\\':
                    text += '\\';
                    break;
                case 'n':
                    text += '\n';
                    break;
                case '\'':
                    text += '\'';
                    break;
                case '"':
                    text += '"';
                    break;
                default:
                    break;
                }
                ++p;
                continue;
            }
            text += *p;
        }
    }
}

HxStr GetMetString(int nId) {
    return g_metStrings.Find(nId);
}
#endif
