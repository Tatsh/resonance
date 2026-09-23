#include "app/cutscene.h"

#include <ee_regs.h>
#include <eekernel.h>
#include <ezmpeg.h>
#include <libdma.h>
#include <libgraph.h>
#include <libmpeg.h>
#include <libpad.h>
#include <libsdr.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "os/log.h"
#include "os/mem.h"

// The binary passes __FILE__ and __LINE__ to the tagged allocator. The literal was "cutscene.c".
static const char kSourceFile[] = "cutscene.c";
enum { kAllocLine = 134, kFreeLine = 167 };

// The one block every decoder buffer is carved from. Each buffer starts on a 64-byte boundary, and
// the block reserves 64 bytes of slack for aligning the first.
enum {
    kBufferAlign = 64,
    kMpegWorkSize = 0x17c300,
    kAudioBufferSize = 0xc000,
    kAudioIopBufferSize = 0x6000,
    kVideoDataSize = 0x80000,
    kFrameImagesSize = 0x2a3000,
    kFrameTagsSize = 0x80880,
    kCutsceneMemorySize = 0x57bc00,
};

// Decoded frames the queue holds, tags and time stamps the decoder records, and the two thread
// stacks.
enum {
    kFrameCount = 2,
    kVideoTagCount = 0x100,
    kTimeStampCount = 0x200,
    kTimeStampSize = 0x18,
    kDefaultStackSize = 0x800,
    kDecodeStackSize = 0x4000,
};

// The display the movie is shown on, and its double buffer, which is half height.
enum {
    kDisplayWidth = 640,
    kDisplayHeight = 480,
    kGsPsmCt32 = 0,
    kGsInterlace = 1,
    kGsNtsc = 2,
    kGsFrameMode = 1,
    kGsResetAll = 0,
    kClearEnabled = 1,
};

// Reading and demultiplexing. The loop stops once fewer than five bytes remain.
enum {
    kReadChunk = 0x10000,
    kMinimumRestSize = 5,
    kPadReportSize = 32,
    kPadButtonCount = 16,
    kPadButtonCross = 0x40,
    kPadButtonStart = 0x800,
    kPadButtonsMask = 0xffff,
    kSkipMinimumFrames = 11,
};

// The values the binary writes to D_CTRL and D_STAT, and the arguments of the controller and sound
// resets. The frame images are handed to the queue through the uncached segment.
enum {
    kDmaCtrlEnable = 3,
    kDmaStatClearVif1 = 4,
    kDmaResetEnable = 1,
    kSdInitCold = 0,
    kFlushCacheWriteBackData = 0,
    kPhysicalAddressMask = 0x0fffffff,
    kUncachedSegment = 0x20000000,
};
#define UNCACHED(pointer)                                                                          \
    ((void *)(((uintptr_t)(pointer) & kPhysicalAddressMask) | kUncachedSegment))
#define ALIGN_UP(pointer)                                                                          \
    ((void *)(((uintptr_t)(pointer) + kBufferAlign - 1) & ~(uintptr_t)(kBufferAlign - 1)))

// 0x0070cae0
static int g_is_with_audio;

// 0x0070cae8
VoBuf voBuf;

// 0x0070cb00
sceGsDBuff db;

// 0x0070cd30
static ReadBuf *g_read_buf;

// 0x0070cd34
static int g_decode_thread;

// 0x0070cd38
static int g_default_thread;

// 0x0070cd40
static StrFile g_in_file;

// 0x0070cd78
VideoDec videoDec;

// 0x0070ce30
AudioDec audioDec;

// 0x0070ce90
// The pad buttons read during the last pass of the playback loop, active high.
static int g_pad_buttons;

// 0x00895400
static int g_default_priority;

// 0x00895404
static void *g_frame_images;

// 0x00895408
static void *g_frame_tags;

// 0x00895440
static unsigned long long g_video_tags[kVideoTagCount][2];

// 0x00896450
static unsigned char *g_mpeg_work;

// 0x00896454
static unsigned char *g_audio_buffer;

// 0x00896458
static void *g_video_data;

// 0x00896480
static unsigned char g_default_stack[kDefaultStackSize] __attribute__((aligned(16)));

// 0x00896c80
static unsigned char g_decode_stack[kDecodeStackSize] __attribute__((aligned(16)));

// 0x0089ac80
// The next global starts 0x100 bytes beyond the time stamps. That bounds each at 0x18 bytes.
static unsigned char g_time_stamps[kTimeStampCount][kTimeStampSize];

// 0x0089dd80
static void *g_cutscene_memory;

// 0x00510f20
void ErrMessage(char *message) {
    LogPrintf("[ Error ] %s\n", message);
}

// 0x00510f48
void switchThread(void) {
    RotateThreadReadyQueue(g_default_priority);
}

// 0x00510f70
static void proceed_audio(void) {
    audioDecSendToIOP(&audioDec);
}

// 0x00511100
static int is_audio_ok(void) {
    if (g_is_with_audio == 0) {
        return 1;
    }
    return audioDecIsPreset(&audioDec);
}

// 0x005110e0
static void default_main(void *argument) {
    (void)argument;
    for (;;) {
        switchThread();
    }
}

// 0x00511088
static void reset_display(void) {
    clearGsMem(0, 0, 0, kDisplayWidth, kDisplayHeight);
    sceGsSetDefDBuff(&db, kGsPsmCt32, kDisplayWidth, kDisplayHeight / 2, 0, 0, kClearEnabled);
    FlushCache(kFlushCacheWriteBackData);
}

// 0x005107a8
static void init_all(void) {
    g_cutscene_memory = MemAllocTagged(kCutsceneMemorySize, kSourceFile, kAllocLine);
    g_mpeg_work = ALIGN_UP(g_cutscene_memory);
    g_audio_buffer = ALIGN_UP(g_mpeg_work + kMpegWorkSize);
    g_video_data = ALIGN_UP(g_audio_buffer + kAudioBufferSize);
    g_frame_images = ALIGN_UP((unsigned char *)g_video_data + kVideoDataSize);
    g_frame_tags = ALIGN_UP((unsigned char *)g_frame_images + kFrameImagesSize);
    g_read_buf = ALIGN_UP((unsigned char *)g_frame_tags + kFrameTagsSize);

    sceGsResetPath();
    sceDmaReset(kDmaResetEnable);
    sceGsSyncPath(0, 0);
    sceGsSyncV(0);
    sceGsResetGraph(kGsResetAll, kGsInterlace, kGsNtsc, kGsFrameMode);
    reset_display();
}

// 0x00510b98
static void prepare_playback(const char *name) {
    *R_EE_D_CTRL |= kDmaCtrlEnable;
    *R_EE_D_STAT = kDmaStatClearVif1;

    readBufCreate(g_read_buf);
    sceMpegInit();
    videoDecCreate(&videoDec,
                   g_mpeg_work,
                   kMpegWorkSize,
                   g_video_data,
                   g_video_tags,
                   kVideoTagCount,
                   g_time_stamps,
                   kTimeStampCount);
    sceSdRemoteInit();
    sceSdRemote(1, rSdInit, kSdInitCold);
    audioDecCreate(&audioDec, g_audio_buffer, kAudioBufferSize, kAudioIopBufferSize);
    videoDecSetStream(&videoDec, sceMpegStrM2V, 0, videoCallback, g_read_buf);
    if (g_is_with_audio != 0) {
        videoDecSetStream(&videoDec, sceMpegStrPCM, 0, pcmCallback, g_read_buf);
    }
    voBufCreate(&voBuf, UNCACHED(g_frame_images), g_frame_tags, kFrameCount);

    // Both threads run at the priority of the calling thread.
    ee_thread_status_t status;
    ReferThreadStatus(GetThreadId(), &status);
    ee_thread_t thread;
    thread.func = (void *)default_main;
    thread.stack = g_default_stack;
    thread.stack_size = kDefaultStackSize;
    thread.initial_priority = status.current_priority;
    thread.gp_reg = &_gp;
    thread.option = 0;
    g_default_priority = status.current_priority;
    g_default_thread = CreateThread(&thread);
    StartThread(g_default_thread, NULL);

    thread.func = (void *)videoDecMain;
    thread.stack = g_decode_stack;
    thread.stack_size = kDecodeStackSize;
    thread.initial_priority = g_default_priority;
    thread.gp_reg = &_gp;
    thread.option = 0;
    g_decode_thread = CreateThread(&thread);
    StartThread(g_decode_thread, &videoDec);

    while (strFileOpen(&g_in_file, name) == 0) {
        LogPrintf("Can't Open file %s\n", name);
    }

    videoDec.hid_endimage = AddDmacHandler(DMAC_GIF, handler_endimage, 0);
    EnableDmac(DMAC_GIF);
    videoDec.hid_vblank = AddIntcHandler(INTC_VBLANK_S, vblankHandler, 0);
    EnableIntc(INTC_VBLANK_S);
}

// Read the pad and report whether Cross or Start has been pressed and released. A button counts
// only once it has been seen up, then down, then up again. play_mpeg() expands this in its loop.
static inline int poll_skip(int *released, int *pressed) {
    unsigned char report[kPadReportSize];
    int skip = 0;
    int i;
    if (scePadRead(0, 0, report) > 0) {
        g_pad_buttons = kPadButtonsMask ^ ((report[2] << 8) | report[3]);
    } else {
        g_pad_buttons = 0;
    }
    for (i = 0; i < kPadButtonCount; ++i) {
        const int bit = 1 << i;
        const int down = g_pad_buttons & bit;
        if (down == 0) {
            *released |= bit;
        }
        if ((*released & bit) != 0 && down != 0) {
            *pressed |= bit;
        }
        if ((*pressed & bit) != 0 && down == 0 &&
            (bit == kPadButtonCross || bit == kPadButtonStart)) {
            skip = 1;
        }
    }
    return skip;
}

// 0x005108b0
static int play_mpeg(VideoDec *video_dec, ReadBuf *buffer, StrFile *file) {
    int released = 0;
    int pressed = 0;
    int rest_to_demux = file->size;
    int rest_to_read = rest_to_demux;
    int is_started = 0;
    int skip = 0;
    unsigned char *put;
    unsigned char *get;
    int size;

    while (rest_to_demux >= kMinimumRestSize && videoDecGetState(video_dec) != VD_STATE_END) {
        // Once set, the skip request stands for the rest of the playback.
        if (poll_skip(&released, &pressed) != 0) {
            skip = 1;
        }
        if (skip != 0 && video_dec->mpeg.frameCount >= kSkipMinimumFrames) {
            videoDecAbort(&videoDec); // Yes, the binary aborts the global rather than video_dec.
        }

        size = readBufBeginPut(buffer, &put);
        if (rest_to_read > 0 && size >= kReadChunk) {
            const int read = strFileRead(file, put, kReadChunk);
            readBufEndPut(buffer, read);
            rest_to_read -= read;
        }

        switchThread();

        size = readBufBeginGet(buffer, &get);
        if (size > 0) {
            const int consumed =
                sceMpegDemuxPssRing(&video_dec->mpeg, get, size, buffer->data, buffer->size);
            readBufEndGet(buffer, consumed);
            rest_to_demux -= consumed;
        }

        proceed_audio();

        if (is_started == 0 && voBufIsFull(&voBuf) != 0 && is_audio_ok() != 0) {
            startDisplay(1);
            if (g_is_with_audio != 0) {
                audioDecStart(&audioDec);
            }
            is_started = 1;
        }
    }

    while (videoDecFlush(video_dec) == 0) {
        switchThread();
    }
    while (videoDecIsFlushed(video_dec) == 0 && videoDecGetState(video_dec) != VD_STATE_END) {
        switchThread();
    }
    endDisplay();
    if (g_is_with_audio != 0) {
        audioDecReset(&audioDec);
    }
    return 1;
}

// 0x00510e28
static void term_all(void) {
    reset_display(); // Inlined in the binary.
    readBufDelete(g_read_buf);
    voBufDelete(&voBuf);
    TerminateThread(g_decode_thread);
    DeleteThread(g_decode_thread);
    TerminateThread(g_default_thread);
    DeleteThread(g_default_thread);
    DisableIntc(INTC_VBLANK_S);
    RemoveIntcHandler(INTC_VBLANK_S, videoDec.hid_vblank);
    DisableDmac(DMAC_GIF);
    RemoveDmacHandler(DMAC_GIF, videoDec.hid_endimage);
    videoDecDelete(&videoDec);
    audioDecDelete(&audioDec);
    strFileClose(&g_in_file);
}

// 0x00510fc0
static int play(const char *name, int with_audio) {
    g_is_with_audio = with_audio;
    prepare_playback(name);
    play_mpeg(&videoDec, g_read_buf, &g_in_file);
    term_all();
    return g_pad_buttons;
}

// 0x00510f90
static void free_all(void) {
    MemFreeTagged(g_cutscene_memory, kSourceFile, kFreeLine);
}

// 0x00511010
void play_cutscene(const char *name, int with_audio) {
    const char *path = strchr(name, ':');
    if (path != NULL) {
        FILE *probe = fopen(path + 1, "rb");
        if (probe == NULL) {
            return;
        }
        fclose(probe);
    }
    init_all();
    play(name, with_audio);
    free_all();
}
