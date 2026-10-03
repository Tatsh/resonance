#include "synth/synth.h"

#include "app/application.h"
#include "app/timeclock.h"
#include "app/timetask.h"
#include "msg/stdmidimsg.h"
#include "sch/tickclock.h"
#include "synth/source.h"

namespace {

constexpr int kChannelCount = 16;
constexpr int kSfxChannel = 15;
constexpr unsigned char kStatusControlChange = 0xb0;
constexpr unsigned char kStatusPitchBend = 0xe0;
constexpr unsigned char kControllerChannelVolume = 7;
constexpr unsigned char kControllerExpression = 11;
constexpr unsigned char kControllerResetAll = 121;
constexpr unsigned char kControllerAllNotesOff = 123;

// The values Setup() leaves on every channel.
constexpr unsigned char kPitchBendCentre = 64;
constexpr unsigned char kSetupVolume = 100;
constexpr unsigned char kSetupExpression = 127;

// SynthFade runs every 100 milliseconds and scales the fade's level to a MIDI volume.
constexpr long long kFadePeriodNs = 100000000;
constexpr long long kNsPerMs = 1000000;
constexpr long long kNsRounding = kNsPerMs / 2;
constexpr float kFadeVolumeScale = 100.0f;
// The fade stops its Source once the duration has passed.
constexpr int kFadeStopsAtEnd = 1;
// FadeOut() starts the task with its epoch at the current time.
constexpr long long kFadeEpochNow = 0;

/**
 * The synthesiser that discards every message, for a build with no sound hardware.
 *
 * File-local to Synth::Setup() in the RTTI, with Synth as its one base. The vtable at
 * `0x007d2e50` fills the pure SendMidi() with an empty body and inherits every other slot. The
 * object is Synth's four bytes.
 */
class NullSynth : public Synth {
public:
    /**
     * Build the synthesiser on the heap. Nothing in the image calls it.
     *
     * @return The new synthesiser.
     * @ghidraAddress NTSC-U/C: 0x0013a0c0
     * @ghidraAddress PAL: 0x0013aa08
     */
    // NTSC-U/C: 0x0013a0c0, PAL: 0x0013aa08
    static Synth *New() {
        return new NullSynth;
    }

    /**
     * Discard one MIDI message.
     *
     * @ghidraAddress NTSC-U/C: 0x0013a478
     * @ghidraAddress PAL: 0x0013adc0
     */
    // NTSC-U/C: 0x0013a478, PAL: 0x0013adc0
    virtual void SendMidi(unsigned char, unsigned char, unsigned char) {
    }
};

/**
 * Task that fades every channel's volume to silence.
 *
 * File-local to Synth::Setup() in the RTTI, with TimeTask as its one base. The vtable
 * is at `0x007d3000`, and the object is 0x38 bytes. Synth::FadeOut() is the one builder.
 */
class SynthFade : public TimeTask {
public:
    /**
     * @param pSynth The synthesiser to fade.
     * @param nDurationMs The length of the fade, in milliseconds.
     */
    SynthFade(Synth *pSynth, int nDurationMs)
        // Globals' watchdog time base is the clock this task posts against.
        : TimeTask(Application::shared()->GetWatchdogTimer(), kFadePeriodNs),
          mSource(Source::AllocateFadeOutSource(kFadeStopsAtEnd, static_cast<float>(nDurationMs))),
          mSynth(pSynth) {
    }

    /**
     * Delete the Source.
     *
     * @ghidraAddress NTSC-U/C: 0x0013a6a8
     * @ghidraAddress PAL: 0x0013aff0
     */
    // NTSC-U/C: 0x0013a6a8, PAL: 0x0013aff0
    virtual ~SynthFade() {
        delete mSource;
    }

    /**
     * Send the fade's level as the volume of every channel, and silence every note at zero.
     *
     * @param nElapsedNs Nanoseconds since the fade started.
     * @return What the Source reports, so the task stops once the fade has finished.
     * @ghidraAddress NTSC-U/C: 0x0013a710
     * @ghidraAddress PAL: 0x0013b058
     */
    // NTSC-U/C: 0x0013a710, PAL: 0x0013b058
    virtual int Tick(long long nElapsedNs) {
        const int nElapsedMs = static_cast<int>((nElapsedNs + kNsRounding) / kNsPerMs);
        float flLevel;
        const int bContinue = mSource->Sample(static_cast<float>(nElapsedMs), &flLevel);
        const int nVolume = static_cast<int>(flLevel * kFadeVolumeScale);
        for (unsigned char nChannel = 0; nChannel < kChannelCount; ++nChannel) {
            mSynth->SendMidi(kStatusControlChange | nChannel,
                             kControllerChannelVolume,
                             static_cast<unsigned char>(nVolume));
        }
        if (nVolume == 0) {
            for (int nChannel = 0; nChannel < kChannelCount; ++nChannel) {
                mSynth->SendMidi(kStatusControlChange | nChannel, kControllerAllNotesOff, 0);
            }
        }
        return bContinue;
    }

private:
    Source *mSource; // +0x30
    Synth *mSynth;   // +0x34
};

} // namespace

// NTSC-U/C: 0x00139fd0, PAL: 0x0013a918
void Synth::Setup() {
    for (unsigned char nChannel = 0; nChannel < kChannelCount; ++nChannel) {
        SendMidi(kStatusPitchBend | nChannel, 0, kPitchBendCentre);
        SendMidi(kStatusControlChange | nChannel, kControllerResetAll, 0);
        SendMidi(kStatusControlChange | nChannel, kControllerAllNotesOff, 0);
        SendMidi(kStatusControlChange | nChannel, kControllerChannelVolume, kSetupVolume);
        SendMidi(kStatusControlChange | nChannel, kControllerExpression, kSetupExpression);
    }
}

// NTSC-U/C: 0x0013a480, PAL: 0x0013adc8
void Synth::FadeOut(int nDurationMs) {
    SynthFade *pFade = new SynthFade(this, nDurationMs);
    pFade->Start(kFadeEpochNow);
    if (pFade != nullptr) {
        pFade->Release();
    }
}

// NTSC-U/C: 0x0013a398, PAL: 0x0013ace0
Synth::~Synth() {
}

// NTSC-U/C: 0x0013a1d0, PAL: 0x0013ab18
void Synth::LoadBankSet4() {
}

// NTSC-U/C: 0x0013a1d8, PAL: 0x0013ab20
void Synth::LoadBankSet5() {
}

// NTSC-U/C: 0x0013a1e0, PAL: 0x0013ab28
void Synth::LoadBankSet6() {
}

// NTSC-U/C: 0x0013a1e8, PAL: 0x0013ab30
void Synth::UnusedHook() {
}

// NTSC-U/C: 0x0013a1f0, PAL: 0x0013ab38
void Synth::UnloadBanks() {
}

// NTSC-U/C: 0x0013a1f8, PAL: 0x0013ab40
void Synth::OnPlayStarted() {
}

// NTSC-U/C: 0x0013a200, PAL: 0x0013ab48
void Synth::SelectBank([[maybe_unused]] unsigned char nChannel,
                       [[maybe_unused]] unsigned char nBank) {
}

// NTSC-U/C: 0x0013a208, PAL: 0x0013ab50
void Synth::SetStereo([[maybe_unused]] int bStereo) {
}

// NTSC-U/C: 0x0013a210, PAL: 0x0013ab58
void Synth::SetRemixMode([[maybe_unused]] int bRemix) {
}

// NTSC-U/C: 0x0013a218, PAL: 0x0013ab60
void Synth::SetPaused([[maybe_unused]] int bPaused) {
}

// NTSC-U/C: 0x0013a220, PAL: 0x0013ab68
void Synth::AllNotesOff() {
    for (unsigned char nChannel = 0; nChannel < kChannelCount; ++nChannel) {
        SendMidi(kStatusControlChange | nChannel, kControllerAllNotesOff, 0);
    }
}

// NTSC-U/C: 0x0013a288, PAL: 0x0013abd0
void Synth::AllNotesOffExceptSfxChannel() {
    for (unsigned char nChannel = 0; nChannel < kSfxChannel; ++nChannel) {
        SendMidi(kStatusControlChange | nChannel, kControllerAllNotesOff, 0);
    }
}

// NTSC-U/C: 0x0013a2f0, PAL: 0x0013ac38
void Synth::SetChannelVolume(unsigned char nVolume) {
    for (unsigned char nChannel = 0; nChannel < kChannelCount; ++nChannel) {
        SendMidi(kStatusControlChange | nChannel, kControllerChannelVolume, nVolume);
    }
}

// The address below is the out-of-line copy.
// NTSC-U/C: 0x0013a360, PAL: 0x0013aca8
inline void Synth::OnStdMidi(StdMidiMsg *pMsg) {
    SendMidi(pMsg->mStatus, pMsg->mData1, pMsg->mData2);
}

// NTSC-U/C: 0x0013a570, PAL: 0x0013aeb8
void Synth::HandleMessage(Message *pMsg) {
    if (pMsg->Type() != static_cast<int>(g_dwStdMidiMsgType)) {
        return;
    }

    OnStdMidi(static_cast<StdMidiMsg *>(pMsg));
}
