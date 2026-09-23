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
void ScoreTrackGraph::Slot10(int, Player *) {
}

// 0x001cf768
int ScoreTrackGraph::Slot11() {
    return 0;
}

// 0x001cf778
void ScoreTrackGraph::Slot12() {
}

// 0x001cf978
PhraseDatabase *ScoreTrackGraph::GetPhraseDatabase() {
    return mPhraseMgr->mDatabase;
}
