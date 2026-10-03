#include "game/axephrasemaker.h"

#include <algorithm>
#include <cstring>

#include "app/application.h"
#include "app/attachment.h"
#include "app/playsound.h"
#include "game/nullplayer.h"
#include "mid/mbt.h"
#include "msg/axisregistermsg.h"
#include "msg/barstatusmsg.h"
#include "msg/beginphrasecatchmsg.h"
#include "msg/cleargemsmsg.h"
#include "msg/notemsg.h"
#include "msg/phrasecapturedmsg.h"
#include "msg/seekermsg.h"
#include "msg/showeraseeffectmsg.h"
#include "script/configquery.h"
#include "synth/ps2hardsynth.h"

namespace {

// The origin every guitar phrase maker reports, in MIDI ticks.
constexpr int kPeriodOrigin = 6;

// One bar at 480 ticks per quarter note.
constexpr int kBarTicks = 1920;

// The bar mPhraseBar holds before the first phrase.
constexpr int kNoBar = -1;

// The axis value the constructor assumes, the middle of the range.
constexpr float kAxisCenter = 0.5f;

// The two configuration codes that decide mSwitchBanks.
constexpr int kBankSwitchConfigCode = 932;
constexpr int kBankSwitchOverrideConfigCode = 929;

constexpr unsigned char kStatusKindMask = 0xf0;
constexpr unsigned char kStatusNoteOff = 0x80;
constexpr unsigned char kStatusNoteOn = 0x90;
constexpr unsigned char kChannelMask = 0xf;

// A held note still sounding when its bar ends lasts until this many ticks after the bar.
constexpr int kHeldNoteOverhang = 1;

// The ShowEraseEffectMsg::mEffectFlag value that Erase() always sets.
constexpr int kEraseEffectFlag = 1;

constexpr char kEraseStepSound[] = "SND_ERASE_SECTION";
constexpr char kEraseBarSound[] = "SND_ERASE";

// A computed position, clamped to the finite range as the inline Mid::MBT arithmetic does.
inline Mid::MBT MakePosition(int nTick) {
    return Mid::MBT(std::min(std::max(nTick, kMBTMinimum), kMBTMaximum));
}

} // namespace

// NTSC-U/C: 0x0019b5d0, PAL: 0x001a1338
AxePhraseMaker::AxePhraseMaker(PhraseMgr *pPhraseMgr,
                               Quantizer *pQuantizer,
                               const TrackData *pTrackData,
                               Sch::TickClock *)
    : mPhraseMgr(pPhraseMgr), mQuantizer(pQuantizer), mTrack(pTrackData->mIndex),
      mChannel(pTrackData->mChannel), mPhrase(nullptr), mPhraseBar(kNoBar), mPlayer(&g_nullPlayer),
      mBarTicks(kBarTicks), mTrackData(pTrackData), mValue(kAxisCenter) {
    mSwitchBanks = 0;
    if (QueryConfigFlag(kBankSwitchConfigCode) != 0) {
        mSwitchBanks = QueryConfigFlag(kBankSwitchOverrideConfigCode) == 0;
    }
}

// NTSC-U/C: 0x0019b958, PAL: 0x001a16c0
void AxePhraseMaker::OnStdMidi(StdMidiMsg *pMsg) {
    const int nTick = pMsg->mTick;
    const unsigned char nKind = pMsg->mStatus & kStatusKindMask;

    if (nKind == kStatusNoteOff) {
        const unsigned char nNote = pMsg->mData1;
        for (auto it = mHeldNotes.begin(); it != mHeldNotes.end(); ++it) {
            if (it->mNote != nNote) {
                continue;
            }
            const Mid::MBT start(it->mTick);
            NoteMsg note(start.mTick,
                         mChannel,
                         nNote,
                         it->mVelocity,
                         MakePosition(nTick - Mid::MBT(it->mTick).mTick));
            RecordMuseMsg(&note);
            mHeldNotes.erase(it);
            return;
        }
        return;
    }

    StartPhrase(nTick);
    if (nKind != kStatusNoteOn) {
        RecordMuseMsg(pMsg);
        return;
    }

    HeldNote held;
    std::memset(&held, 0, sizeof(held));
    held.mNote = pMsg->mData1;
    held.mVelocity = pMsg->mData2;
    held.mTick = nTick;
    mHeldNotes.push_back(held);
    mChannel = pMsg->mStatus & kChannelMask;
}

// NTSC-U/C: 0x0019bbd8, PAL: 0x001a1940
void AxePhraseMaker::RecordMuseMsg(MuseMsg *pMsg) {
    const Mid::MBT barStart = MakePosition(mBarTicks.mTick * mPhraseBar);
    const Mid::MBT offset = MakePosition(pMsg->mTick - barStart.mTick);
    (void)Mid::MBT(0); // Yes, the binary discards this position.
    mPhrase->AddMuseMsg(offset.mTick, pMsg);
    mPhrase->AddValue(offset.mTick, mValue);
}

// NTSC-U/C: 0x0019bce0, PAL: 0x001a1a48
void AxePhraseMaker::StartPhrase(int nTick) {
    const int nBar = nTick / mBarTicks.mTick;
    if (mPhraseBar == nBar && mPhrase != nullptr) {
        return;
    }

    FinishPhrase();
    mPhraseBar = nBar;
    mPhrase = new Phrase();
    mPhrase->mPlayer = mPlayer;
    mPhrase->AddValue(Mid::MBT(0).mTick, mValue);

    ClearGemsMsg clear;
    clear.mBar = nBar;
    clear.mTrack = mTrack;
    Send(&clear);

    BarStatusMsg status(nBar, mTrack, mPlayer);
    Send(&status);

    int nPoints = mTrackData->GetPoints(nBar);
    if (mPlayer->MarkBarScored(nBar) == 0) {
        nPoints = 0;
    }
    BeginPhraseCatchMsg begin(mPlayer, nPoints, mPlayer->GetMultiplier(nBar));
    Send(&begin);

    PhraseCapturedMsg captured(
        mPhraseBar, mPhraseBar + 1, mPhraseBar, mPhraseBar + 1, mTrack, mPlayer, nPoints, 0, 0);
    Send(&captured);
}

// NTSC-U/C: 0x0019bf80, PAL: 0x001a1ce8
void AxePhraseMaker::Erase(Player *pPlayer, int nTick, int bWholeStep) {
    const int nBar = nTick / mBarTicks.mTick;
    int bErased = mPhrase != nullptr;

    int nFirstBar;
    int nEndBar;
    if (bWholeStep != 0) {
        nFirstBar = mTrackData->StepStartBar(nBar);
        nEndBar = mTrackData->FollowingStepBar(nFirstBar);
    } else {
        nFirstBar = nBar;
        nEndBar = nBar + 1;
    }

    for (int nClear = nFirstBar; nClear < nEndBar; ++nClear) {
        if (mPhraseMgr->GetPhraseOwner(nClear) == pPlayer) {
            bErased = 1;
            mPhraseMgr->ClearPhrase(nClear, 0);
        }
    }

    if (bErased != 0) {
        PlaySoundByName(bWholeStep != 0 ? kEraseStepSound : kEraseBarSound);
        // The effect names mPlayer, not the player the erase was for.
        ShowEraseEffectMsg effect(mPlayer, mTrack, nFirstBar, nEndBar, kEraseEffectFlag);
        Send(&effect);
    }

    Attachment::ReleaseIfSet(mPhrase);
    mPhrase = nullptr;
}

// NTSC-U/C: 0x0019c118, PAL: 0x001a1e80
void AxePhraseMaker::FinishPhrase() {
    if (mPhrase == nullptr) {
        return;
    }

    const Mid::MBT barEnd = MakePosition(mBarTicks.mTick * (mPhraseBar + 1));
    const Mid::MBT end = MakePosition(barEnd.mTick + Mid::MBT(kHeldNoteOverhang).mTick);
    for (const auto &held : mHeldNotes) {
        const Mid::MBT start(held.mTick);
        NoteMsg note(start.mTick,
                     mChannel,
                     held.mNote,
                     held.mVelocity,
                     MakePosition(end.mTick - Mid::MBT(held.mTick).mTick));
        RecordMuseMsg(&note);
    }
    mHeldNotes.clear();

    mPhraseMgr->InstallPhrase(mPhrase, mPhraseBar, 0);
    Attachment::ReleaseIfSet(mPhrase);
    mPhrase = nullptr;
}

// NTSC-U/C: 0x0019c368, PAL: 0x001a20d0
void AxePhraseMaker::PostSeekerMsg(int) {
    if (mPlayer->IsNull()) {
        return;
    }
    SeekerMsg msg(mPlayer);
    Send(&msg);
}

// NTSC-U/C: 0x0019c408, PAL: 0x001a2170
void AxePhraseMaker::HandleMessage(Message *pMsg) {
    const int nType = pMsg->Type();
    if (nType == g_nAxisRegisterMsgType) {
        AxisRegisterMsg *pAxis = static_cast<AxisRegisterMsg *>(pMsg);
        if (pAxis->mTrack == mTrack && pAxis->mPlayer == mPlayer) {
            mValue = pAxis->mValue;
        }
    } else if (nType == static_cast<int>(g_dwStdMidiMsgType)) {
        OnStdMidi(static_cast<StdMidiMsg *>(pMsg));
    } else if (nType == static_cast<int>(g_dwSustainNoteMsgType)) {
        OnSustainNote(static_cast<SustainNoteMsg *>(pMsg));
    } else if (nType == static_cast<int>(g_dwTrackSelectMsgType)) {
        OnTrackSelect(static_cast<TrackSelectMsg *>(pMsg));
    } else if (nType == g_nInvalidateSeekerMsgType) {
        OnMsg(*static_cast<InvalidateSeekerMsg *>(pMsg));
    }
}

// NTSC-U/C: 0x0019d438, PAL: 0x001a31a0
int AxePhraseMaker::GetPeriodOrigin() {
    return Mid::MBT(kPeriodOrigin).mTick;
}

// NTSC-U/C: 0x0019d860, PAL: 0x001a35c8
void AxePhraseMaker::OnTrackSelect(TrackSelectMsg *pMsg) {
    if (pMsg->mTrack != mTrack || pMsg->mPlace != 0) {
        return;
    }
    mPlayer = pMsg->mPlayer;
    if (mPlayer->IsNull()) {
        return;
    }
    PostSeekerMsg(pMsg->mPosition.mTick / mBarTicks.mTick);
}

// NTSC-U/C: 0x0019d8f0, PAL: 0x001a3658
void AxePhraseMaker::OnMsg(const InvalidateSeekerMsg &msg) {
    if (msg.mTrack == mTrack) {
        PostSeekerMsg(msg.mBar);
    }
}

// NTSC-U/C: 0x0019d920, PAL: 0x001a3688
void AxePhraseMaker::OnSustainNote(SustainNoteMsg *pMsg) {
    StartPhrase(pMsg->mTick);
    RecordMuseMsg(pMsg);
}

// NTSC-U/C: 0x0019d990, PAL: 0x001a36f8
void AxePhraseMaker::OnPeriod(int nBar) {
    if ((nBar - 1) == mPhraseBar) {
        FinishPhrase();
    }
    PostSeekerMsg(nBar);
    if (mSwitchBanks == 0) {
        return;
    }
    if (mTrackData->IsStepStart(nBar) == 0) {
        return;
    }
    Application::shared()->GetSynth()->SelectBank(mChannel, mTrackData->FindStepIndex(nBar));
}

// NTSC-U/C: 0x0019da58, PAL: 0x001a37c0
int AxePhraseMaker::IsBarPlayable(int nBar) {
    int bPlayable = 0;
    if (mTrackData->QueryBar(nBar) != 0) {
        bPlayable = mPlayer->IsFreestyleBar(nBar) != 0;
    }
    return bPlayable;
}
