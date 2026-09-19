#include "math/plane.h"

Plane TransformPlaneToWorld(const Plane &plane, const float *pXfm) {
    Plane result;
    result.a = (pXfm[0] * plane.a) + (pXfm[4] * plane.b) + (pXfm[8] * plane.c);
    result.b = (pXfm[1] * plane.a) + (pXfm[5] * plane.b) + (pXfm[9] * plane.c);
    result.c = (pXfm[2] * plane.a) + (pXfm[6] * plane.b) + (pXfm[10] * plane.c);
    result.d = plane.d - ((pXfm[12] * result.a) + (pXfm[13] * result.b) + (pXfm[14] * result.c));
    return result;
}
