#include "rndartt/arect.h"

// 0x00613b58
ARect ARect::Intersection(const ARect &other) const {
    ARect result;
    result.mLeft = other.mLeft < mLeft ? mLeft : other.mLeft;
    result.mTop = other.mTop < mTop ? mTop : other.mTop;
    result.mRight = mRight < other.mRight ? mRight : other.mRight;
    result.mBottom = mBottom < other.mBottom ? mBottom : other.mBottom;
    return result;
}
