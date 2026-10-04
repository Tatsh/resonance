#include "app/cutscene.h"

#include <eekernel.h>
#include <eeregs.h>
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

// The one block every decoder buffer is carved from. Each buffer starts on a 64-byte boundary, and
// the block reserves 64 bytes of slack for aligning the first.
enum {
    kBufferAlign = 64,
    kAudioBufferSize = 0xc000,
    kAudioIopBufferSize = 0x6000,
    kVideoDataSize = 0x80000,
#ifdef VIDEO_STANDARD_PAL
    kMpegWorkSize = 0x1c8200,
    kFrameImagesSize = 0x32a000,
    kFrameTagsSize = 0x99c80,
    kCutsceneMemorySize = 0x667f00,
#else
    kMpegWorkSize = 0x17c300,
    kFrameImagesSize = 0x2a3000,
    kFrameTagsSize = 0x80880,
    kCutsceneMemorySize = 0x57bc00,
#endif
};

// Decoded frames the queue holds, tags and time stamps the decoder records, and the two thread
// stacks.
enum {
    kFrameCount = 2,
    kVideoTagCount = 0x100,
    kVideoTagSize = 16,
    kVideoTagSlots = ((kVideoTagCount + 1) * kVideoTagSize + kBufferAlign - 1) / kBufferAlign *
                     kBufferAlign / kVideoTagSize,
    kTimeStampCount = 0x200,
    kTimeStampSize = 0x18,
    kDefaultStackSize = 0x800,
    kDecodeStackSize = 0x4000,
};

// The display the movie is shown on, and its double buffer, which is half height.
enum {
    kDisplayWidth = 640,
#ifdef VIDEO_STANDARD_PAL
    kDisplayHeight = 512,
    kGsVideoMode = 3,
#else
    kDisplayHeight = 480,
    kGsVideoMode = 2,
#endif
    kGsPsmCt32 = 0,
    kGsInterlace = 1,
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

// NTSC-U/C: 0x0070cae0, PAL: 0x007509d0
static int g_is_with_audio = 1;

VoBuf voBuf __attribute__((aligned(kBufferAlign)));

sceGsDBuff db __attribute__((aligned(kBufferAlign)));

// NTSC-U/C: 0x0070cd30, PAL: 0x00750c20
static ReadBuf *g_read_buf;

// NTSC-U/C: 0x0070cd34, PAL: 0x00750c24
static int g_decode_thread;

// NTSC-U/C: 0x0070cd38, PAL: 0x00750c28
static int g_default_thread;

// NTSC-U/C: 0x0070cd40, PAL: 0x00750c30
static StrFile g_in_file;

VideoDec videoDec;

AudioDec audioDec;

// NTSC-U/C: 0x0070ce90, PAL: 0x00750d80
// The pad buttons read during the last pass of the playback loop, active high.
static int g_pad_buttons;

// NTSC-U/C: 0x00895400, PAL: 0x008da440
static int g_default_priority;

// NTSC-U/C: 0x00895404, PAL: 0x008da444
static void *g_frame_images;

// NTSC-U/C: 0x00895408, PAL: 0x008da448
static void *g_frame_tags;

// NTSC-U/C: 0x00895440, PAL: 0x008da480
// The ring the input buffer builds ends in one more tag that points back to the first. The input
// buffer rewrites the tags through the uncached segment while the DMA controller follows them. The
// ring therefore starts on a cache line and fills its last line. A cached variable sharing a line
// with the tags would write a stale copy of them back when the line is evicted.
static unsigned long long g_video_tags[kVideoTagSlots][2] __attribute__((aligned(kBufferAlign)));

// NTSC-U/C: 0x00896450, PAL: 0x008db490
static unsigned char *g_mpeg_work;

// NTSC-U/C: 0x00896454, PAL: 0x008db494
static unsigned char *g_audio_buffer;

// NTSC-U/C: 0x00896458, PAL: 0x008db498
static void *g_video_data;

// NTSC-U/C: 0x00896480, PAL: 0x008db4c0
static unsigned char g_default_stack[kDefaultStackSize] __attribute__((aligned(16)));

// NTSC-U/C: 0x00896c80, PAL: 0x008dbcc0
static unsigned char g_decode_stack[kDecodeStackSize] __attribute__((aligned(16)));

// NTSC-U/C: 0x0089ac80, PAL: 0x008dfcc0
// The next global starts 0x100 bytes beyond the time stamps. That bounds each at 0x18 bytes.
static unsigned char g_time_stamps[kTimeStampCount][kTimeStampSize];

// NTSC-U/C: 0x0089dd80, PAL: 0x008e2dc0
static void *g_cutscene_memory;

void ErrMessage(char *message) {
    printf("[ Error ] %s\n", message);
}

void switchThread(void) {
    RotateThreadReadyQueue(g_default_priority);
}

// NTSC-U/C: 0x00510f70, PAL: 0x005511e8
static void proceed_audio(void) {
    audioDecSendToIOP(&audioDec);
}

// NTSC-U/C: 0x00511100, PAL: 0x00551378
static int is_audio_ok(void) {
    if (g_is_with_audio == 0) {
        return 1;
    }
    return audioDecIsPreset(&audioDec);
}

// NTSC-U/C: 0x005110e0, PAL: 0x00551358
static void default_main(void *argument) {
    (void)argument;
    for (;;) {
        switchThread();
    }
}

// NTSC-U/C: 0x00511088, PAL: 0x00551300
static void reset_display(void) {
    clearGsMem(0, 0, 0, kDisplayWidth, kDisplayHeight);
    sceGsSetDefDBuff(&db, kGsPsmCt32, kDisplayWidth, kDisplayHeight / 2, 0, 0, kClearEnabled);
    FlushCache(kFlushCacheWriteBackData);
}

// NTSC-U/C: 0x005107a8, PAL: 0x00550a20
static void init_all(void) {
    g_cutscene_memory = MemAllocTagged(kCutsceneMemorySize, __FILE__, __LINE__);
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
    sceGsResetGraph(kGsResetAll, kGsInterlace, kGsVideoMode, kGsFrameMode);
    reset_display();
}

// NTSC-U/C: 0x00510b98, PAL: 0x00550e10
static void prepare_playback(const char *name) {
    *D_CTRL |= kDmaCtrlEnable;
    *D_STAT = kDmaStatClearVif1;

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
    struct ThreadParam status;
    ReferThreadStatus(GetThreadId(), &status);
    struct ThreadParam thread;
    thread.entry = default_main;
    thread.stack = g_default_stack;
    thread.stackSize = kDefaultStackSize;
    thread.initPriority = status.currentPriority;
    thread.gpReg = _gp;
    thread.option = 0;
    g_default_priority = status.currentPriority;
    g_default_thread = CreateThread(&thread);
    StartThread(g_default_thread, NULL);

    thread.entry = (void (*)(void *))videoDecMain;
    thread.stack = g_decode_stack;
    thread.stackSize = kDecodeStackSize;
    thread.initPriority = g_default_priority;
    thread.gpReg = _gp;
    thread.option = 0;
    g_decode_thread = CreateThread(&thread);
    StartThread(g_decode_thread, &videoDec);

    while (strFileOpen(&g_in_file, name) == 0) {
        printf("Can't Open file %s\n", name);
    }

    videoDec.hid_vblank = AddIntcHandler(INTC_VBLANK_S, vblankHandler, 0);
    EnableIntc(INTC_VBLANK_S);
    videoDec.hid_endimage = AddDmacHandler(DMAC_GIF, handler_endimage, 0);
    EnableDmac(DMAC_GIF);
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

// NTSC-U/C: 0x005108b0, PAL: 0x00550b28
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

// NTSC-U/C: 0x00510e28, PAL: 0x005510a0
static void term_all(void) {
    reset_display(); // Inlined in the binary.
    readBufDelete(g_read_buf);
    voBufDelete(&voBuf);
    TerminateThread(g_decode_thread);
    DeleteThread(g_decode_thread);
    TerminateThread(g_default_thread);
    DeleteThread(g_default_thread);
    DisableDmac(DMAC_GIF);
    RemoveDmacHandler(DMAC_GIF, videoDec.hid_endimage);
    DisableIntc(INTC_VBLANK_S);
    RemoveIntcHandler(INTC_VBLANK_S, videoDec.hid_vblank);
    videoDecDelete(&videoDec);
    audioDecDelete(&audioDec);
    strFileClose(&g_in_file);
}

// NTSC-U/C: 0x00510fc0, PAL: 0x00551238
static int play(const char *name, int with_audio) {
    g_is_with_audio = with_audio;
    prepare_playback(name);
    play_mpeg(&videoDec, g_read_buf, &g_in_file);
    term_all();
    return g_pad_buttons;
}

// NTSC-U/C: 0x00510f90, PAL: 0x00551208
static void free_all(void) {
    MemFreeTagged(g_cutscene_memory, __FILE__, __LINE__);
}

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
