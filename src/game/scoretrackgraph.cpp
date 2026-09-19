#include "game/scoretrackgraph.h"

#include "app/msgsource.h"

// 0x001cf750
void ScoreTrackGraph::Slot5(MsgSource *) {
}

// 0x001cf758
int ScoreTrackGraph::Slot9() {
    return 1;
}

// 0x001cf760. The compiled body is two instructions and writes no return register, so this default
// produces an indeterminate value and is not meant to be called through. The slot type is int
// rather than void, which the sibling override that forwards both arguments and returns a result
// establishes, so the empty body is a property of the original and not a missing return here.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
int ScoreTrackGraph::Slot10(int, int) {
}
#pragma GCC diagnostic pop

// 0x001cf768
int ScoreTrackGraph::Slot11() {
    return 0;
}

// 0x001cf778
void ScoreTrackGraph::Slot12() {
}
