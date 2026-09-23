#include "game/harmony.h"

#include <iostream>
#include <vector>

// 0x001a4ee8
void Harmony::Print(std::ostream &stream) {
    stream << "(";
    for (std::vector<char>::iterator it = mNotes.begin(); it != mNotes.end(); ++it) {
        stream << *it << " ";
    }
    stream << ")";
}
