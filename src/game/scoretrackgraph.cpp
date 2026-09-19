#include "game/scoretrackgraph.h"

#include "app/msgsource.h"

// 0x001cf750
void ScoreTrackGraph::Slot5(MsgSource *) {
}

// 0x001cf758
int ScoreTrackGraph::Slot9() {
    return 1;
}

// 0x001cf760
int ScoreTrackGraph::Slot10(int, int) {
    // The compiled body writes no return register, so the default yields an indeterminate value
    // and is not meant to be called.
}

// 0x001cf768
int ScoreTrackGraph::Slot11() {
    return 0;
}

// 0x001cf778
void ScoreTrackGraph::Slot12() {
}
