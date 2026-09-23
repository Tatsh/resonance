#include "game/playmaplinear.h"
#include "script/cxx/int.h"
#include "script/cxx/seqbase.h"
#include "script/scripteval.h"

namespace {

// The script template that yields a table's step rings, and the step value before the first.
constexpr int kStepRingTemplate = 0x3a3;
constexpr int kNoStep = -1;

} // namespace

// 0x00128410
void PlayMapLinear::LoadStepRings() {
    for (int nSet = 0; nSet < kSetCount; ++nSet) {
        Py::Sequence rings(EvalScriptTemplate(kStepRingTemplate, nSet));
        for (Py::Sequence::iterator it = rings.begin(); it != rings.end(); ++it) {
            Py::Sequence ring(*it);
            int nFirst = kNoStep;
            int nPrevious = kNoStep;
            for (Py::Sequence::iterator step = ring.begin(); step != ring.end(); ++step) {
                Py::Int value(*step);
                const int nStep = static_cast<long>(value);
                if (nPrevious != kNoStep) {
                    mUnknown68[nSet].push_back(StepPair{nPrevious, nStep});
                } else {
                    nFirst = nStep;
                }
                nPrevious = nStep;
            }
            mUnknown68[nSet].push_back(StepPair{nPrevious, nFirst});
        }
    }
}
