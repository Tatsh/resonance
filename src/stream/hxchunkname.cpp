#include "stream/hxchunkname.h"

#include "stream/hxstream.h"

// Defined in address order. The static initialiser at 0x00145608 builds all fifteen.
HxChunkName g_listChunkName("LIST");
HxChunkName g_riffChunkName("RIFF");
HxChunkName g_midiChunkName("MIDI");
HxChunkName g_mthdChunkName("MThd");
HxChunkName g_mtrkChunkName("MTrk");
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

// 0x00146550
HxStream &operator>>(HxStream &stream, HxChunkName &name) {
    stream.Read(name.mText, HxChunkName::kLength);
    return stream;
}
