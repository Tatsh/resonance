#include "stream/hxchunkname.h"

#include "stream/hxstream.h"

// Defined in address order. The static initialiser at 0x00145608 builds all fifteen.
HxChunkName kListChunkID("LIST");
HxChunkName kRiffChunkID("RIFF");
HxChunkName g_midiChunkName("MIDI");
HxChunkName kMidiHeaderChunkID("MThd");
HxChunkName kMidiTrackChunkID("MTrk");
HxChunkName g_waveChunkName("WAVE");
HxChunkName g_fmtChunkName("fmt ");
HxChunkName g_dataChunkName("data");
HxChunkName g_factChunkName("fact");
HxChunkName g_instChunkName("inst");
HxChunkName g_smplChunkName("smpl");
HxChunkName g_cueChunkName("cue ");
HxChunkName g_lablChunkName("labl");
HxChunkName g_ltxtChunkName("ltxt");
HxChunkName g_adtlChunkName("adtl");

HxStream &operator>>(HxStream &stream, HxChunkName &name) {
    stream.ReadData(name.mText, HxChunkName::kLength);
    return stream;
}
