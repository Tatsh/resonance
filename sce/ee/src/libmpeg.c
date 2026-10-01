#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <eekernel.h>
#include <eetypes.h>
#include <libmpeg.h>

#include "os/log.h"

// Layout facts recovered from the disassembly of the routines in this file. The decoder
// (sceMpeg) lives in the caller and points at the work area through pContext (+0x40). The work
// area holds seven callback slots (+0x0c), the stream table pointer (+0x44), and the stream
// count (+0x48), followed by decoder state and the input ring (+0x108).
enum {
    kAlignMask = 3,
    kPictureAlignMask = 0x3f,
    kPhysicalAddressMask = 0x0fffffff,
    kUncachedSegment = 0x20000000,
    kMaxStreamCallbacks = 0x40,
    kStreamEntrySize = 0x18,
    kMinWorkSize = 0x118,
    kErrorMessageSize = 256,
    kStreamAllocSize = 0x600,
    kStreamAllocAlign = 8,
    kSlotCount = 7,
    kTableCount = 9,
    kPictureCountIntra = 0,
    kPictureCountPredicted = 1,
    kPictureCountBidirectional = 2,
    kPictureCountTypes = 3,
    kFrameCentreOffsetCount = 3
};

// One stream entry, 0x18 bytes. The key is compared as eight bytes, the template copies the
// eight bytes for the stream type, and the callback returns on duplicate registration.
typedef struct {
    unsigned long long key; // Combined key, compared as a pair. +0x00
    unsigned long long templateBits; // Template for the stream type. +0x08
    sceMpegCallback callback; // Stream callback, returned on duplicate registration. +0x10
    void *data; // Stream data. +0x14
} StreamEntry;

// One callback slot, 8 bytes. Seven slots run from +0x0c to +0x44, where the table pointer
// sits. Slots two and three start with default callbacks.
typedef struct {
    void *callback; // Slot callback. A replacement returns the old callback. +0x00
    void *data; // Slot data. +0x04
} MpegSlot;

// The input ring at +0x108. The base and size bound the buffer while the write pointer doubles
// as the bump allocator cursor and the commit pointer saves it.
typedef struct {
    int mBase; // Buffer base, set at reset. +0x00
    int mSize; // Buffer size, set at reset. +0x04
    int mWrite; // Write position, bumped by allocation. +0x08
    int mCommit; // Committed write position. +0x0c
} MpegRing;

// The work area behind the decoder context pointer. Reserved members are never read or written.
typedef struct {
    int mCompleted; // Set to 1 when the picture is done and cleared to arm or fail. +0x00
    int mPictureIndex; // Pictures decoded since the last flush. +0x04
    int mOutputState; // Idle, decoding, or shown, and cleared by sceMpegReset(). +0x08
    MpegSlot mSlots[kSlotCount]; // Callback slots. +0x0c
    StreamEntry *mStreamTable; // Stream entries, bump-allocated from the ring. +0x44
    int mStreamCount; // Entries used. Duplicates overwrite and still count. +0x48
    int mReserved4C[9]; // Untouched at creation. +0x4c
    int mUseDefaultPtsGap; // Set to 1 to interpolate a missing stamp from the default gap. +0x70
    int mReserved74; // Untouched at creation. +0x74
    unsigned long long mDefaultPtsGap; // Stamp ticks per frame for the interpolation. +0x78
    int mLastPts; // Stamp of the last output picture, -1 at creation and on drain. +0x80
    int mReserved84; // Untouched at creation. +0x84
    unsigned long long mDisplayFieldCount; // Fields the displayed picture occupies. +0x88
    int mOddGapPictures; // Pictures interpolated with an odd gap, for the rounding. +0x90
    int mDecodeLimits[kPictureCountTypes]; // Pictures of each type to decode, -1 for all. +0x94
    int mDecodeCounts[kPictureCountTypes]; // Pictures of each type met so far. +0xa0
    int mFrameCountBase; // Picture counter when the first picture was shown. +0xac
    int mConvertColours; // Set to 1 to colour-convert the output, 0 to copy it raw. +0xb0
    // Copied from the displayed picture. +0xb4
    int mFrameCentreHorizontalOffset[kFrameCentreOffsetCount];
    // Copied from the displayed picture. +0xc0
    int mFrameCentreVerticalOffset[kFrameCentreOffsetCount];
    int mDisplayHorizontalSize; // Copied from the displayed picture. +0xcc
    int mDisplayVerticalSize; // Copied from the displayed picture. +0xd0
    int mFirstFieldStructure; // The picture_structure of the sequence's first picture. +0xd4
    int mPictureAddress; // Output buffer of the picture under decode. +0xd8
    int mOutputWidth; // Output buffer width in pixels, or 0 for a macroblock count. +0xdc
    int mOutputHeight; // Output buffer height in pixels, or 0 for a macroblock count. +0xe0
    int mOutputMacroblocks; // Output buffer capacity in macroblocks. +0xe4
    // Treated as broken_link and only ever cleared. The purpose is inferred. +0xe8
    int mForceBrokenLink;
    int mReservedEC; // Untouched by the observed code. +0xec
    long long mPendingPts; // Pending time stamp, or -1 when there is none. +0xf0
    int mPendingPtsState; // Armed, then ready once a picture is output. +0xf8
    int mFirstFrameBuffer; // First of three frame buffers carved from the ring per sequence. +0xfc
    int mSecondFrameBuffer; // +0x100
    int mThirdFrameBuffer; // +0x104
    MpegRing mRing; // Input ring. +0x108
} MpegWork;

// One motion compensation job for a kernel, 28 bytes. The source spans two staged macroblock
// columns and continues into the macroblock below after mRows rows.
typedef struct {
    int mDest;        // Prediction output address. +0x00
    int mShift;       // Byte offset of the reference column within the loaded quadword. +0x04
    int mRows;        // Rows read before the source crosses into the macroblock below. +0x08
    int mRowsBelow;   // Rows read from the macroblock below. +0x0c
    int mStride;      // Source row stride. +0x10
    int mSourceLeft;  // Source in the left staged column. +0x14
    int mSourceRight; // Source in the right staged column. +0x18
} MpegMcDescriptor;

typedef void (*MpegMcKernel)(const MpegMcDescriptor *pDescriptor);

enum {
    kMcPredictionSlots = 4,
    kMcKernelVariants = 8,
};

// One macroblock in flight, 0x140 bytes. Two alternate so the kernels of one macroblock run while
// the IPU decodes the next.
typedef struct {
    int mStaging;                                     // Reference macroblocks land here. +0x000
    int mCoefficients;                                // The IPU writes the block here. +0x004
    int mRefLeft[kMcPredictionSlots];                 // Left reference column pair. +0x008
    int mRefRight[kMcPredictionSlots];                // Right reference column pair. +0x018
    MpegMcKernel mLumaKernels[kMcPredictionSlots];    // +0x028
    MpegMcKernel mChromaKernels[kMcPredictionSlots];  // +0x038
    MpegMcDescriptor mLuma[kMcPredictionSlots];       // +0x048
    MpegMcDescriptor mChroma[kMcPredictionSlots];     // +0x0b8
    int mOutput;                                      // Macroblock in the frame. +0x128
    int mPredictionCount;                             // +0x12c
    int mIntra;                                       // +0x130
    int mFollowsCoded;                                // Coded and adjacent, and never read. +0x134
    int mDmaPending;                                  // References are being staged. +0x138
    int mNotCoded;                                    // The macroblock has no block data. +0x13c
} MpegMcBuffer;

// Motion compensation state at 0x007a30f8.
typedef struct {
    MpegMcBuffer mBuffers[2]; // +0x000
    int mCurrent;             // Buffer the next macroblock uses. +0x280
    int mPictureResetWord;    // Cleared per picture. No reader was found. +0x284
} MpegIpuTable;
static MpegIpuTable g_mpegIpuTable;

// One picture table, 0x68 bytes. The picture setup writes the first five words, and the reorder
// step copies the picture header state into the rest when a picture is decoded into it.
typedef struct {
    int mBuffer; // Frame buffer address, uncached. +0x00
    int mWidth; // Width in pixels. +0x04
    int mHeight; // Height in pixels. +0x08
    int mMbWidth; // Width in macroblocks. +0x0c
    int mMbHeight; // Height in macroblocks and the column stride of the buffer. +0x10
    int mReserved14; // Untouched by the observed writers. +0x14
    long long mPts; // Presentation time stamp, or -1 when absent. +0x18
    long long mDts; // Decoding time stamp, or -1 when absent. +0x20
    int mDecoded; // Set to 1 once the picture is complete and cleared when it is reused. +0x28
    int mPictureCodingType; // +0x2c
    int mPictureStructure; // +0x30
    int mProgressiveSequence; // +0x34
    int mProgressiveFrame; // +0x38
    int mTopFieldFirst; // +0x3c
    int mRepeatFirstField; // +0x40
    int mFrameCentreHorizontalOffset[kFrameCentreOffsetCount]; // +0x44
    int mFrameCentreVerticalOffset[kFrameCentreOffsetCount]; // +0x50
    int mDisplayHorizontalSize; // +0x5c
    int mDisplayVerticalSize; // +0x60
    int mReserved64; // Untouched by the observed writers. +0x64
} MpegSeqTable;


// Sequence table arenas at 0x007a2d50, nine of 0x68 bytes ending where the IPU table below
// begins. Creation points the table slots at them in order.
static MpegSeqTable g_mpegSeqAreas[kTableCount];
static MpegSeqTable *g_mpegTables[kTableCount];

// Nibble dispatch words at 0x007a3408, read in full. The last two are code addresses the image
// stores as data; no call passes through them here.
static unsigned int g_mpegNibbleTable[16] = {
    0x00000001u, 0x00000001u, 0x00000000u, 0x00000000u,
    0x00000000u, 0x00000001u, 0x00000001u, 0x00000001u,
    0x00000001u, 0x00000001u, 0x00000000u, 0xffffffffu,
    0x00000000u, 0x00000000u, 0x0060e880u, 0x0060e668u,
};

// Extension handlers at 0x007a3440, indexed by extension_start_code_identifier. The last two
// nibble table words above are the first two entries.
int sceMpegReservedExtension(void);
int sceMpegSequenceExtension(void);
int sceMpegSequenceDisplayExtension(void);
int sceMpegQuantMatrixExtension(void);
int sceMpegCopyrightExtension(void);
int sceMpegSequenceScalableExtension(void);
int sceMpegPictureSpatialScalableExtension(void);
int sceMpegPictureDisplayExtension(void);
int sceMpegPictureCodingExtension(void);
int sceMpegPictureTemporalScalableExtension(void);
typedef int (*MpegExtensionHandler)(void);
static MpegExtensionHandler g_mpegExtensionHandlers[11] = {
    sceMpegReservedExtension,
    sceMpegSequenceExtension,
    sceMpegSequenceDisplayExtension,
    sceMpegQuantMatrixExtension,
    sceMpegCopyrightExtension,
    sceMpegSequenceScalableExtension,
    sceMpegReservedExtension,
    sceMpegPictureDisplayExtension,
    sceMpegPictureCodingExtension,
    sceMpegPictureSpatialScalableExtension,
    sceMpegPictureTemporalScalableExtension,
};

// The scratchpad offsets of the two macroblock buffers sceMpegResetMcBuffers() places.
enum {
    kMcFirstCoefficientsOffset = 0x1800,
    kMcSecondStagingOffset = 0x1b00,
    kMcSecondCoefficientsOffset = 0x3300,
};

// The scratchpad base sceMpegResetMcBuffers() derives the macroblock buffers from.
// 0x007a38b0
static int g_mpegIpuBase = 0x70000000;
static int g_mpegIpuBusyFlag; // Word at 0x007a2b24, nonzero while an IPU command is outstanding.
static int g_nMpegIsMpeg2;    // Word at 0x007a33b0, set once a sequence extension marks MPEG-2.

// The default quantiser matrices in zigzag order. The IPU reads them by DMA when a sequence header
// does not load its own.
// 0x007a2b80
static const unsigned char g_abMpegDefaultIntraMatrix[] __attribute__((aligned(16))) = {
    8,  16, 16, 19, 16, 19, 22, 22, 22, 22, 22, 22, 26, 24, 26, 27, 27, 27, 26, 26, 26, 26,
    27, 27, 27, 29, 29, 29, 34, 34, 34, 29, 29, 29, 27, 27, 29, 29, 32, 32, 34, 34, 37, 38,
    37, 35, 35, 34, 35, 38, 38, 40, 40, 40, 48, 48, 46, 46, 56, 56, 58, 69, 69, 83};
// 0x007a2bc0
static const unsigned char g_abMpegDefaultNonIntraMatrix[] __attribute__((aligned(16))) = {
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16};
static int g_mpegShiftAccum; // Word at 0x007a3398, the next 32 bits, shifted down by the readers.
static int g_mpegShiftBudget; // Word at 0x007a339c, compared against the bit count read.

// Picture header, group of pictures header, and picture numbering words.
static int g_nMpegTemporalReference; // Word at 0x007a2c78.
static int g_nMpegPictureCodingType; // Word at 0x007a2c7c, returned by the picture header search.
static int g_nMpegVbvDelay; // Word at 0x007a2c80.
static int g_nMpegFullPelForwardVector; // Word at 0x007a2c84.
static int g_nMpegForwardFCode; // Word at 0x007a2c88.
static int g_nMpegFullPelBackwardVector; // Word at 0x007a2c8c.
static int g_nMpegBackwardFCode; // Word at 0x007a2c90.
static int g_nMpegGopPictureBase; // Word at 0x007a3430, picture number of temporal reference 0.
static int g_nMpegLatestPictureNumber; // Word at 0x007a3434, highest picture number so far.
static int g_nMpegGopStarted; // Word at 0x007a3438, set by a GOP header until the next picture.
static int g_nMpegDropFrameFlag; // Word at 0x007a2d1c.
static int g_nMpegTimeCodeHours; // Word at 0x007a2d20.
static int g_nMpegTimeCodeMinutes; // Word at 0x007a2d24.
static int g_nMpegTimeCodeSeconds; // Word at 0x007a2d28.
static int g_nMpegTimeCodePictures; // Word at 0x007a2d2c.
static int g_nMpegClosedGop; // Word at 0x007a2d30.
static int g_nMpegBrokenLink; // Word at 0x007a2d34.
static unsigned long long g_llMpegNextPts; // Words at 0x007a3388, from the time stamp callback.
static unsigned long long g_llMpegNextDts; // Words at 0x007a3390, from the time stamp callback.
static int g_nMpegTemporalReferenceWrapped; // Word at 0x007a346c, set once the reference wraps.
static int g_nMpegPreviousTemporalReference; // Word at 0x007a3470.
static int g_nMpegPictureNumber; // Word at 0x007a2d38, temporal reference plus the GOP base.
static int g_nMpegResetDcPredictor; // Word at 0x007a2d3c, the BDEC DC reset bit.

// Sequence header words and the frame geometry derived from them.
static int g_nMpegCodedWidth; // Word at 0x007a2c0c.
static int g_nMpegCodedHeight; // Word at 0x007a2c10.
static int g_nMpegChromaWidth; // Word at 0x007a2c14.
static int g_nMpegChromaHeight; // Word at 0x007a2c18.
static int g_nMpegHorizontalSize; // Word at 0x007a2c20.
static int g_nMpegVerticalSize; // Word at 0x007a2c24.
static int g_nMpegMbWidth; // Word at 0x007a2c28.
static int g_nMpegMbHeight; // Word at 0x007a2c2c.
static int g_nMpegAspectRatioInformation; // Word at 0x007a2c30.
static int g_nMpegFrameRateCode; // Word at 0x007a2c34.
static int g_nMpegBitRate; // Word at 0x007a2c38.
static int g_nMpegVbvBufferSize; // Word at 0x007a2c3c.
static int g_nMpegConstrainedParametersFlag; // Word at 0x007a2c40.
static int g_nMpegProgressiveSequence; // Word at 0x007a2c48.
static int g_nMpegChromaFormat; // Word at 0x007a2c4c.
static int g_nMpegMatrixCoefficients; // Word at 0x007a2c6c.
static int g_nMpegFramePredFrameDct; // Word at 0x007a2cb4.
static int g_nMpegProgressiveFrame; // Word at 0x007a2cc8.
static int g_nMpegLoadIntraQuantiserMatrix; // Word at 0x007a33a0.
static int g_nMpegLoadNonIntraQuantiserMatrix; // Word at 0x007a33a4.

// Fields the MPEG-2 extensions record.
static int g_nMpegProfileAndLevel;       // Word at 0x007a2c44, profile_and_level_indication.
static int g_nMpegLowDelay;       // Word at 0x007a2c50, low_delay.
static int g_nMpegFrameRateExtensionN;       // Word at 0x007a2c54, frame_rate_extension_n.
static int g_nMpegFrameRateExtensionD;       // Word at 0x007a2c58, frame_rate_extension_d.
static int g_nMpegVideoFormat;       // Word at 0x007a2c5c, video_format.
static int g_nMpegColourDescription;       // Word at 0x007a2c60, colour_description.
static int g_nMpegColourPrimaries;       // Word at 0x007a2c64, colour_primaries.
static int g_nMpegTransferCharacteristics;       // Word at 0x007a2c68, transfer_characteristics.
static int g_nMpegDisplayHorizontalSize;       // Word at 0x007a2c70, display_horizontal_size.
static int g_nMpegDisplayVerticalSize;       // Word at 0x007a2c74, display_vertical_size.
static int g_anMpegFCodes[4];    // Words at 0x007a2c98, the four f_code values.
static int g_nMpegIntraDcPrecision;       // Word at 0x007a2ca8, intra_dc_precision.
static int g_nMpegPictureStructure;       // Word at 0x007a2cac, picture_structure.
static int g_nMpegTopFieldFirst;       // Word at 0x007a2cb0, top_field_first.
static int g_nMpegConcealmentMotionVectors;       // Word at 0x007a2cb8, concealment_motion_vectors.
static int g_nMpegIntraVlcFormat;       // Word at 0x007a2cbc, intra_vlc_format.
static int g_nMpegRepeatFirstField;       // Word at 0x007a2cc0, repeat_first_field.
static int g_nMpegChroma420Type;       // Word at 0x007a2cc4, chroma_420_type.
static int g_nMpegCompositeDisplayFlag;       // Word at 0x007a2ccc, composite_display_flag.
static int g_nMpegVAxis;       // Word at 0x007a2cd0, v_axis.
static int g_nMpegFieldSequence;       // Word at 0x007a2cd4, field_sequence.
static int g_nMpegSubCarrier;       // Word at 0x007a2cd8, sub_carrier.
static int g_nMpegBurstAmplitude;       // Word at 0x007a2cdc, burst_amplitude.
static int g_nMpegSubCarrierPhase;       // Word at 0x007a2ce0, sub_carrier_phase.
// Words at 0x007a2ce8 and 0x007a2cf8, frame_centre_horizontal_offset and
// frame_centre_vertical_offset.
static int g_anMpegFrameCentreHorizontalOffsets[3];
static int g_anMpegFrameCentreVerticalOffsets[3];
static int g_nMpegCopyrightFlag;       // Word at 0x007a2d04, copyright_flag.
static int g_nMpegCopyrightIdentifier;       // Word at 0x007a2d08, copyright_identifier.
static int g_nMpegOriginalOrCopy;       // Word at 0x007a2d0c, original_or_copy.
static int g_nMpegCopyrightNumber1;       // Word at 0x007a2d10, copyright_number_1.
static int g_nMpegCopyrightNumber2;       // Word at 0x007a2d14, copyright_number_2.
static int g_nMpegCopyrightNumber3;       // Word at 0x007a2d18, copyright_number_3.
static int g_nMpegQScaleType;       // Word at 0x007a33b4, q_scale_type.
static int g_nMpegAlternateScan;       // Word at 0x007a33b8, alternate_scan.

// Forward declarations for the header parsers, whose routines call one another in an order the
// file layout does not match.
static void sceMpegExtensionAndUserData(void);
static int sceMpegUpdatePictureNumber(void);
static int sceMpegSequenceHeader(void);
static int sceMpegIssueIpuCommand(unsigned int nCommand);
static int sceMpegLoadDefaultMatrix(unsigned int nCommand, const unsigned char *pMatrix);
static int sceMpegSetTableSize(MpegSeqTable *pTable, int nWidth, int nHeight);
static void sceMpegAssignFrameBuffers(MpegSeqTable *pFrameForward,
                                      MpegSeqTable *pFrameBackward,
                                      MpegSeqTable *pFrameBidirectional,
                                      MpegSeqTable *pTopForward,
                                      MpegSeqTable *pTopBackward,
                                      MpegSeqTable *pTopBidirectional,
                                      MpegSeqTable *pBottomForward,
                                      MpegSeqTable *pBottomBackward,
                                      MpegSeqTable *pBottomBidirectional,
                                      int nForwardBuffer,
                                      int nBackwardBuffer,
                                      int nBidirectionalBuffer);
static int sceMpegReportNoData(void *pDecoder);
static int sceMpegWaitIpuIdle(void);
static int sceMpegReadIpuData(void);
static int sceMpegSkipBits(int nBits);
static int sceMpegPeekBits(int nBits);
static int sceMpegNextStartCode(void);
int sceIpuSetMpeg1Mode(int nMpeg1);
static void sceMpegSkipExtraInformation(void);
static void *g_decoderInstance;

// IPU register words and the watchdog limit the bit readers share.
enum {
    kIpuCommandAddress = 0x10002000,
    kIpuControlAddress = 0x10002010,
    kIpuBusyMask = 0x80004000u,
    kIpuBusyValue = 0x80000000u,
    kIpuDataReadyBit = 0x4000u,
    kIpuWatchdogLimit = 0x1389u
};

// Registers and values the picture output path drives.
enum {
    kIpuControlReset = 0x40000000,
    kIpuControlErrorBit = 0x4000,
    kIpuCommandBitstreamClear = 0,
    kIpuCommandColourConvert = 0x70000000,
    kIpuFromChcrAddress = 0x1000b000,
    kIpuFromMadrAddress = 0x1000b010,
    kIpuFromQwcAddress = 0x1000b020,
    kIpuToChcrAddress = 0x1000b400,
    kIpuToMadrAddress = 0x1000b410,
    kIpuToQwcAddress = 0x1000b420,
    kFromSprChcrAddress = 0x1000d000,
    kFromSprMadrAddress = 0x1000d010,
    kFromSprQwcAddress = 0x1000d020,
    kFromSprSadrAddress = 0x1000d080,
    kToSprChcrAddress = 0x1000d400,
    kToSprMadrAddress = 0x1000d410,
    kToSprQwcAddress = 0x1000d420,
    kToSprSadrAddress = 0x1000d480,
    kDmacStatusAddress = 0x1000e010,
    kDmaChannelFromIpu = 3,
    kDmaChannelToIpu = 4,
    kDmacStatusFromIpu = 1 << kDmaChannelFromIpu,
    kDmacStatusToIpu = 1 << kDmaChannelToIpu,
    kDmaChcrStart = 0x100,
    kDmaChcrStartFromMemory = 0x101,
    kDmaChcrStartBit = 8,
    kMacroblockQwc = 24,
    kMacroblockBytes = 384,
    kHalfMacroblockBytes = 192,
    kQwcShift = 4,
    kMacroblockQwcShift = 6,
    kMacroblockPixels = 256,
    kColourConvertLimit = 1024,
    kColourConvertChunkMacroblocks = 1023,
    kColourConvertChunkQwc = 0xffc0,
    kColourConvertChunkBytes = 0xffc00,
    kToIpuChunkQwc = 0xffff,
    kToIpuChunkBytes = 0xffff0,
};

// Callback types the output path reports through the callback slots.
enum {
    kMpegCbStopDma = 2,
    kMpegCbRestartDma = 3,
    kMpegCbBackground = 4,
};

// picture_structure and picture_coding_type values.
enum {
    kPictureStructureTopField = 1,
    kPictureStructureBottomField = 2,
    kPictureStructureFrame = 3,
    kPictureCodingPredicted = 2,
    kPictureCodingBidirectional = 3,
};

// Slots of g_mpegTables. Each picture structure has an older reference, a newer reference, and
// a bidirectional picture.
enum {
    kTableFrameForward = 0,
    kTableFrameBackward = 1,
    kTableFrameBidirectional = 2,
    kTableTopForward = 3,
    kTableTopBackward = 4,
    kTableTopBidirectional = 5,
    kTableBottomForward = 6,
    kTableBottomBackward = 7,
    kTableBottomBidirectional = 8,
};

// The tables the current picture decodes into, one per picture structure.
enum {
    kCurrentFrame = 0,
    kCurrentTopField = 1,
    kCurrentBottomField = 2,
    kCurrentTableCount = 3,
};

// Bits of the picture flags word the output path sets.
enum {
    kPictureFlagFieldCountShift = 5,
    kPictureFlagFieldCountMask = 0xf,
    kPictureFlagTopFieldFirst = 0x40,
    kPictureFlagStructureShift = 3,
    kPictureFlagRepeatFirstFieldShift = 5,
    kPictureFlagTopFieldFirstShift = 6,
    kPictureFlagProgressiveFrameShift = 7,
    kPictureFlagProgressiveSequenceShift = 8,
};

// 0x007a2d44
static MpegSeqTable *g_mpegCurrentTables[kCurrentTableCount];

// Set between the first and second field of a field picture pair.
// 0x007a2c1c
static int g_mpegSecondFieldPending;

// Fields each picture occupies, indexed by the repeat_first_field, top_field_first,
// progressive_frame, and progressive_sequence flag bits.
// 0x007a3478
static const int g_mpegFieldCountTable[] = {2, 0, 2, 0, 2, 3, 2, 3, 0, 0, 0, 0, 2, 4, 0, 6};

// The macroblock stream the to-IPU interrupt handler continues, and the interrupts it has taken.
// 0x008ea920
static volatile int g_nToIpuInterrupts;
// 0x008ea924
static volatile unsigned int g_nToIpuRemainingQwc;
// 0x008ea928
static volatile unsigned int g_nToIpuNextAddress;

// The chunked colour conversion the from-IPU interrupt handler continues.
// 0x008ea914
static volatile int g_nColourConvertRemaining;
// 0x008ea918
static volatile unsigned int g_nColourConvertNextAddress;
// 0x008ea91c
static volatile int g_nColourConvertChunks;
// 0x007c3818
static volatile int g_nColourConvertError;
// 0x007c3820
static volatile int g_nFromIpuInterrupts;

// 0x0060b820
static int sceMpegGetBits(int nBits) {
    volatile unsigned int *pControl;
    volatile unsigned int *pData;
    unsigned int count;
    unsigned int command;
    unsigned int index;
    int shifted;

    pControl = (volatile unsigned int *)(uintptr_t)kIpuControlAddress;
    pData = (volatile unsigned int *)(uintptr_t)kIpuCommandAddress;
    if ((*pControl & kIpuBusyMask) == kIpuBusyValue) {
        count = 0;
        for (;;) {
            if (count >= kIpuWatchdogLimit) {
                sceMpegReportNoData(g_decoderInstance);
                count = 0;
            } else {
                ++count;
            }
            if ((*pControl & kIpuBusyMask) != kIpuBusyValue) {
                break;
            }
        }
    }
    if (g_mpegIpuBusyFlag != 0 || g_mpegShiftBudget < nBits) {
        *pData = 0x40000000u;
        g_mpegIpuBusyFlag = (int)g_mpegNibbleTable[4];
        g_mpegShiftAccum = sceMpegReadIpuData();
    }
    g_mpegShiftBudget = 0x20;
    command = (unsigned int)nBits | 0x40000000u;
    *pData = command;
    shifted = (int)((unsigned int)g_mpegShiftAccum >> ((0x20 - nBits) & 31));
    index = (command >> 28) & 0xfu;
    g_mpegIpuBusyFlag = (int)g_mpegNibbleTable[index];
    g_mpegShiftAccum = sceMpegReadIpuData();
    return shifted;
}

// 0x0060bb88
static int sceMpegPictureHeader(void) {
    g_nMpegTemporalReference = sceMpegGetBits(0xa);
    g_nMpegPictureCodingType = sceMpegGetBits(3);
    g_nMpegVbvDelay = sceMpegGetBits(0x10);
    if ((unsigned int)(g_nMpegPictureCodingType - 2) < 2u) {
        g_nMpegFullPelForwardVector = sceMpegGetBits(1);
        g_nMpegForwardFCode = sceMpegGetBits(3);
    }
    if (g_nMpegPictureCodingType == 3) {
        g_nMpegFullPelBackwardVector = sceMpegGetBits(1);
        g_nMpegBackwardFCode = sceMpegGetBits(3);
    }
    sceMpegSkipExtraInformation();
    sceMpegExtensionAndUserData();
    return sceMpegUpdatePictureNumber();
}

// 0x0060c050
static void sceMpegGroupOfPicturesHeader(void) {
    sceMpeg *decoder;
    MpegWork *work;

    decoder = (sceMpeg *)g_decoderInstance;
    work = (MpegWork *)decoder->pContext;
    work->mForceBrokenLink = 0;
    g_nMpegGopPictureBase = g_nMpegLatestPictureNumber + 1;
    g_nMpegGopStarted = 1;
    g_nMpegDropFrameFlag = sceMpegGetBits(1);
    g_nMpegTimeCodeHours = sceMpegGetBits(5);
    g_nMpegTimeCodeMinutes = sceMpegGetBits(6);
    (void)sceMpegGetBits(1);
    g_nMpegTimeCodeSeconds = sceMpegGetBits(6);
    g_nMpegTimeCodePictures = sceMpegGetBits(6);
    g_nMpegClosedGop = sceMpegGetBits(1);
    g_nMpegBrokenLink = sceMpegGetBits(1);
    sceMpegExtensionAndUserData();
}

// 0x0060bf38
static void sceMpegSkipExtraInformation(void) {
    for (;;) {
        if (sceMpegGetBits(1) == 0) {
            return;
        }
        sceMpegSkipBits(8);
    }
}

// 0x0060bc58
static void sceMpegExtensionAndUserData(void) {
    int index;

    sceMpegNextStartCode();
    for (;;) {
        index = sceMpegPeekBits(0x20);
        if (index == 0x1b5) {
            sceMpegSkipBits(0x20);
            index = sceMpegGetBits(4);
            if ((unsigned int)index > 10u) {
                index = 0;
            }
            g_mpegExtensionHandlers[index]();
            sceMpegNextStartCode();
            continue;
        }
        if (index != 0x1b2) {
            return;
        }
        sceMpegSkipBits(0x20);
        sceMpegNextStartCode();
    }
}

// 0x0060bf70
static int sceMpegUpdatePictureNumber(void) {
    if (g_nMpegPictureCodingType != 3 && g_nMpegTemporalReference != g_nMpegPreviousTemporalReference) {
        if (g_nMpegTemporalReferenceWrapped != 0) {
            g_nMpegTemporalReferenceWrapped = 0;
            g_nMpegGopPictureBase += 0x400;
        }
        if (g_nMpegTemporalReference < g_nMpegPreviousTemporalReference && g_nMpegGopStarted == 0) {
            g_nMpegTemporalReferenceWrapped = 1;
        }
        g_nMpegGopStarted = 0;
        g_nMpegPreviousTemporalReference = g_nMpegTemporalReference;
    }
    g_nMpegPictureNumber = g_nMpegGopPictureBase + g_nMpegTemporalReference;
    if (g_nMpegTemporalReferenceWrapped != 0 && g_nMpegPreviousTemporalReference >= g_nMpegTemporalReference) {
        g_nMpegPictureNumber += 0x400;
    }
    if (g_nMpegLatestPictureNumber < g_nMpegPictureNumber) {
        g_nMpegLatestPictureNumber = g_nMpegPictureNumber;
    }
    return g_nMpegLatestPictureNumber;
}

// 0x0060ba60
int sceMpegNextPictureHeader(void) {
    StreamEntry entry;

    for (;;) {
        sceMpegNextStartCode(); // Yes, the binary discards the result and reads the code itself.
        int status = sceMpegGetBits(0x20);
        if (status == 0x1b3) {
            sceMpegSequenceHeader();
            continue;
        }
        if ((unsigned int)status >= 0x1b4u) {
            if (status == 0x1b7) {
                return 0;
            }
            if (status == 0x1b8) {
                sceMpegGroupOfPicturesHeader();
            }
            continue;
        }
        if (status != 0x100) {
            continue;
        }
        sceMpegPictureHeader();
        entry.key = 5;
        entry.templateBits = ~(unsigned long long)0;
        entry.callback = (void *)~(uintptr_t)0;
        entry.data = (void *)~(uintptr_t)0;
        sceMpegInvokeCallbackSlot(g_decoderInstance, &entry);
        g_llMpegNextPts = entry.templateBits;
        g_llMpegNextDts = (unsigned long long)(unsigned int)(uintptr_t)entry.data << 32 |
            (unsigned int)(uintptr_t)entry.callback;
        return g_nMpegPictureCodingType;
    }
}

// 0x0060e020
static int sceMpegSequenceHeader(void) {
    sceMpeg *decoder;
    MpegWork *work;
    int bits;
    int half;
    int frameBytes;

    decoder = (sceMpeg *)g_decoderInstance;
    work = (MpegWork *)decoder->pContext;
    // The clear falls in the setup call delay slot, so it lands before the setup body.
    work->mFirstFieldStructure = 0;
    bits = sceMpegGetBits(0x20);
    g_nMpegFrameRateCode = bits & 0xf;
    g_nMpegAspectRatioInformation = (bits >> 4) & 0xf;
    g_nMpegHorizontalSize = (unsigned int)bits >> 0x14;
    if (((bits >> 8) & 0xfff) >= 0xaf1) {
        sceMpegRaiseError("vertical size > 2800");
    }
    g_nMpegVerticalSize = (bits >> 8) & 0xfff;
    bits = sceMpegGetBits(0x1e);
    g_nMpegConstrainedParametersFlag = bits & 1;
    g_nMpegVbvBufferSize = (bits >> 1) & 0x3ff;
    g_nMpegBitRate = (unsigned int)bits >> 12;
    bits = sceMpegGetBits(1);
    g_nMpegLoadIntraQuantiserMatrix = bits;
    if (bits == 0) {
        sceMpegLoadDefaultMatrix(0x50000000u, g_abMpegDefaultIntraMatrix);
    } else {
        sceMpegWaitIpuIdle();
        sceMpegIssueIpuCommand(0x50000000u);
        sceMpegWaitIpuIdle();
    }
    bits = sceMpegGetBits(1);
    g_nMpegLoadNonIntraQuantiserMatrix = bits;
    if (bits == 0) {
        sceMpegLoadDefaultMatrix(0x58000000u, g_abMpegDefaultNonIntraMatrix);
    } else {
        sceMpegWaitIpuIdle();
        sceMpegIssueIpuCommand(0x58000000u);
        sceMpegWaitIpuIdle();
    }
    sceMpegExtensionAndUserData();
    decoder = (sceMpeg *)g_decoderInstance;
    work = (MpegWork *)decoder->pContext;
    if (g_nMpegIsMpeg2 == 0) {
        g_nMpegPictureStructure = 3;
        g_nMpegFramePredFrameDct = 1;
        g_nMpegMatrixCoefficients = 5;
        g_nMpegProgressiveSequence = 1;
        g_nMpegChromaFormat = 1;
        g_nMpegProgressiveFrame = 1;
    }
    g_nMpegMbWidth = (g_nMpegHorizontalSize + 0xf) >> 4;
    if (g_nMpegIsMpeg2 == 0 || g_nMpegProgressiveSequence != 0) {
        g_nMpegMbHeight = (g_nMpegVerticalSize + 0xf) >> 4;
    } else {
        g_nMpegMbHeight = ((g_nMpegVerticalSize + 0x1f) >> 5) << 1;
    }
    g_nMpegCodedWidth = g_nMpegMbWidth << 4;
    g_nMpegCodedHeight = g_nMpegMbHeight << 4;
    if (g_nMpegCodedWidth == decoder->width && g_nMpegCodedHeight == decoder->height) {
        return decoder->height;
    }
    decoder->height = g_nMpegCodedHeight;
    decoder->width = g_nMpegCodedWidth;
    g_nMpegChromaWidth = g_nMpegCodedWidth >> 1;
    g_nMpegChromaHeight = g_nMpegCodedHeight >> 1;
    sceMpegRewindWritePointer(&work->mRing);
    // A macroblock stores 384 bytes for each 256 pixels.
    frameBytes = (int)((unsigned int)(g_nMpegCodedWidth * (g_nMpegCodedHeight * kMacroblockBytes)) >> 8);
    work->mFirstFrameBuffer =
        (int)(uintptr_t)sceMpegCheckWorkAreaSize(&work->mRing, frameBytes, 0x40);
    work->mSecondFrameBuffer =
        (int)(uintptr_t)sceMpegCheckWorkAreaSize(&work->mRing, frameBytes, 0x40);
    work->mThirdFrameBuffer =
        (int)(uintptr_t)sceMpegCheckWorkAreaSize(&work->mRing, frameBytes, 0x40);
    sceMpegAssignFrameBuffers(&g_mpegSeqAreas[0],
                        &g_mpegSeqAreas[1],
                        &g_mpegSeqAreas[2],
                        &g_mpegSeqAreas[3],
                        &g_mpegSeqAreas[4],
                        &g_mpegSeqAreas[5],
                        &g_mpegSeqAreas[6],
                        &g_mpegSeqAreas[7],
                        &g_mpegSeqAreas[8],
                        work->mFirstFrameBuffer,
                        work->mSecondFrameBuffer,
                        work->mThirdFrameBuffer);
    sceMpegSetTableSize(&g_mpegSeqAreas[0], g_nMpegCodedWidth, g_nMpegCodedHeight);
    sceMpegSetTableSize(&g_mpegSeqAreas[1], g_nMpegCodedWidth, g_nMpegCodedHeight);
    sceMpegSetTableSize(&g_mpegSeqAreas[2], g_nMpegCodedWidth, g_nMpegCodedHeight);
    half = g_nMpegCodedHeight / 2;
    sceMpegSetTableSize(&g_mpegSeqAreas[3], g_nMpegCodedWidth, half);
    sceMpegSetTableSize(&g_mpegSeqAreas[4], g_nMpegCodedWidth, half);
    sceMpegSetTableSize(&g_mpegSeqAreas[5], g_nMpegCodedWidth, half);
    sceMpegSetTableSize(&g_mpegSeqAreas[6], g_nMpegCodedWidth, half);
    sceMpegSetTableSize(&g_mpegSeqAreas[7], g_nMpegCodedWidth, half);
    return sceMpegSetTableSize(&g_mpegSeqAreas[8], g_nMpegCodedWidth, half);
}

// 0x0060e668
// The sequence extension. The size, bit rate, and buffer size extensions are folded into the
// sequence header's fields.
int sceMpegSequenceExtension(void) {
    unsigned int bits;
    unsigned int bitRateExt;
    unsigned int horizontalExt;
    unsigned int verticalExt;
    unsigned int bufferExt;

    g_nMpegIsMpeg2 = 1;
    sceIpuSetMpeg1Mode(0);
    bits = (unsigned int)sceMpegGetBits(0x1c);
    bitRateExt = (bits >> 1) & 0xfff;
    horizontalExt = (bits >> 15) & 3;
    verticalExt = (bits >> 13) & 3;
    g_nMpegChromaFormat = (int)((bits >> 17) & 3);
    if (g_nMpegChromaFormat != 1) {
        sceMpegRaiseError("_chroma_format needs to be 1: 420");
    }
    g_nMpegProfileAndLevel = (int)(bits >> 20);
    g_nMpegProgressiveSequence = (int)((bits >> 19) & 1);
    bits = (unsigned int)sceMpegGetBits(0x10);
    g_nMpegFrameRateExtensionD = (int)(bits & 0x1f);
    g_nMpegFrameRateExtensionN = (int)((bits >> 5) & 3);
    g_nMpegLowDelay = (int)((bits >> 7) & 1);
    bufferExt = bits >> 8;
    if (g_nMpegProfileAndLevel != 0x48 && g_nMpegProfileAndLevel != 0x58) {
        sceMpegRaiseError("Unsupported profile/level");
    }
    g_nMpegHorizontalSize = (int)((horizontalExt << 12) | ((unsigned int)g_nMpegHorizontalSize & 0xfff));
    g_nMpegVerticalSize = (int)((verticalExt << 12) | ((unsigned int)g_nMpegVerticalSize & 0xfff));
    g_nMpegBitRate += (int)(bitRateExt << 18);
    g_nMpegVbvBufferSize += (int)(bufferExt << 10);
    return g_nMpegVerticalSize;
}

// 0x0060e7d0
// The sequence display extension.
int sceMpegSequenceDisplayExtension(void) {
    g_nMpegVideoFormat = sceMpegGetBits(3);
    g_nMpegColourDescription = sceMpegGetBits(1);
    if (g_nMpegColourDescription != 0) {
        g_nMpegColourPrimaries = sceMpegGetBits(8);
        g_nMpegTransferCharacteristics = sceMpegGetBits(8);
        g_nMpegMatrixCoefficients = sceMpegGetBits(8);
    }
    g_nMpegDisplayHorizontalSize = sceMpegGetBits(14);
    sceMpegGetBits(1); // The marker bit.
    g_nMpegDisplayVerticalSize = sceMpegGetBits(14);
    return g_nMpegDisplayVerticalSize;
}

// 0x0060c138
// The quantiser matrix extension. A loaded matrix goes straight to the IPU, and the chroma
// matrices of the 4:2:2 and 4:4:4 formats are reported as unsupported.
int sceMpegQuantMatrixExtension(void) {
    g_nMpegLoadIntraQuantiserMatrix = sceMpegGetBits(1);
    if (g_nMpegLoadIntraQuantiserMatrix != 0) {
        sceMpegWaitIpuIdle();
        sceMpegIssueIpuCommand(0x50000000u);
        sceMpegWaitIpuIdle();
    }
    g_nMpegLoadNonIntraQuantiserMatrix = sceMpegGetBits(1);
    if (g_nMpegLoadNonIntraQuantiserMatrix != 0) {
        sceMpegWaitIpuIdle();
        sceMpegIssueIpuCommand(0x58000000u);
        sceMpegWaitIpuIdle();
    }
    if (sceMpegGetBits(1) != 0) {
        sceMpegRaiseError("load_chroma_intra_quantizer_matrix == 1");
    }
    if (sceMpegGetBits(1) != 0) {
        sceMpegRaiseError("load_chroma_non_intra_quantizer_matrix == 1");
    }
    return 0;
}

// 0x0060c2d8
// The copyright extension.
int sceMpegCopyrightExtension(void) {
    g_nMpegCopyrightFlag = sceMpegGetBits(1);
    g_nMpegCopyrightIdentifier = sceMpegGetBits(8);
    g_nMpegOriginalOrCopy = sceMpegGetBits(1);
    sceMpegGetBits(7); // Reserved bits.
    sceMpegGetBits(1); // The marker bit.
    g_nMpegCopyrightNumber1 = sceMpegGetBits(0x14);
    sceMpegGetBits(1); // The marker bit.
    g_nMpegCopyrightNumber2 = sceMpegGetBits(0x16);
    sceMpegGetBits(1); // The marker bit.
    g_nMpegCopyrightNumber3 = sceMpegGetBits(0x16);
    return 0;
}

// 0x0060c1e8
// The picture display extension. The number of frame centre offsets follows from the sequence and
// picture flags.
int sceMpegPictureDisplayExtension(void) {
    int count;
    int i;

    if (g_nMpegProgressiveSequence != 0) {
        if (g_nMpegRepeatFirstField == 0) {
            count = 1;
        } else {
            count = g_nMpegTopFieldFirst != 0 ? 3 : 2;
        }
    } else if (g_nMpegPictureStructure != 3) {
        count = 1;
    } else {
        count = g_nMpegRepeatFirstField != 0 ? 3 : 2;
    }
    for (i = 0; i < count; ++i) {
        g_anMpegFrameCentreHorizontalOffsets[i] = sceMpegGetBits(0x10);
        sceMpegGetBits(1); // The marker bit.
        g_anMpegFrameCentreVerticalOffsets[i] = sceMpegGetBits(0x10);
        sceMpegGetBits(1); // The marker bit.
    }
    return 0;
}

// Replace the field of the IPU control register that starts at nShift and is nWidth bits wide.
static void sceMpegSetIpuControlField(int nShift, unsigned int nWidthMask, unsigned int nValue) {
    volatile unsigned int *pControl = (volatile unsigned int *)(uintptr_t)kIpuControlAddress;
    *pControl = (*pControl & ~(nWidthMask << nShift)) | (nValue << nShift);
}

// 0x0060bd08
// The picture coding extension. The DC precision and three coding flags are also mirrored into the
// IPU control register.
int sceMpegPictureCodingExtension(void) {
    sceMpeg *decoder = (sceMpeg *)g_decoderInstance;
    MpegWork *work = (MpegWork *)decoder->pContext;
    int i;

    for (i = 0; i < 4; ++i) {
        g_anMpegFCodes[i] = sceMpegGetBits(4);
    }
    g_nMpegIntraDcPrecision = sceMpegGetBits(2);
    sceMpegSetIpuControlField(16, 3u, (unsigned int)g_nMpegIntraDcPrecision);
    g_nMpegPictureStructure = sceMpegGetBits(2);
    if (work->mFirstFieldStructure == 0) {
        work->mFirstFieldStructure = g_nMpegPictureStructure;
    }
    g_nMpegTopFieldFirst = sceMpegGetBits(1);
    g_nMpegFramePredFrameDct = sceMpegGetBits(1);
    g_nMpegConcealmentMotionVectors = sceMpegGetBits(1);
    g_nMpegQScaleType = sceMpegGetBits(1);
    sceMpegSetIpuControlField(22, 1u, (unsigned int)g_nMpegQScaleType);
    g_nMpegIntraVlcFormat = sceMpegGetBits(1);
    sceMpegSetIpuControlField(21, 1u, (unsigned int)g_nMpegIntraVlcFormat);
    g_nMpegAlternateScan = sceMpegGetBits(1);
    sceMpegSetIpuControlField(20, 1u, (unsigned int)g_nMpegAlternateScan);
    g_nMpegRepeatFirstField = sceMpegGetBits(1);
    g_nMpegChroma420Type = sceMpegGetBits(1);
    g_nMpegProgressiveFrame = sceMpegGetBits(1);
    g_nMpegCompositeDisplayFlag = sceMpegGetBits(1);
    if (g_nMpegCompositeDisplayFlag != 0) {
        g_nMpegVAxis = sceMpegGetBits(1);
        g_nMpegFieldSequence = sceMpegGetBits(3);
        g_nMpegSubCarrier = sceMpegGetBits(1);
        g_nMpegBurstAmplitude = sceMpegGetBits(7);
        g_nMpegSubCarrierPhase = sceMpegGetBits(8);
        return g_nMpegSubCarrierPhase;
    }
    return 0;
}

// 0x0060e870
int sceMpegSequenceScalableExtension(void) {
    sceMpegRaiseError("_sequenceScalableExtension() is not implemented");
    return 0;
}

// 0x0060e880
int sceMpegReservedExtension(void) {
    sceMpegRaiseError("Unknown Extension");
    return 0;
}

// 0x0060e890
int sceMpegPictureSpatialScalableExtension(void) {
    sceMpegRaiseError("_pictureSpatialScalableExtension is not supported");
    return 0;
}

// 0x0060e8a0
int sceMpegPictureTemporalScalableExtension(void) {
    sceMpegRaiseError("_pictureTemporalScalableExtension is not supported");
    return 0;
}

// Issues an IPU command word and returns the nibble table word its top nibble selects.
// 0x0060b290
static int sceMpegIssueIpuCommand(unsigned int nCommand) {
    unsigned int index;
    int value;

    *(volatile unsigned int *)(uintptr_t)kIpuCommandAddress = nCommand;
    index = (nCommand >> 28) & 0xfu;
    value = (int)g_mpegNibbleTable[index];
    g_mpegIpuBusyFlag = value;
    return value;
}

// 0x0060e5a8
static int sceMpegLoadDefaultMatrix(unsigned int nCommand, const unsigned char *pMatrix) {
    StreamEntry entry;
    volatile unsigned int *pData;
    volatile unsigned int *pGifA;
    volatile unsigned int *pGifB;

    entry.key = 2;
    entry.templateBits = 0;
    entry.callback = NULL;
    entry.data = NULL;
    sceMpegInvokeCallbackSlot(g_decoderInstance, &entry);
    sceMpegWaitIpuIdle();
    pData = (volatile unsigned int *)(uintptr_t)kIpuCommandAddress;
    *pData = 0u; // Clears the IPU input FIFO.
    sceMpegWaitIpuIdle();
    pGifA = (volatile unsigned int *)(uintptr_t)0x1000b410;
    *pGifA = (unsigned int)(uintptr_t)pMatrix & 0x0fffffffu;
    pGifB = (volatile unsigned int *)(uintptr_t)0x1000b420;
    *pGifB = 4u;
    // Start the IPU input channel on the matrix.
    *(volatile unsigned int *)(uintptr_t)0x1000b400 = 0x101u;
    sceMpegIssueIpuCommand(nCommand);
    sceMpegWaitIpuIdle();
    entry.key = 3;
    return sceMpegInvokeCallbackSlot(g_decoderInstance, &entry);
}

// Sets picture dimensions into a sequence table and reports one.
// 0x0060e000
static int sceMpegSetTableSize(MpegSeqTable *pTable, int nWidth, int nHeight) {
    pTable->mMbWidth = nWidth >> 4;
    pTable->mMbHeight = nHeight >> 4;
    pTable->mWidth = nWidth;
    pTable->mHeight = nHeight;
    return 1;
}

// 0x0060e4c0
static void sceMpegAssignFrameBuffers(MpegSeqTable *pFrameForward,
                                      MpegSeqTable *pFrameBackward,
                                      MpegSeqTable *pFrameBidirectional,
                                      MpegSeqTable *pTopForward,
                                      MpegSeqTable *pTopBackward,
                                      MpegSeqTable *pTopBidirectional,
                                      MpegSeqTable *pBottomForward,
                                      MpegSeqTable *pBottomBackward,
                                      MpegSeqTable *pBottomBidirectional,
                                      int nForwardBuffer,
                                      int nBackwardBuffer,
                                      int nBidirectionalBuffer) {
    // A field starts half a frame of macroblocks into its frame buffer.
    const int fieldBytes = g_nMpegCodedWidth * g_nMpegCodedHeight / (kMacroblockPixels * 2) * kMacroblockBytes;
    const int forward =
        (int)(((unsigned int)nForwardBuffer & kPhysicalAddressMask) | kUncachedSegment);
    const int backward =
        (int)(((unsigned int)nBackwardBuffer & kPhysicalAddressMask) | kUncachedSegment);
    const int bidirectional =
        (int)(((unsigned int)nBidirectionalBuffer & kPhysicalAddressMask) | kUncachedSegment);

    pFrameForward->mBuffer = forward;
    pFrameBackward->mBuffer = backward;
    pFrameBidirectional->mBuffer = bidirectional;
    pTopForward->mBuffer = forward;
    pTopBackward->mBuffer = backward;
    pTopBidirectional->mBuffer = bidirectional;
    pBottomForward->mBuffer = (int)(((unsigned int)(fieldBytes + nForwardBuffer) &
                                     kPhysicalAddressMask) |
                                    kUncachedSegment);
    pBottomBackward->mBuffer = (int)(((unsigned int)(fieldBytes + nBackwardBuffer) &
                                      kPhysicalAddressMask) |
                                     kUncachedSegment);
    pBottomBidirectional->mBuffer = (int)(((unsigned int)(fieldBytes + nBidirectionalBuffer) &
                                           kPhysicalAddressMask) |
                                          kUncachedSegment);
}

// 0x0060b708
static int sceMpegSkipBits(int nBits) {
    volatile unsigned int *pControl;
    volatile unsigned int *pData;
    unsigned int count;
    unsigned int index;
    int result;

    pControl = (volatile unsigned int *)(uintptr_t)kIpuControlAddress;
    pData = (volatile unsigned int *)(uintptr_t)kIpuCommandAddress;
    if ((*pControl & kIpuBusyMask) == kIpuBusyValue) {
        count = 0;
        for (;;) {
            if (count >= kIpuWatchdogLimit) {
                sceMpegReportNoData(g_decoderInstance);
                count = 0;
            } else {
                ++count;
            }
            if ((*pControl & kIpuBusyMask) != kIpuBusyValue) {
                break;
            }
        }
    }
    *pData = (unsigned int)nBits | 0x40000000u;
    index = (((unsigned int)nBits | 0x40000000u) >> 28) & 0xfu;
    g_mpegIpuBusyFlag = (int)g_mpegNibbleTable[index];
    result = sceMpegReadIpuData();
    g_mpegShiftAccum = result;
    g_mpegShiftBudget = 0x20;
    return result;
}

// 0x0060b5d0
static int sceMpegPeekBits(int nBits) {
    volatile unsigned int *pControl;
    volatile unsigned int *pData;
    unsigned int count;

    pControl = (volatile unsigned int *)(uintptr_t)kIpuControlAddress;
    pData = (volatile unsigned int *)(uintptr_t)kIpuCommandAddress;
    if (g_mpegIpuBusyFlag != 0 || g_mpegShiftBudget < nBits) {
        count = 0;
        for (;;) {
            if (count >= kIpuWatchdogLimit) {
                sceMpegReportNoData(g_decoderInstance);
                count = 0;
            } else {
                ++count;
            }
            if ((*pControl & kIpuBusyMask) != kIpuBusyValue) {
                break;
            }
        }
        *pData = 0x40000000u;
        g_mpegIpuBusyFlag = (int)g_mpegNibbleTable[4];
        g_mpegShiftAccum = sceMpegReadIpuData();
        g_mpegShiftBudget = 0x20;
    }
    // The shift is a variable rotate the hardware masks to five bits.
    return (int)((unsigned int)g_mpegShiftAccum >> ((0 - nBits) & 31));
}

// 0x0060b988
static int sceMpegNextStartCode(void) {
    volatile unsigned int *pStatus;
    unsigned int arg;
    int result;

    sceMpegWaitIpuIdle();
    pStatus = (volatile unsigned int *)(uintptr_t)0x10002020;
    arg = (0u - (*pStatus & 7u)) & 7u;
    if (arg != 0) {
        sceMpegSkipBits((int)arg);
    }
    for (;;) {
        result = sceMpegPeekBits(0x18);
        if (result == 1) {
            return result;
        }
        sceMpegSkipBits(8);
    }
}

// 0x005e0a08
static int sceMpegReportNoData(void *pDecoder) {
    StreamEntry entry;

    // The image stores only the low key word and leaves the rest of the stack entry as garbage.
    // The reconstruction zeroes it instead, which no observed reader distinguishes.
    entry.key = 1;
    entry.templateBits = 0;
    entry.callback = NULL;
    entry.data = NULL;
    return sceMpegInvokeCallbackSlot(pDecoder, &entry);
}

// 0x0060b2c0
static int sceMpegWaitIpuIdle(void) {
    volatile unsigned int *pControl;
    unsigned int count;

    pControl = (volatile unsigned int *)(uintptr_t)kIpuControlAddress;
    count = 0;
    if ((*pControl & kIpuBusyMask) != kIpuBusyValue) {
        return (int)kIpuBusyValue; // The image returns the mask word still in the result register.
    }
    do {
        if (count >= kIpuWatchdogLimit) {
            sceMpegReportNoData(g_decoderInstance);
            count = 0;
        } else {
            ++count;
        }
    } while ((*pControl & kIpuBusyMask) == kIpuBusyValue);
    return (int)count;
}

// 0x0060b368
static int sceMpegReadIpuData(void) {
    volatile unsigned long long *pData;
    volatile unsigned int *pControl;
    long long value;
    unsigned int count;

    pData = (volatile unsigned long long *)(uintptr_t)kIpuCommandAddress;
    pControl = (volatile unsigned int *)(uintptr_t)kIpuControlAddress;
    value = (long long)*pData;
    count = 0;
    for (;;) {
        if (value >= 0) {
            return (int)value;
        }
        if ((*pControl & kIpuDataReadyBit) != 0) {
            return (int)value;
        }
        if (count >= kIpuWatchdogLimit) {
            sceMpegReportNoData(g_decoderInstance);
            count = 0;
        } else {
            ++count;
        }
        value = (long long)*pData;
    }
}

// The decoder instance address retained for the interrupt handlers.
// 0x007a38bc
static void *g_decoderInstance;
// The count of pictures decoded since sceMpegReset() last reset it. The frame count is its
// distance from the work area's base count.
// 0x007a2c04
static int g_mpegPictureCounter;
// Set once a decoded picture is ready for output. The picture path loops until it is set.
// 0x007a3380
static int g_pictureWaitFlag;

// The default callback pointers installed for slots two and three, standing in for the words
// near 0x0062dd58. Their values are not yet recovered.
static void *g_defaultSlotTwo;
static void *g_defaultSlotThree;

// 0x007798b8
// Key and match mask for each stream type. A packet belongs to a stream when its key, masked,
// equals the stream key. The mask also selects where the channel number goes.
static const unsigned long long g_streamTemplates[10][2] = {
    { 0xe000000000ULL, 0xff00000000ULL }, // MPEG-2 video, channel in the stream identifier.
    { 0xbdffc00000ULL, 0xffffffffffULL },
    { 0xbdffa00000ULL, 0xffffffffffULL }, // PCM audio.
    { 0xbdffa10000ULL, 0xffffffffffULL },
    { 0xbdff900000ULL, 0xffffffffffULL },
    { 0xc000000000ULL, 0xff00000000ULL },
    { 0xbd80000000ULL, 0xffff000000ULL },
    { 0xbda0000000ULL, 0xffff000000ULL },
    { 0xbd88000000ULL, 0xffff000000ULL },
    { 0xbd90000000ULL, 0xffff000000ULL },
};

// 0x005e0ad8
void sceMpegResetRingPointers(void *pRing, void *pBase, int nSize) {
    MpegRing *ring;

    ring = (MpegRing *)pRing;
    ring->mCommit = (int)(uintptr_t)pBase;
    ring->mSize = nSize;
    ring->mBase = (int)(uintptr_t)pBase;
    ring->mWrite = (int)(uintptr_t)pBase;
}

// 0x005e0af0
int sceMpegCommitWritePointer(void *pRing) {
    MpegRing *ring;
    int value;

    ring = (MpegRing *)pRing;
    value = ring->mWrite;
    ring->mCommit = value;
    return value;
}

// 0x005e0b00
void sceMpegRewindWritePointer(void *pRing) {
    MpegRing *ring;
    int value;

    ring = (MpegRing *)pRing;
    value = ring->mCommit;
    ring->mWrite = value;
}

// 0x005e0b10
void *sceMpegCheckWorkAreaSize(void *pRing, int nNeed, int nAlign) {
    MpegRing *ring;
    unsigned int write;
    unsigned int size;
    unsigned int base;
    unsigned int aligned;
    unsigned int end;
    unsigned int needed;

    ring = (MpegRing *)pRing;
    write = (unsigned int)ring->mWrite;
    size = (unsigned int)ring->mSize;
    base = (unsigned int)ring->mBase;
    aligned = ((write + (unsigned int)nAlign) - 1u) / (unsigned int)nAlign;
    end = base + size;
    aligned = aligned * (unsigned int)nAlign;
    needed = aligned + (unsigned int)nNeed;
    if (end < needed) {
        sceMpegRaiseError("work area size is too small");
        return NULL;
    }
    ring->mWrite = (int)needed;
    return (void *)(uintptr_t)aligned;
}

// 0x0060ded0
void sceMpegRaiseError(const char *pFormat) {
    sceMpeg *decoder;
    MpegWork *work;
    StreamEntry entry;

    decoder = (sceMpeg *)g_decoderInstance;
    if (decoder == NULL) {
        sceMpegPrintErrorLine(pFormat);
        return;
    }
    work = (MpegWork *)decoder->pContext;
    if (work == NULL || work->mSlots[0].callback == NULL) {
        sceMpegPrintErrorLine(pFormat);
        return;
    }
    // The low word carries zero and the high word carries the message, which the slot callback
    // reads back from the entry. The remaining words stay uninitialised, as they do in the image.
    entry.key = (unsigned long long)(unsigned int)(uintptr_t)pFormat << 32;
    sceMpegInvokeCallbackSlot(decoder, &entry);
}

// 0x0060de90
void sceMpegPrintErrorLine(const char *pMessage) {
    LogPrintf("[MPEG ERROR]%s\n", pMessage);
}

// 0x0060dea0
void sceMpegReportErrorFormatted(const char *pFormat, ...) {
    char buffer[kErrorMessageSize];
    va_list args;

    va_start(args, pFormat);
    vsprintf(buffer, pFormat, args);
    va_end(args);
    sceMpegRaiseError(buffer);
}

// 0x0060a038
int sceIpuSetMpeg1Mode(int nMpeg1) {
    volatile unsigned int *pControl;
    unsigned int value;

    pControl = (volatile unsigned int *)(uintptr_t)0x10002010;
    value = (*pControl & 0xff7fffffu) | ((unsigned int)nMpeg1 << 23);
    // The store falls in the return delay slot, so it lands before the return either way.
    *pControl = value;
    return (int)value;
}

// 0x00637178
int sceIpuSync(int nMode) {
    volatile unsigned int *pControl;

    pControl = (volatile unsigned int *)(uintptr_t)0x10002010;
    if (nMode == 0) {
        while ((int)*pControl < 0) {
        }
        return 0;
    }
    if (nMode == 1) {
        return (int)(*pControl >> 31);
    }
    return 0;
}

// 0x0060dd78
void sceMpegResetMcBuffers(void) {
    int base;

    sceIpuSetMpeg1Mode(1);
    base = g_mpegIpuBase;
    g_mpegIpuTable.mBuffers[0].mStaging = base;
    g_mpegIpuTable.mBuffers[0].mCoefficients = base + kMcFirstCoefficientsOffset;
    g_mpegIpuTable.mBuffers[1].mStaging = base + kMcSecondStagingOffset;
    g_mpegIpuTable.mBuffers[1].mCoefficients = base + kMcSecondCoefficientsOffset;
    g_mpegIpuTable.mCurrent = 0;
}

// 0x0060ddc8
void sceMpegResetIpuChannels(void *pDecoder) {
    volatile unsigned int *pStatus;
    volatile unsigned int *pStatusSet;
    volatile unsigned int *pMaskA;
    volatile unsigned int *pMaskB;
    volatile unsigned int *pClearC;
    unsigned int value;

    (void)pDecoder;
    g_nMpegResetDcPredictor = 0;
    DIntr();
    // The enable store falls in the disable call delay slot, so it lands first.
    g_mpegIpuBusyFlag = 1;
    pStatus = (volatile unsigned int *)(uintptr_t)0x1000f520;
    pStatusSet = (volatile unsigned int *)(uintptr_t)0x1000f590;
    value = *pStatus;
    value = value | 0x00010000u;
    *pStatusSet = value;
    pMaskA = (volatile unsigned int *)(uintptr_t)0x1000b000;
    *pMaskA = 0;
    pMaskB = (volatile unsigned int *)(uintptr_t)0x1000b400;
    *pMaskB = 0;
    pClearC = (volatile unsigned int *)(uintptr_t)0x1000d400;
    *pClearC = 0;
    value = *pStatus;
    value = value & 0xfffeffffu;
    EIntr();
    // The clear store falls in the reenable call delay slot, so it lands first.
    *pStatusSet = value;
    pMaskA = (volatile unsigned int *)(uintptr_t)0x1000b020;
    *pMaskA = 0;
    pMaskB = (volatile unsigned int *)(uintptr_t)0x1000b420;
    *pMaskB = 0;
    pClearC = (volatile unsigned int *)(uintptr_t)0x1000d420;
    *pClearC = 0;
    *(volatile int *)(uintptr_t)0x10002010 = 0x40000000;
    sceIpuSync(0);
}

// 0x0060dce0
void sceMpegSelectMpeg1(void) {
    g_nMpegIsMpeg2 = 0;
    sceIpuSetMpeg1Mode(1);
}

// 0x0061d9d8
int sceIpuWriteToChannelControl(int nChcr) {
    volatile unsigned int *pStatus;
    volatile unsigned int *pStatusSet;
    volatile unsigned int *pClear;
    unsigned int value;

    DIntr();
    pStatus = (volatile unsigned int *)(uintptr_t)0x1000f520;
    pStatusSet = (volatile unsigned int *)(uintptr_t)0x1000f590;
    value = *pStatus;
    value = value | 0x00010000u;
    *pStatusSet = value;
    pClear = (volatile unsigned int *)(uintptr_t)0x1000b400;
    *pClear = (unsigned int)nChcr;
    value = *pStatus;
    value = value & 0xfffeffffu;
    *pStatusSet = value;
    return (int)EIntr();
}

// One IPU input-FIFO quadword, addressable as bytes.
typedef union {
    unsigned char mBytes[16];
    u_long128 mQuad;
} IpuQuad;

// 0x007a5ae0
// The intra quantiser matrix, four quadwords, then the flat non-intra row the initialiser sends
// four times.
static const IpuQuad kIpuQuantRows[5] = {
    {{0x08, 0x10, 0x10, 0x13, 0x10, 0x13, 0x16, 0x16,
      0x16, 0x16, 0x16, 0x16, 0x1a, 0x18, 0x1a, 0x1b}},
    {{0x1b, 0x1b, 0x1a, 0x1a, 0x1a, 0x1a, 0x1b, 0x1b,
      0x1b, 0x1d, 0x1d, 0x1d, 0x22, 0x22, 0x22, 0x1d}},
    {{0x1d, 0x1d, 0x1b, 0x1b, 0x1d, 0x1d, 0x20, 0x20,
      0x22, 0x22, 0x25, 0x26, 0x25, 0x23, 0x23, 0x22}},
    {{0x23, 0x26, 0x26, 0x28, 0x28, 0x28, 0x30, 0x30,
      0x2e, 0x2e, 0x38, 0x38, 0x3a, 0x45, 0x45, 0x53}},
    {{0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
      0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10}},
};

// 0x007a5b30
// The colour lookup table for vector quantisation.
static const IpuQuad kIpuVqTable[2] = {
    {{0x00, 0x00, 0x21, 0x04, 0x42, 0x08, 0xe0, 0x03,
      0x84, 0x10, 0xa5, 0x14, 0xc6, 0x18, 0xe7, 0x1c}},
    {{0x1f, 0x00, 0x29, 0x25, 0x4a, 0x29, 0x00, 0x7c,
      0x8c, 0x31, 0xad, 0x35, 0xff, 0x7f, 0xce, 0x39}},
};

enum {
    kIpuCmdBclr = 0x00000000,
    kIpuCmdSetIqIntra = 0x50000000,
    kIpuCmdSetIqNonIntra = 0x58000000,
    kIpuCmdSetVq = 0x60000000,
    kIpuCmdSetTh = (int)0x90000000u,
    kIpuCtrlReset = 0x40000000,
    kIpuQuantRowsPerMatrix = 4,
};

#define IPU_CMD_REG (*(volatile unsigned int *)(uintptr_t)0x10002000)
#define IPU_CTRL_REG (*(volatile unsigned int *)(uintptr_t)0x10002010)
#define IPU_IN_FIFO ((volatile u_long128 *)(uintptr_t)0x10007010)

// Waits until the IPU is idle and returns the control word that ended the wait.
static inline int IpuWaitIdle(void) {
    int value;

    do {
        value = (int)IPU_CTRL_REG;
    } while (value < 0);
    return value;
}

// 0x0061da40
// Resets the IPU and loads both quantiser matrices, the colour lookup table, and the threshold.
int sceIpuResetAndLoadTables(void) {
    int i;

    sceIpuWriteToChannelControl(1);
    IPU_CTRL_REG = kIpuCtrlReset;
    (void)IpuWaitIdle();
    IPU_CMD_REG = kIpuCmdBclr;
    (void)IpuWaitIdle();
    for (i = 0; i < kIpuQuantRowsPerMatrix; ++i) {
        ee_store_quadword(IPU_IN_FIFO, kIpuQuantRows[i].mQuad);
    }
    for (i = 0; i < kIpuQuantRowsPerMatrix; ++i) {
        ee_store_quadword(IPU_IN_FIFO, kIpuQuantRows[kIpuQuantRowsPerMatrix].mQuad);
    }
    IPU_CMD_REG = kIpuCmdSetIqIntra;
    (void)IpuWaitIdle();
    IPU_CMD_REG = kIpuCmdSetIqNonIntra;
    (void)IpuWaitIdle();
    ee_store_quadword(IPU_IN_FIFO, kIpuVqTable[0].mQuad);
    ee_store_quadword(IPU_IN_FIFO, kIpuVqTable[1].mQuad);
    IPU_CMD_REG = kIpuCmdSetVq;
    (void)IpuWaitIdle();
    IPU_CMD_REG = (unsigned int)kIpuCmdSetTh;
    (void)IpuWaitIdle();
    IPU_CTRL_REG = kIpuCtrlReset;
    (void)IpuWaitIdle();
    IPU_CMD_REG = kIpuCmdBclr;
    return IpuWaitIdle();
}

// 0x005caa58
int sceMpegDemuxPss(sceMpeg *pMpeg, unsigned char *pStart, int nSize) {
    return sceMpegDemuxPssRing(pMpeg, pStart, nSize, NULL, -1);
}

// 0x005ca4e0
static unsigned long long buildStreamKey(int nType, int nChannel) {
    static const unsigned long long kSubstreamByteMask = 0xffffULL << 24;
    static const unsigned long long kStreamIdMask = 0xff00ULL << 24;
    unsigned long long mask;
    int shift;

    if ((unsigned int)nType >= 10u) {
        return 0;
    }
    mask = g_streamTemplates[nType][1];
    if (mask == kSubstreamByteMask) {
        shift = 24;
    } else if (mask > kSubstreamByteMask) {
        shift = 0;
    } else if (mask == kStreamIdMask) {
        shift = 32;
    } else {
        shift = 0;
    }
    return g_streamTemplates[nType][0] | ((unsigned long long)(long long)nChannel << shift);
}

// 0x005e08e8
void sceMpegReset(void *pDecoder) {
    sceMpeg *decoder;
    MpegWork *work;

    decoder = (sceMpeg *)pDecoder;
    work = (MpegWork *)decoder->pContext;
    work->mCompleted = 0;
    work->mPictureIndex = 0;
    work->mOutputState = 0;
    decoder->frameCount = 0;
    work->mLastPts = -1;
    // The work word at +0xac falls in the call delay slot, so it lands before the drain body.
    work->mFrameCountBase = 0;
    sceMpegResetIpuChannels(pDecoder);
    g_mpegPictureCounter = 0;
    sceMpegSelectMpeg1();
}

// Flag tables visited in binary order. Only six of the nine table slots participate.
static const int kPendingTableOrder[6] = { 0, 3, 6, 1, 4, 7 };

// 0x005e0928
int sceMpegClearRefBuff(void *pDecoder) {
    int i;
    int index;

    (void)pDecoder;
    for (i = 0; i < 6; ++i) {
        index = kPendingTableOrder[i];
        if (g_mpegTables[index] != NULL) {
            g_mpegTables[index]->mDecoded = 0;
        }
    }
    return 1;
}

// 0x005e0490
int sceMpegInit(void) {
    volatile unsigned int *pStatus;
    volatile unsigned int *pStatusSet;
    volatile unsigned int *pMaskA;
    volatile unsigned int *pMaskB;
    volatile unsigned int *pClearA;
    volatile unsigned int *pClearB;
    unsigned int value;

    DIntr();
    pStatus = (volatile unsigned int *)(uintptr_t)0x1000f520;
    pStatusSet = (volatile unsigned int *)(uintptr_t)0x1000f590;
    value = *pStatus;
    value = value | 0x00010000u;
    *pStatusSet = value;
    pMaskA = (volatile unsigned int *)(uintptr_t)0x1000b000;
    value = *pMaskA;
    value = value & 0xfffffeffu;
    *pMaskA = value;
    pMaskB = (volatile unsigned int *)(uintptr_t)0x1000b400;
    value = *pMaskB;
    value = value & 0xfffffeffu;
    *pMaskB = value;
    value = *pStatus;
    value = value & 0xfffeffffu;
    *pStatusSet = value;
    EIntr();
    pClearA = (volatile unsigned int *)(uintptr_t)0x1000b020;
    *pClearA = 0;
    pClearB = (volatile unsigned int *)(uintptr_t)0x1000b420;
    *pClearB = 0;
    return sceIpuResetAndLoadTables();
}

// Bit reader over the input, 0x30 bytes. The cache has the next bits left-aligned, and each
// advance refills it past 56 valid bits. A read of up to 32 bits therefore never needs a refill. The fetch pointer wraps
// from the ring end back to its base.
typedef struct {
    unsigned long long mCache;
    unsigned char *mStart;
    unsigned char *mFetch;
    int mCached;
    unsigned long long mPosition; // Bits consumed since mStart.
    unsigned char *mRingBase;
    uintptr_t mRingEnd; // All ones without a ring, which the fetch pointer never arrives at.
    int mRingSize;
} BitReader;

// The pack header fields the demultiplexer stores, 0x10 bytes.
typedef struct {
    int mScrExtension;
    unsigned int mScrLow; // System clock reference bits 31 to 0.
    int mScrHigh; // System clock reference bit 32.
    int mHasSystemHeader;
} PackHeader;

// One parsed packet, 0x2c bytes. The key is the stream identifier shifted up by 32 bits, with the
// substream word of a private stream in the low word. Positions are bit positions from the start
// of the input.
typedef struct {
    unsigned long long mKey;
    int mPacketLength;
    int mScrambling;
    long long mPts;
    long long mDts;
    int mDataPosition;
    int mDataLength;
    int mHeaderPosition;
} PesPacket;

enum {
    kCacheRefillBits = 57,
    kCacheTopShift = 56,
    kPacketStartPrefix = 0x000001,
    kPackStartCode = 0x000001ba,
    kSystemHeaderStartCode = 0x000001bb,
    kProgramEndCode = 0x000001b9,
    kStreamIdProgramStreamMap = 0xbc,
    kStreamIdPrivate1 = 0xbd,
    kStreamIdPadding = 0xbe,
    kStreamIdPrivate2 = 0xbf,
    kStreamIdEcm = 0xf0,
    kStreamIdEmm = 0xf1,
    kStreamIdDsmcc = 0xf2,
    kStreamIdH2221TypeE = 0xf8,
    kStreamIdDirectory = 0xff,
    kPtsFlag = 2,
    kPtsDtsFlags = 3,
    kSubstreamHeaderBytes = 4,
    // The packet length counts the three bytes of flags and header length ahead of the header data.
    kPesFlagBytes = 3,
    kPrivateDataBits = 128
};

// Stream key the demultiplexer falls back to when no registered stream matches a packet.
static const unsigned long long kDefaultStreamKey = 0xbdffULL << 24;

// 0x00779958
// Bits the optional PES fields occupy for each combination of the ES rate, trick mode, copy
// information, and CRC flags.
static const unsigned char kOptionalFieldBits[] = {
    0, 16, 8, 24, 8, 24, 16, 32, 24, 40, 32, 48, 32, 48, 40, 56 };

static unsigned long long streamKey(int nStreamId) {
    return (unsigned long long)(unsigned int)nStreamId << 32;
}

// 0x006102c0
static void bitReaderAdvance(BitReader *pReader, int nBits) {
    pReader->mCache <<= nBits;
    pReader->mCached -= nBits;
    // An overdrawn count wraps to a large unsigned value and skips the refill, as in the image.
    while ((unsigned int)pReader->mCached < kCacheRefillBits) {
        pReader->mCache |= (unsigned long long)*pReader->mFetch
                           << (kCacheTopShift - pReader->mCached);
        ++pReader->mFetch;
        if ((uintptr_t)pReader->mFetch >= pReader->mRingEnd) {
            pReader->mFetch = pReader->mRingBase;
        }
        pReader->mCached += 8;
    }
    pReader->mPosition += (unsigned long long)(long long)nBits;
}

// 0x00610268
static void initBitReader(BitReader *pReader,
                          unsigned char *pStart,
                          unsigned char *pRingBase,
                          int nRingSize) {
    pReader->mFetch = pStart;
    pReader->mRingEnd = (uintptr_t)pRingBase + (uintptr_t)(intptr_t)nRingSize;
    pReader->mRingSize = nRingSize;
    pReader->mStart = pStart;
    pReader->mCache = 0;
    pReader->mCached = 0;
    pReader->mPosition = 0;
    pReader->mRingBase = pRingBase;
    bitReaderAdvance(pReader, 0);
}

// 0x006102a0
static int peekBits(const BitReader *pReader, int nBits) {
    return (int)(pReader->mCache >> (64 - nBits));
}

// 0x00610358
static int getBits(BitReader *pReader, int nBits) {
    int value = peekBits(pReader, nBits);
    bitReaderAdvance(pReader, nBits);
    return value;
}

// 0x006103a8
static int getBit(BitReader *pReader) {
    int value = peekBits(pReader, 1);
    bitReaderAdvance(pReader, 1);
    return value;
}

// 0x006103f0
// Moves the reader nBytes ahead of its bit position, rounded down to a byte, and refills.
static void skipBytes(BitReader *pReader, int nBytes) {
    unsigned long long position;
    unsigned char *pFetch;

    position = pReader->mPosition + (unsigned long long)(long long)(nBytes * 8);
    pReader->mCache = 0;
    pReader->mCached = 0;
    pFetch = pReader->mStart + (int)(position >> 3);
    if ((uintptr_t)pFetch >= pReader->mRingEnd) {
        pFetch -= pReader->mRingSize;
    }
    pReader->mFetch = pFetch;
    pReader->mPosition = position;
    bitReaderAdvance(pReader, 0);
}

// 0x00610448
static unsigned char *pointerAt(const BitReader *pReader, int nBitPosition) {
    unsigned char *pByte = pReader->mStart + (nBitPosition >> 3);
    if ((uintptr_t)pByte >= pReader->mRingEnd) {
        pByte -= pReader->mRingSize;
    }
    return pByte;
}

// A 33-bit time stamp split 3, 15, and 15 around marker bits, after its four-bit prefix.
static inline long long readTimestamp(BitReader *pReader) {
    int high;
    int middle;
    int low;
    unsigned int lowWord;

    (void)getBits(pReader, 4);
    high = getBits(pReader, 3);
    (void)getBit(pReader);
    middle = getBits(pReader, 15);
    (void)getBit(pReader);
    low = getBits(pReader, 15);
    (void)getBit(pReader);
    lowWord = ((unsigned int)high << 30) | ((unsigned int)middle << 15) | (unsigned int)low;
    return (long long)(((unsigned long long)((high >> 2) & 1) << 32) | lowWord);
}

// 0x005cacc0
static int parseSystemHeader(BitReader *pReader, PackHeader *pPack) {
    (void)pPack; // The image passes the pack header and never reads it.
    (void)getBits(pReader, 56); // Start code, header length, and the first rate bits.
    (void)getBits(pReader, 40); // The rest of the fixed fields.
    while (peekBits(pReader, 1) == 1) {
        (void)getBits(pReader, 24); // One stream bound entry.
    }
    return 1;
}

// 0x005cab70
static int parsePackHeader(BitReader *pReader, PackHeader *pPack) {
    int high;
    int middle;
    int low;
    int stuffing;
    int i;

    (void)getBits(pReader, 34); // Start code and the '01' marker.
    high = getBits(pReader, 3);
    (void)getBit(pReader);
    middle = getBits(pReader, 15);
    (void)getBit(pReader);
    low = getBits(pReader, 15);
    (void)getBit(pReader);
    pPack->mScrExtension = getBits(pReader, 9);
    (void)getBits(pReader, 30); // Marker, mux rate, markers, and reserved bits.
    stuffing = getBits(pReader, 3);
    pPack->mScrLow = ((unsigned int)high << 30) | ((unsigned int)middle << 15) | (unsigned int)low;
    pPack->mScrHigh = (int)(((unsigned int)high >> 2) & 1);
    for (i = 0; i < stuffing; ++i) {
        (void)getBits(pReader, 8);
    }
    if (peekBits(pReader, 32) == kSystemHeaderStartCode) {
        pPack->mHasSystemHeader = 1;
        parseSystemHeader(pReader, pPack);
    } else {
        pPack->mHasSystemHeader = 0;
    }
    return 1;
}

// 0x005cad30
// Returns zero when the packet embeds a pack header. A program stream may not embed one.
static int parsePacket(BitReader *pReader, PesPacket *pPacket) {
    int streamId;
    int ptsDtsFlags;
    int hasEscr;
    int optionalFlags;
    int hasExtension;
    int headerDataLength;
    int headerStart;
    int payload;
    int skip;
    int i;

    pPacket->mHeaderPosition = (int)pReader->mPosition;
    (void)getBits(pReader, 24);
    streamId = getBits(pReader, 8);
    pPacket->mKey = streamKey(streamId);
    pPacket->mPacketLength = getBits(pReader, 16);
    pPacket->mPts = -1;
    pPacket->mDts = -1;
    switch (streamId) {
    case kStreamIdPrivate2:
        pPacket->mKey |= (unsigned int)getBits(pReader, 32);
        skip = pPacket->mPacketLength - kSubstreamHeaderBytes;
        if (skip != 0) {
            skipBytes(pReader, skip);
        }
        return 1;
    case kStreamIdProgramStreamMap:
    case kStreamIdPadding:
    case kStreamIdEcm:
    case kStreamIdEmm:
    case kStreamIdDirectory:
    case kStreamIdDsmcc:
    case kStreamIdH2221TypeE:
        if (pPacket->mPacketLength != 0) {
            skipBytes(pReader, pPacket->mPacketLength);
        }
        return 1;
    default:
        break;
    }

    (void)getBits(pReader, 2); // The '10' marker.
    pPacket->mScrambling = getBits(pReader, 2);
    (void)getBits(pReader, 4); // Priority, alignment, copyright, and original flags.
    ptsDtsFlags = getBits(pReader, 2);
    hasEscr = getBits(pReader, 1);
    optionalFlags = getBits(pReader, 4);
    hasExtension = getBits(pReader, 1);
    headerDataLength = getBits(pReader, 8);
    headerStart = (int)pReader->mPosition;
    if ((ptsDtsFlags & kPtsFlag) != 0) {
        pPacket->mPts = readTimestamp(pReader);
    }
    if (ptsDtsFlags == kPtsDtsFlags) {
        pPacket->mDts = readTimestamp(pReader);
    }
    if (hasEscr == 1) {
        (void)getBits(pReader, 48);
    }
    if (optionalFlags != 0) {
        (void)getBits(pReader, kOptionalFieldBits[optionalFlags]);
    }
    if (hasExtension == 1) {
        int hasPrivateData = getBits(pReader, 1);
        int hasPackHeader = getBits(pReader, 1);
        int hasSequenceCounter = getBits(pReader, 1);
        int hasPStdBuffer = getBits(pReader, 1);
        int hasExtension2;

        (void)getBits(pReader, 3);
        hasExtension2 = getBits(pReader, 1);
        if (hasPrivateData == 1) {
            // 128 bits, read as 48, 48, and 32.
            (void)getBits(pReader, 48);
            (void)getBits(pReader, 48);
            (void)getBits(pReader, 32);
        }
        if (hasPackHeader == 1) {
            sceMpegRaiseError("pack_header_field_flag needs to be '0' in PS\n");
            return 0;
        }
        if (hasSequenceCounter == 1) {
            (void)getBits(pReader, 16);
        }
        if (hasPStdBuffer == 1) {
            (void)getBits(pReader, 16);
        }
        if (hasExtension2 == 1) {
            int length;

            (void)getBit(pReader);
            length = getBits(pReader, 7);
            for (i = 0; i < length; ++i) {
                (void)getBits(pReader, 8);
            }
        }
    }
    // Skip the rest of the header data, including any stuffing.
    skip = headerDataLength -
           (int)((pReader->mPosition - (unsigned long long)(long long)headerStart) >> 3);
    if (skip != 0) {
        skipBytes(pReader, skip);
    }
    payload = pPacket->mPacketLength - headerDataLength;
    pPacket->mDataLength = payload - kPesFlagBytes;
    pPacket->mDataPosition = (int)pReader->mPosition;
    skip = payload - kPesFlagBytes;
    if (pPacket->mKey == streamKey(kStreamIdPrivate1)) {
        // The substream word stays in the reported data and length.
        pPacket->mKey |= (unsigned int)getBits(pReader, 32);
        skip = payload - kPesFlagBytes - kSubstreamHeaderBytes;
    }
    if (skip != 0) {
        skipBytes(pReader, skip);
    }
    return 1;
}

static int deliverPacket(sceMpeg *pMpeg,
                         const BitReader *pReader,
                         const PesPacket *pPacket,
                         sceMpegCallback pfnCallback,
                         void *pData) {
    sceMpegCbDataStr callbackData;

    callbackData.type = sceMpegCbStr;
    callbackData.header = pointerAt(pReader, pPacket->mHeaderPosition);
    callbackData.data = pointerAt(pReader, pPacket->mDataPosition);
    callbackData.len = (unsigned int)pPacket->mDataLength;
    callbackData.pts = pPacket->mPts;
    callbackData.dts = pPacket->mDts;
    return pfnCallback(pMpeg, &callbackData, pData);
}

static int isPacketStart(const BitReader *pReader) {
    return peekBits(pReader, 24) == kPacketStartPrefix &&
           peekBits(pReader, 32) != kPackStartCode && peekBits(pReader, 32) != kProgramEndCode;
}

// 0x005ca768
int sceMpegDemuxPssRing(sceMpeg *pMpeg,
                        unsigned char *pStart,
                        int nSize,
                        unsigned char *pBuffer,
                        int nBufferSize) {
    MpegWork *work;
    StreamEntry *table;
    BitReader reader;
    PackHeader pack;
    // Packets without header data do not set the payload fields, so a callback sees the previous
    // packet's values. The image starts them from uninitialised stack memory.
    PesPacket packet = { 0 };
    sceMpegCallback defaultCallback;
    void *defaultData;
    unsigned long long limit;
    int consumed;
    int proceed;
    int count;
    int i;

    work = (MpegWork *)pMpeg->pContext;
    table = work->mStreamTable;
    defaultCallback = NULL;
    defaultData = NULL;
    consumed = 0;
    proceed = 1;
    limit = (unsigned long long)(long long)(nSize * 8);
    initBitReader(&reader, pStart, pBuffer, nBufferSize);
    count = work->mStreamCount;
    for (i = 0; i < count; ++i) {
        if (table[i].key == kDefaultStreamKey) {
            defaultData = table[i].data;
            defaultCallback = table[i].callback;
        }
        if (defaultCallback != NULL) {
            break;
        }
    }

    if (peekBits(&reader, 32) == kPackStartCode) {
        parsePackHeader(&reader, &pack);
    }
    for (;;) {
        if (isPacketStart(&reader) && reader.mPosition < limit) {
            if (proceed == 0) {
                return consumed;
            }
            parsePacket(&reader, &packet);
            if (limit < reader.mPosition) {
                continue;
            }
            count = work->mStreamCount;
            for (i = 0; i < count; ++i) {
                if (table[i].key == (packet.mKey & table[i].templateBits)) {
                    proceed =
                        deliverPacket(pMpeg, &reader, &packet, table[i].callback, table[i].data);
                    break;
                }
            }
            // The count is read again after a callback. The callback may register a stream.
            if (i == work->mStreamCount && defaultCallback != NULL) {
                proceed = deliverPacket(pMpeg, &reader, &packet, defaultCallback, defaultData);
            }
            if (proceed != 0) {
                consumed = (int)(reader.mPosition >> 3);
            }
            continue;
        }
        if (limit < reader.mPosition || peekBits(&reader, 32) != kPackStartCode) {
            return consumed;
        }
        parsePackHeader(&reader, &pack);
    }
}

// 0x005e08c8
int sceMpegIsEnd(void *pDecoder) {
    MpegWork *work;

    work = (MpegWork *)((sceMpeg *)pDecoder)->pContext;
    return work->mCompleted;
}

static int g_nMpegDecodeError; // Word at 0x007a2c08, set when a VLC or slice error was reported.
static int g_nMpegQuantiserScaleCode; // Word at 0x007a2d40, quantiser_scale_code.
static int g_nMpegIntraSlice; // Word at 0x007a33c0, intra_slice.


// The reference DMA chain. The word at 0x007a38b4 records its address.
static unsigned long long g_mpegMcChain[kMcPredictionSlots * 4] __attribute__((aligned(64)));

enum {
    kIpuBitPointerAddress = 0x10002020,
    kIpuTopAddress = 0x10002030,
    kIpuErrorCodeDetected = 0x4000u,
    kIpuReset = 0x40000000u,
    kIpuCommandBdec = 0x20000000u,
    kIpuCommandVdec = 0x30000000u,
    kIpuVdecTableShift = 26,
    kIpuPictureTypeShift = 24,
    kIpuPictureTypeMask = 7u,
    kBdecIntraShift = 27,
    kBdecResetDcShift = 26,
    kBdecDctTypeShift = 25,
    kBdecQuantiserShift = 16,
    kIpuBitsPerWord = 32,
    kIpuBitPointerMask = 0x1f,
    kToSprTadrAddress = 0x1000d430,
    kDmacEnableReadAddress = 0x1000f520,
    kDmacEnableWriteAddress = 0x1000f590,
    kDmacSuspend = 0x10000u,
    kDmaChcrChainToSpr = 0x105u,
    kDmaScratchpadFlag = 0x80000000u,
    kDmaTagRef = 0x30000000u,
    kDmaTagRefe = 0,
    kCoefficientQwords = 48,
    kStagingPairQwords = 48,
    kMpegMcOutputBase = 0x70003600, // Word at 0x007a38b8, never written.
};

// The VDEC tables, the macroblock_type bits, and the coding enumerations of ISO/IEC 13818-2.
enum {
    kVdecMacroblockAddressIncrement = 0,
    kVdecMacroblockType = 1,
    kVdecMotionCode = 2,
    kVdecDmVector = 3,
    kMbIntra = 1,
    kMbPattern = 2,
    kMbMotionBackward = 4,
    kMbMotionForward = 8,
    kMbQuant = 0x10,
    kMcField = 1,
    kMcFrame = 2,
    kMc16x8 = 2,
    kMcDualPrime = 3,
    kTopField = 1,
    kBottomField = 2,
    kFramePicture = 3,
    kPictureI = 1,
    kPictureP = 2,
    kMbaiStuffing = 34,
    kMbaiEscape = 35,
    kMbaiEscapeIncrement = 33,
    kMbaiPeekBits = 11,
    kMpeg1StuffingCode = 15,
    kSliceStartCodeFirst = 0x101,
    kSliceStartCodeCount = 0xaf,
    kSliceDone = 0,
    kSliceError = 1,
    kSliceSkipPicture = 2,
    kSliceNext = 3,
};

// Staging and output layout. A staged reference is four macroblocks, two rows of each of two
// columns, and a macroblock stores 256 luma bytes then 64 bytes of each chroma plane.
enum {
    kStagingBytes = 1536,
    kStagingRightColumn = 768,
    kChromaPlaneOffset = 256,
    kLumaSize = 16,
    kChromaSize = 8,
    kHalfLumaSize = 8,
    kLumaRowShift = 4,
    kChromaRowShift = 3,
    kLumaDestRowShift = 5,
    kChromaDestRowShift = 4,
    kChromaDestOffset = 512,
};

// The hand-written MMI kernels at 0x00609268 to 0x0060a038. Each prediction kernel takes one
// MpegMcDescriptor. The luma kernels read quadword rows; the chroma kernels read doubleword rows of
// both planes. Index bit 0 is half-pel vertical, bit 1 half-pel horizontal, and bit 2 averages
// with the prediction already in the output.
void sceMpegMcLumaCopy(const MpegMcDescriptor *pDescriptor);
void sceMpegMcChromaCopy(const MpegMcDescriptor *pDescriptor);
void sceMpegMcLumaHalfV(const MpegMcDescriptor *pDescriptor);
void sceMpegMcChromaHalfV(const MpegMcDescriptor *pDescriptor);
void sceMpegMcLumaHalfH(const MpegMcDescriptor *pDescriptor);
void sceMpegMcChromaHalfH(const MpegMcDescriptor *pDescriptor);
void sceMpegMcLumaHalfHV(const MpegMcDescriptor *pDescriptor);
void sceMpegMcChromaHalfHV(const MpegMcDescriptor *pDescriptor);
void sceMpegMcLumaAverageCopy(const MpegMcDescriptor *pDescriptor);
void sceMpegMcChromaAverageCopy(const MpegMcDescriptor *pDescriptor);
void sceMpegMcLumaAverageHalfV(const MpegMcDescriptor *pDescriptor);
void sceMpegMcChromaAverageHalfV(const MpegMcDescriptor *pDescriptor);
void sceMpegMcLumaAverageHalfH(const MpegMcDescriptor *pDescriptor);
void sceMpegMcChromaAverageHalfH(const MpegMcDescriptor *pDescriptor);
void sceMpegMcLumaAverageHalfHV(const MpegMcDescriptor *pDescriptor);
void sceMpegMcChromaAverageHalfHV(const MpegMcDescriptor *pDescriptor);
// Adds the prediction to the decoded block, saturates, and packs 384 bytes to nDest.
void sceMpegMcAddBlock(int nDest, int nPrediction, int nBlock);
// Saturates and packs 384 bytes of nSource to nDest.
void sceMpegMcPutBlock(int nDest, int nSource);

// 0x007a33c8
static const MpegMcKernel g_mpegLumaKernels[kMcKernelVariants] = {
    sceMpegMcLumaCopy, sceMpegMcLumaHalfV, sceMpegMcLumaHalfH, sceMpegMcLumaHalfHV,
    sceMpegMcLumaAverageCopy, sceMpegMcLumaAverageHalfV, sceMpegMcLumaAverageHalfH, sceMpegMcLumaAverageHalfHV,
};

// 0x007a33e8
static const MpegMcKernel g_mpegChromaKernels[kMcKernelVariants] = {
    sceMpegMcChromaCopy, sceMpegMcChromaHalfV, sceMpegMcChromaHalfH, sceMpegMcChromaHalfHV,
    sceMpegMcChromaAverageCopy, sceMpegMcChromaAverageHalfV, sceMpegMcChromaAverageHalfH, sceMpegMcChromaAverageHalfHV,
};

// The routines retain the image's layout. Each starts at the same offset within a quadword and
// the clamp constant at 0x0060a020 stays quadword aligned. The constant sits inside the last
// routine's path and executes as a no-op shift of $zero.
__asm__(
    "    .text\n"
    "    .set push\n"
    "    .set noreorder\n"
    "    .set nomacro\n"
    "    .set noat\n"
    "    .align 4\n"
    "    nop\n"
    "    nop\n"
"    .type sceMpegMcLumaCopy, @function\n"
    "sceMpegMcLumaCopy:\n"
    "    lw $5,20($4)\n"
    "    lw $6,24($4)\n"
    "    lw $7,8($4)\n"
    "    lw $14,0($4)\n"
    "    lw $13,4($4)\n"
    "    lw $12,16($4)\n"
    "    sll $11,$12,0x1\n"
    "    addiu $15,$0,-1\n"
    "    mtsab $13,0\n"
    ".LMcLumaCopyRow:\n"
    "    lq $8,0($5)\n"
    "    addi $7,$7,-1\n"
    "    lq $9,0($6)\n"
    "    addu $5,$5,$12\n"
    "    qfsrv $10,$9,$8\n"
    "    pextlb $8,$0,$10\n"
    "    pextub $9,$0,$10\n"
    "    sq $8,0($14)\n"
    "    addu $6,$6,$12\n"
    "    sq $9,16($14)\n"
    "    bgtz $7,.LMcLumaCopyRow\n"
    "    addu $14,$14,$11\n"
    "    addiu $5,$5,128\n"
    "    addiu $6,$6,128\n"
    "    lw $7,12($4)\n"
    "    and $10,$15,$7\n"
    "    bne $10,$0,.LMcLumaCopyRow\n"
    "    daddu $15,$0,$0\n"
    "    jr $31\n"
    "    sll $0,$0,0x0\n"
    "    sll $0,$0,0x0\n"
    "    .type sceMpegMcChromaCopy, @function\n"
    "sceMpegMcChromaCopy:\n"
    "    lw $5,20($4)\n"
    "    lw $6,24($4)\n"
    "    lw $14,0($4)\n"
    "    lw $13,4($4)\n"
    "    lw $12,16($4)\n"
    "    sll $11,$12,0x1\n"
    "    mtsab $13,0\n"
    "    addiu $24,$0,-1\n"
    ".LMcChromaCopyPlane:\n"
    "    lw $7,8($4)\n"
    "    addiu $15,$0,-1\n"
    ".LMcChromaCopyRow:\n"
    "    ld $8,0($5)\n"
    "    ld $9,0($6)\n"
    "    pcpyld $8,$9,$8\n"
    "    qfsrv $9,$8,$8\n"
    "    pextlb $8,$0,$9\n"
    "    sq $8,0($14)\n"
    "    addi $7,$7,-1\n"
    "    addu $5,$5,$12\n"
    "    addu $14,$14,$11\n"
    "    bgtz $7,.LMcChromaCopyRow\n"
    "    addu $6,$6,$12\n"
    "    addiu $5,$5,320\n"
    "    addiu $6,$6,320\n"
    "    lw $7,12($4)\n"
    "    and $10,$15,$7\n"
    "    bne $10,$0,.LMcChromaCopyRow\n"
    "    daddu $15,$0,$0\n"
    "    lw $5,20($4)\n"
    "    lw $6,24($4)\n"
    "    lw $14,0($4)\n"
    "    addiu $5,$5,64\n"
    "    addiu $6,$6,64\n"
    "    addiu $14,$14,128\n"
    "    bne $24,$0,.LMcChromaCopyPlane\n"
    "    daddu $24,$0,$0\n"
    "    jr $31\n"
    "    sll $0,$0,0x0\n"
    "    sll $0,$0,0x0\n"
    "    .type sceMpegMcLumaHalfV, @function\n"
    "sceMpegMcLumaHalfV:\n"
    "    pnor $25,$0,$0\n"
    "    psrlh $25,$25,0xf\n"
    "    lw $5,20($4)\n"
    "    lw $6,24($4)\n"
    "    lw $7,8($4)\n"
    "    lw $14,0($4)\n"
    "    lw $13,4($4)\n"
    "    lw $24,16($4)\n"
    "    lq $8,0($5)\n"
    "    sll $12,$24,0x1\n"
    "    lq $9,0($6)\n"
    "    mtsab $13,0\n"
    "    qfsrv $10,$9,$8\n"
    "    pextlb $8,$0,$10\n"
    "    addiu $11,$0,-1\n"
    "    beq $7,$0,.LMcLumaHalfVBelow\n"
    "    pextub $9,$0,$10\n"
    ".LMcLumaHalfVRow:\n"
    "    addu $5,$5,$24\n"
    "    addu $6,$6,$24\n"
    "    lq $10,0($5)\n"
    "    lq $15,0($6)\n"
    "    qfsrv $2,$15,$10\n"
    "    pextlb $10,$0,$2\n"
    "    addi $7,$7,-1\n"
    "    pextub $15,$0,$2\n"
    "    paddh $2,$8,$10\n"
    "    paddh $3,$9,$15\n"
    "    por $8,$10,$0\n"
    "    por $9,$15,$0\n"
    "    paddh $2,$2,$25\n"
    "    paddh $3,$3,$25\n"
    "    psrlh $2,$2,0x1\n"
    "    psrlh $3,$3,0x1\n"
    "    sq $2,0($14)\n"
    "    sq $3,16($14)\n"
    "    bgtz $7,.LMcLumaHalfVRow\n"
    "    addu $14,$14,$12\n"
    ".LMcLumaHalfVBelow:\n"
    "    addiu $5,$5,128\n"
    "    addiu $6,$6,128\n"
    "    lw $7,12($4)\n"
    "    and $10,$11,$7\n"
    "    bne $10,$0,.LMcLumaHalfVRow\n"
    "    daddu $11,$0,$0\n"
    "    jr $31\n"
    "    sll $0,$0,0x0\n"
    "    sll $0,$0,0x0\n"
    "    .type sceMpegMcChromaHalfV, @function\n"
    "sceMpegMcChromaHalfV:\n"
    "    pnor $25,$0,$0\n"
    "    psrlh $25,$25,0xf\n"
    "    lw $5,20($4)\n"
    "    lw $6,24($4)\n"
    "    lw $14,0($4)\n"
    "    lw $13,4($4)\n"
    "    lw $12,16($4)\n"
    "    addiu $11,$0,1\n"
    "    sll $24,$12,0x1\n"
    "    mtsab $13,0\n"
    ".LMcChromaHalfVPlane:\n"
    "    lw $7,8($4)\n"
    "    ld $8,0($5)\n"
    "    ld $9,0($6)\n"
    "    pcpyld $8,$9,$8\n"
    "    qfsrv $8,$8,$8\n"
    "    ori $11,$11,0x8000\n"
    "    beq $7,$0,.LMcChromaHalfVBelow\n"
    "    pextlb $15,$0,$8\n"
    ".LMcChromaHalfVRow:\n"
    "    addu $5,$5,$12\n"
    "    addu $6,$6,$12\n"
    "    ld $8,0($5)\n"
    "    ld $9,0($6)\n"
    "    pcpyld $8,$9,$8\n"
    "    qfsrv $8,$8,$8\n"
    "    pextlb $10,$0,$8\n"
    "    addi $7,$7,-1\n"
    "    paddh $9,$10,$15\n"
    "    por $15,$10,$0\n"
    "    paddh $10,$9,$25\n"
    "    psrlh $10,$10,0x1\n"
    "    sq $10,0($14)\n"
    "    bgtz $7,.LMcChromaHalfVRow\n"
    "    addu $14,$14,$24\n"
    ".LMcChromaHalfVBelow:\n"
    "    psrah $10,$11,0xf\n"
    "    addiu $5,$5,320\n"
    "    lw $7,12($4)\n"
    "    addiu $6,$6,320\n"
    "    and $10,$10,$7\n"
    "    bne $10,$0,.LMcChromaHalfVRow\n"
    "    andi $11,$11,0x7fff\n"
    "    lw $5,20($4)\n"
    "    lw $6,24($4)\n"
    "    lw $14,0($4)\n"
    "    addiu $5,$5,64\n"
    "    addiu $6,$6,64\n"
    "    addiu $14,$14,128\n"
    "    andi $10,$11,0x1\n"
    "    bne $10,$0,.LMcChromaHalfVPlane\n"
    "    andi $11,$11,0xfffe\n"
    "    jr $31\n"
    "    sll $0,$0,0x0\n"
    "    sll $0,$0,0x0\n"
    "    .type sceMpegMcLumaHalfH, @function\n"
    "sceMpegMcLumaHalfH:\n"
    "    pnor $25,$0,$0\n"
    "    psrlh $25,$25,0xf\n"
    "    lw $5,20($4)\n"
    "    lw $6,24($4)\n"
    "    lw $7,8($4)\n"
    "    lw $14,0($4)\n"
    "    lw $13,4($4)\n"
    "    addiu $24,$0,1\n"
    "    lw $9,16($4)\n"
    "    sll $8,$9,0x1\n"
    "    addiu $11,$0,-1\n"
    ".LMcLumaHalfHRow:\n"
    "    lq $10,0($5)\n"
    "    lq $15,0($6)\n"
    "    mtsab $13,0\n"
    "    qfsrv $2,$15,$10\n"
    "    qfsrv $3,$10,$15\n"
    "    pextlb $10,$0,$2\n"
    "    addi $7,$7,-1\n"
    "    pextub $15,$0,$2\n"
    "    mtsab $24,0\n"
    "    qfsrv $3,$3,$2\n"
    "    pextlb $2,$0,$3\n"
    "    pextub $3,$0,$3\n"
    "    paddh $10,$10,$2\n"
    "    paddh $15,$15,$3\n"
    "    paddh $2,$10,$25\n"
    "    paddh $3,$15,$25\n"
    "    psrlh $2,$2,0x1\n"
    "    psrlh $3,$3,0x1\n"
    "    sq $2,0($14)\n"
    "    sq $3,16($14)\n"
    "    addu $5,$5,$9\n"
    "    addu $6,$6,$9\n"
    "    bgtz $7,.LMcLumaHalfHRow\n"
    "    addu $14,$14,$8\n"
    "    addiu $5,$5,128\n"
    "    addiu $6,$6,128\n"
    "    lw $7,12($4)\n"
    "    and $12,$11,$7\n"
    "    bne $12,$0,.LMcLumaHalfHRow\n"
    "    daddu $11,$0,$0\n"
    "    jr $31\n"
    "    sll $0,$0,0x0\n"
    "    sll $0,$0,0x0\n"
    "    .type sceMpegMcChromaHalfH, @function\n"
    "sceMpegMcChromaHalfH:\n"
    "    pnor $25,$0,$0\n"
    "    psrlh $25,$25,0xf\n"
    "    lw $5,20($4)\n"
    "    lw $6,24($4)\n"
    "    lw $14,0($4)\n"
    "    lw $13,4($4)\n"
    "    addiu $24,$0,1\n"
    "    addiu $12,$0,-1\n"
    "    lw $3,16($4)\n"
    "    sll $2,$3,0x1\n"
    ".LMcChromaHalfHPlane:\n"
    "    lw $7,8($4)\n"
    "    addiu $11,$0,-1\n"
    ".LMcChromaHalfHRow:\n"
    "    ld $8,0($5)\n"
    "    ld $9,0($6)\n"
    "    pcpyld $8,$9,$8\n"
    "    mtsab $13,0\n"
    "    qfsrv $8,$8,$8\n"
    "    pextlb $9,$0,$8\n"
    "    addi $7,$7,-1\n"
    "    addu $5,$5,$3\n"
    "    addu $6,$6,$3\n"
    "    mtsab $24,0\n"
    "    qfsrv $10,$0,$8\n"
    "    pextlb $8,$0,$10\n"
    "    paddh $10,$9,$8\n"
    "    paddh $10,$10,$25\n"
    "    psrlh $10,$10,0x1\n"
    "    sq $10,0($14)\n"
    "    bgtz $7,.LMcChromaHalfHRow\n"
    "    addu $14,$14,$2\n"
    "    addiu $5,$5,320\n"
    "    addiu $6,$6,320\n"
    "    lw $7,12($4)\n"
    "    and $10,$11,$7\n"
    "    bne $10,$0,.LMcChromaHalfHRow\n"
    "    daddu $11,$0,$0\n"
    "    lw $5,20($4)\n"
    "    lw $6,24($4)\n"
    "    lw $14,0($4)\n"
    "    addiu $5,$5,64\n"
    "    addiu $6,$6,64\n"
    "    addiu $14,$14,128\n"
    "    bne $12,$0,.LMcChromaHalfHPlane\n"
    "    daddu $12,$0,$0\n"
    "    jr $31\n"
    "    sll $0,$0,0x0\n"
    "    .type sceMpegMcLumaHalfHV, @function\n"
    "sceMpegMcLumaHalfHV:\n"
    "    pnor $25,$0,$0\n"
    "    psrlh $25,$25,0xf\n"
    "    psllh $25,$25,0x1\n"
    "    lw $5,20($4)\n"
    "    lw $6,24($4)\n"
    "    lw $7,8($4)\n"
    "    lw $14,0($4)\n"
    "    lw $13,4($4)\n"
    "    lw $12,16($4)\n"
    "    addiu $24,$0,1\n"
    "    lq $8,0($5)\n"
    "    lq $9,0($6)\n"
    "    mtsab $13,0\n"
    "    qfsrv $10,$9,$8\n"
    "    qfsrv $15,$8,$9\n"
    "    pextlb $8,$0,$10\n"
    "    pextub $9,$0,$10\n"
    "    addiu $11,$0,-1\n"
    "    mtsab $24,0\n"
    "    qfsrv $15,$15,$10\n"
    "    pextlb $10,$0,$15\n"
    "    pextub $15,$0,$15\n"
    "    paddh $8,$8,$10\n"
    "    beq $7,$0,.LMcLumaHalfHVBelow\n"
    "    paddh $9,$9,$15\n"
    ".LMcLumaHalfHVRow:\n"
    "    addu $5,$5,$12\n"
    "    addu $6,$6,$12\n"
    "    lq $10,0($5)\n"
    "    lq $15,0($6)\n"
    "    mtsab $13,0\n"
    "    qfsrv $2,$15,$10\n"
    "    qfsrv $3,$10,$15\n"
    "    pextlb $10,$0,$2\n"
    "    addi $7,$7,-1\n"
    "    pextub $15,$0,$2\n"
    "    mtsab $24,0\n"
    "    qfsrv $3,$3,$2\n"
    "    pextlb $2,$0,$3\n"
    "    pextub $3,$0,$3\n"
    "    paddh $10,$10,$2\n"
    "    paddh $15,$15,$3\n"
    "    paddh $2,$8,$10\n"
    "    paddh $3,$9,$15\n"
    "    por $8,$10,$0\n"
    "    por $9,$15,$0\n"
    "    paddh $2,$2,$25\n"
    "    paddh $3,$3,$25\n"
    "    psrlh $2,$2,0x2\n"
    "    psrlh $3,$3,0x2\n"
    "    sq $2,0($14)\n"
    "    sll $10,$12,0x1\n"
    "    sq $3,16($14)\n"
    "    bgtz $7,.LMcLumaHalfHVRow\n"
    "    addu $14,$14,$10\n"
    ".LMcLumaHalfHVBelow:\n"
    "    addiu $5,$5,128\n"
    "    addiu $6,$6,128\n"
    "    lw $7,12($4)\n"
    "    and $10,$11,$7\n"
    "    bne $10,$0,.LMcLumaHalfHVRow\n"
    "    daddu $11,$0,$0\n"
    "    jr $31\n"
    "    sll $0,$0,0x0\n"
    "    .type sceMpegMcChromaHalfHV, @function\n"
    "sceMpegMcChromaHalfHV:\n"
    "    pnor $25,$0,$0\n"
    "    psrlh $25,$25,0xf\n"
    "    psllh $25,$25,0x1\n"
    "    lw $5,20($4)\n"
    "    lw $6,24($4)\n"
    "    lw $14,0($4)\n"
    "    lw $13,4($4)\n"
    "    lw $12,16($4)\n"
    "    addiu $24,$0,1\n"
    "    addiu $11,$0,1\n"
    ".LMcChromaHalfHVPlane:\n"
    "    lw $7,8($4)\n"
    "    ld $8,0($5)\n"
    "    ld $9,0($6)\n"
    "    pcpyld $8,$9,$8\n"
    "    mtsab $13,0\n"
    "    qfsrv $8,$8,$8\n"
    "    pextlb $9,$0,$8\n"
    "    addu $5,$5,$12\n"
    "    ori $11,$11,0x8000\n"
    "    mtsab $24,0\n"
    "    qfsrv $10,$0,$8\n"
    "    pextlb $8,$0,$10\n"
    "    beq $7,$0,.LMcChromaHalfHVBelow\n"
    "    paddh $15,$9,$8\n"
    ".LMcChromaHalfHVRow:\n"
    "    addu $6,$6,$12\n"
    "    ld $8,0($5)\n"
    "    ld $9,0($6)\n"
    "    pcpyld $8,$9,$8\n"
    "    mtsab $13,0\n"
    "    qfsrv $8,$8,$8\n"
    "    pextlb $9,$0,$8\n"
    "    addi $7,$7,-1\n"
    "    addu $5,$5,$12\n"
    "    mtsab $24,0\n"
    "    qfsrv $10,$0,$8\n"
    "    pextlb $8,$0,$10\n"
    "    paddh $10,$9,$8\n"
    "    paddh $9,$10,$15\n"
    "    por $15,$10,$0\n"
    "    paddh $10,$9,$25\n"
    "    sll $8,$12,0x1\n"
    "    psrlh $10,$10,0x2\n"
    "    sq $10,0($14)\n"
    "    bgtz $7,.LMcChromaHalfHVRow\n"
    "    addu $14,$14,$8\n"
    ".LMcChromaHalfHVBelow:\n"
    "    psrah $10,$11,0xf\n"
    "    addiu $5,$5,320\n"
    "    lw $7,12($4)\n"
    "    addiu $6,$6,320\n"
    "    and $10,$10,$7\n"
    "    bne $10,$0,.LMcChromaHalfHVRow\n"
    "    andi $11,$11,0x7fff\n"
    "    lw $5,20($4)\n"
    "    lw $6,24($4)\n"
    "    lw $14,0($4)\n"
    "    addiu $5,$5,64\n"
    "    addiu $6,$6,64\n"
    "    addiu $14,$14,128\n"
    "    andi $10,$11,0x1\n"
    "    bne $10,$0,.LMcChromaHalfHVPlane\n"
    "    andi $11,$11,0xfffe\n"
    "    jr $31\n"
    "    sll $0,$0,0x0\n"
    "    sll $0,$0,0x0\n"
    "    .type sceMpegMcLumaAverageCopy, @function\n"
    "sceMpegMcLumaAverageCopy:\n"
    "    lw $5,20($4)\n"
    "    lw $6,24($4)\n"
    "    lw $7,8($4)\n"
    "    lw $14,0($4)\n"
    "    lw $13,4($4)\n"
    "    lw $9,16($4)\n"
    "    sll $8,$9,0x1\n"
    "    addiu $11,$0,-1\n"
    "    mtsab $13,0\n"
    ".LMcLumaAverageCopyRow:\n"
    "    lq $10,0($5)\n"
    "    lq $15,0($6)\n"
    "    qfsrv $2,$15,$10\n"
    "    pextlb $10,$0,$2\n"
    "    pextub $15,$0,$2\n"
    "    lq $2,0($14)\n"
    "    lq $3,16($14)\n"
    "    paddh $2,$2,$10\n"
    "    paddh $3,$3,$15\n"
    "    pcgth $10,$2,$0\n"
    "    psrlh $10,$10,0xf\n"
    "    paddh $10,$2,$10\n"
    "    psrlh $2,$10,0x1\n"
    "    pcgth $10,$3,$0\n"
    "    psrlh $10,$10,0xf\n"
    "    paddh $10,$3,$10\n"
    "    psrlh $3,$10,0x1\n"
    "    sq $2,0($14)\n"
    "    sq $3,16($14)\n"
    "    addi $7,$7,-1\n"
    "    addu $5,$5,$9\n"
    "    addu $14,$14,$8\n"
    "    bgtz $7,.LMcLumaAverageCopyRow\n"
    "    addu $6,$6,$9\n"
    "    addiu $5,$5,128\n"
    "    addiu $6,$6,128\n"
    "    lw $7,12($4)\n"
    "    and $12,$11,$7\n"
    "    bne $12,$0,.LMcLumaAverageCopyRow\n"
    "    daddu $11,$0,$0\n"
    "    jr $31\n"
    "    sll $0,$0,0x0\n"
    "    sll $0,$0,0x0\n"
    "    .type sceMpegMcChromaAverageCopy, @function\n"
    "sceMpegMcChromaAverageCopy:\n"
    "    lw $5,20($4)\n"
    "    lw $6,24($4)\n"
    "    lw $14,0($4)\n"
    "    lw $13,4($4)\n"
    "    addiu $12,$0,-1\n"
    "    lw $3,16($4)\n"
    "    sll $2,$3,0x1\n"
    "    mtsab $13,0\n"
    ".LMcChromaAverageCopyPlane:\n"
    "    lw $7,8($4)\n"
    "    addiu $11,$0,-1\n"
    ".LMcChromaAverageCopyRow:\n"
    "    ld $8,0($5)\n"
    "    ld $9,0($6)\n"
    "    pcpyld $8,$9,$8\n"
    "    qfsrv $8,$8,$8\n"
    "    pextlb $9,$0,$8\n"
    "    addi $7,$7,-1\n"
    "    addu $5,$5,$3\n"
    "    addu $6,$6,$3\n"
    "    lq $8,0($14)\n"
    "    paddh $10,$9,$8\n"
    "    pcgth $9,$10,$0\n"
    "    psrlh $9,$9,0xf\n"
    "    paddh $10,$10,$9\n"
    "    psrlh $10,$10,0x1\n"
    "    sq $10,0($14)\n"
    "    bgtz $7,.LMcChromaAverageCopyRow\n"
    "    addu $14,$14,$2\n"
    "    addiu $5,$5,320\n"
    "    addiu $6,$6,320\n"
    "    lw $7,12($4)\n"
    "    and $10,$11,$7\n"
    "    bne $10,$0,.LMcChromaAverageCopyRow\n"
    "    daddu $11,$0,$0\n"
    "    lw $5,20($4)\n"
    "    lw $6,24($4)\n"
    "    lw $14,0($4)\n"
    "    addiu $5,$5,64\n"
    "    addiu $6,$6,64\n"
    "    addiu $14,$14,128\n"
    "    bne $12,$0,.LMcChromaAverageCopyPlane\n"
    "    daddu $12,$0,$0\n"
    "    jr $31\n"
    "    sll $0,$0,0x0\n"
    "    sll $0,$0,0x0\n"
    "    .type sceMpegMcLumaAverageHalfV, @function\n"
    "sceMpegMcLumaAverageHalfV:\n"
    "    pnor $25,$0,$0\n"
    "    psrlh $25,$25,0xf\n"
    "    lw $5,20($4)\n"
    "    lw $6,24($4)\n"
    "    lw $7,8($4)\n"
    "    lw $14,0($4)\n"
    "    lw $13,4($4)\n"
    "    lw $12,16($4)\n"
    "    lq $8,0($5)\n"
    "    lq $9,0($6)\n"
    "    mtsab $13,0\n"
    "    qfsrv $10,$9,$8\n"
    "    sll $24,$12,0x1\n"
    "    pextlb $8,$0,$10\n"
    "    addiu $11,$0,-1\n"
    "    beq $7,$0,.LMcLumaAverageHalfVBelow\n"
    "    pextub $9,$0,$10\n"
    ".LMcLumaAverageHalfVRow:\n"
    "    addu $5,$5,$12\n"
    "    addu $6,$6,$12\n"
    "    lq $10,0($5)\n"
    "    lq $15,0($6)\n"
    "    qfsrv $2,$15,$10\n"
    "    pextlb $10,$0,$2\n"
    "    addi $7,$7,-1\n"
    "    pextub $15,$0,$2\n"
    "    paddh $2,$8,$10\n"
    "    paddh $3,$9,$15\n"
    "    por $8,$10,$0\n"
    "    por $9,$15,$0\n"
    "    paddh $2,$2,$25\n"
    "    paddh $3,$3,$25\n"
    "    psrlh $2,$2,0x1\n"
    "    psrlh $3,$3,0x1\n"
    "    lq $10,0($14)\n"
    "    lq $15,16($14)\n"
    "    paddh $2,$2,$10\n"
    "    paddh $3,$3,$15\n"
    "    pcgth $10,$2,$0\n"
    "    psrlh $10,$10,0xf\n"
    "    paddh $10,$2,$10\n"
    "    psrlh $2,$10,0x1\n"
    "    pcgth $10,$3,$0\n"
    "    psrlh $10,$10,0xf\n"
    "    paddh $10,$3,$10\n"
    "    psrlh $3,$10,0x1\n"
    "    sq $2,0($14)\n"
    "    sq $3,16($14)\n"
    "    bgtz $7,.LMcLumaAverageHalfVRow\n"
    "    addu $14,$14,$24\n"
    ".LMcLumaAverageHalfVBelow:\n"
    "    addiu $5,$5,128\n"
    "    addiu $6,$6,128\n"
    "    lw $7,12($4)\n"
    "    and $10,$11,$7\n"
    "    bne $10,$0,.LMcLumaAverageHalfVRow\n"
    "    daddu $11,$0,$0\n"
    "    jr $31\n"
    "    sll $0,$0,0x0\n"
    "    sll $0,$0,0x0\n"
    "    .type sceMpegMcChromaAverageHalfV, @function\n"
    "sceMpegMcChromaAverageHalfV:\n"
    "    pnor $25,$0,$0\n"
    "    psrlh $25,$25,0xf\n"
    "    lw $5,20($4)\n"
    "    lw $6,24($4)\n"
    "    lw $14,0($4)\n"
    "    lw $13,4($4)\n"
    "    lw $12,16($4)\n"
    "    addiu $11,$0,1\n"
    "    sll $24,$12,0x1\n"
    "    mtsab $13,0\n"
    ".LMcChromaAverageHalfVPlane:\n"
    "    lw $7,8($4)\n"
    "    ld $8,0($5)\n"
    "    ld $9,0($6)\n"
    "    pcpyld $8,$9,$8\n"
    "    qfsrv $8,$8,$8\n"
    "    ori $11,$11,0x8000\n"
    "    beq $7,$0,.LMcChromaAverageHalfVBelow\n"
    "    pextlb $15,$0,$8\n"
    ".LMcChromaAverageHalfVRow:\n"
    "    addu $5,$5,$12\n"
    "    addu $6,$6,$12\n"
    "    ld $8,0($5)\n"
    "    ld $9,0($6)\n"
    "    pcpyld $8,$9,$8\n"
    "    qfsrv $8,$8,$8\n"
    "    pextlb $10,$0,$8\n"
    "    addi $7,$7,-1\n"
    "    paddh $9,$10,$15\n"
    "    por $15,$10,$0\n"
    "    paddh $10,$9,$25\n"
    "    psrlh $10,$10,0x1\n"
    "    lq $8,0($14)\n"
    "    paddh $10,$10,$8\n"
    "    pcgth $9,$10,$0\n"
    "    psrlh $9,$9,0xf\n"
    "    paddh $10,$10,$9\n"
    "    psrlh $10,$10,0x1\n"
    "    sq $10,0($14)\n"
    "    bgtz $7,.LMcChromaAverageHalfVRow\n"
    "    addu $14,$14,$24\n"
    ".LMcChromaAverageHalfVBelow:\n"
    "    psrah $10,$11,0xf\n"
    "    addiu $5,$5,320\n"
    "    lw $7,12($4)\n"
    "    addiu $6,$6,320\n"
    "    and $10,$10,$7\n"
    "    bne $10,$0,.LMcChromaAverageHalfVRow\n"
    "    andi $11,$11,0x7fff\n"
    "    lw $5,20($4)\n"
    "    lw $6,24($4)\n"
    "    lw $14,0($4)\n"
    "    addiu $5,$5,64\n"
    "    addiu $6,$6,64\n"
    "    addiu $14,$14,128\n"
    "    andi $10,$11,0x1\n"
    "    bne $10,$0,.LMcChromaAverageHalfVPlane\n"
    "    andi $11,$11,0xfffe\n"
    "    jr $31\n"
    "    sll $0,$0,0x0\n"
    "    sll $0,$0,0x0\n"
    "    .type sceMpegMcLumaAverageHalfH, @function\n"
    "sceMpegMcLumaAverageHalfH:\n"
    "    pnor $25,$0,$0\n"
    "    psrlh $25,$25,0xf\n"
    "    lw $5,20($4)\n"
    "    lw $6,24($4)\n"
    "    lw $7,8($4)\n"
    "    lw $14,0($4)\n"
    "    lw $13,4($4)\n"
    "    addiu $24,$0,1\n"
    "    lw $9,16($4)\n"
    "    sll $8,$9,0x1\n"
    "    addiu $11,$0,-1\n"
    ".LMcLumaAverageHalfHRow:\n"
    "    lq $10,0($5)\n"
    "    lq $15,0($6)\n"
    "    mtsab $13,0\n"
    "    qfsrv $2,$15,$10\n"
    "    qfsrv $3,$10,$15\n"
    "    pextlb $10,$0,$2\n"
    "    addi $7,$7,-1\n"
    "    pextub $15,$0,$2\n"
    "    mtsab $24,0\n"
    "    qfsrv $3,$3,$2\n"
    "    pextlb $2,$0,$3\n"
    "    pextub $3,$0,$3\n"
    "    paddh $10,$10,$2\n"
    "    paddh $15,$15,$3\n"
    "    paddh $2,$10,$25\n"
    "    paddh $3,$15,$25\n"
    "    psrlh $2,$2,0x1\n"
    "    psrlh $3,$3,0x1\n"
    "    lq $10,0($14)\n"
    "    lq $15,16($14)\n"
    "    paddh $2,$2,$10\n"
    "    paddh $3,$3,$15\n"
    "    pcgth $10,$2,$0\n"
    "    psrlh $10,$10,0xf\n"
    "    paddh $10,$2,$10\n"
    "    psrlh $2,$10,0x1\n"
    "    pcgth $10,$3,$0\n"
    "    psrlh $10,$10,0xf\n"
    "    paddh $10,$3,$10\n"
    "    psrlh $3,$10,0x1\n"
    "    sq $2,0($14)\n"
    "    sq $3,16($14)\n"
    "    addu $5,$5,$9\n"
    "    addu $6,$6,$9\n"
    "    bgtz $7,.LMcLumaAverageHalfHRow\n"
    "    addu $14,$14,$8\n"
    "    addiu $5,$5,128\n"
    "    addiu $6,$6,128\n"
    "    lw $7,12($4)\n"
    "    and $12,$11,$7\n"
    "    bne $12,$0,.LMcLumaAverageHalfHRow\n"
    "    daddu $11,$0,$0\n"
    "    jr $31\n"
    "    sll $0,$0,0x0\n"
    "    sll $0,$0,0x0\n"
    "    .type sceMpegMcChromaAverageHalfH, @function\n"
    "sceMpegMcChromaAverageHalfH:\n"
    "    pnor $25,$0,$0\n"
    "    psrlh $25,$25,0xf\n"
    "    lw $5,20($4)\n"
    "    lw $6,24($4)\n"
    "    lw $14,0($4)\n"
    "    lw $13,4($4)\n"
    "    addiu $24,$0,1\n"
    "    addiu $12,$0,-1\n"
    "    lw $3,16($4)\n"
    "    sll $2,$3,0x1\n"
    ".LMcChromaAverageHalfHPlane:\n"
    "    lw $7,8($4)\n"
    "    addiu $11,$0,-1\n"
    ".LMcChromaAverageHalfHRow:\n"
    "    ld $8,0($5)\n"
    "    ld $9,0($6)\n"
    "    pcpyld $8,$9,$8\n"
    "    mtsab $13,0\n"
    "    qfsrv $8,$8,$8\n"
    "    pextlb $9,$0,$8\n"
    "    addi $7,$7,-1\n"
    "    addu $5,$5,$3\n"
    "    addu $6,$6,$3\n"
    "    mtsab $24,0\n"
    "    qfsrv $10,$0,$8\n"
    "    pextlb $8,$0,$10\n"
    "    paddh $10,$9,$8\n"
    "    paddh $10,$10,$25\n"
    "    psrlh $10,$10,0x1\n"
    "    lq $8,0($14)\n"
    "    paddh $10,$10,$8\n"
    "    pcgth $9,$10,$0\n"
    "    psrlh $9,$9,0xf\n"
    "    paddh $10,$10,$9\n"
    "    psrlh $10,$10,0x1\n"
    "    sq $10,0($14)\n"
    "    bgtz $7,.LMcChromaAverageHalfHRow\n"
    "    addu $14,$14,$2\n"
    "    addiu $5,$5,320\n"
    "    addiu $6,$6,320\n"
    "    lw $7,12($4)\n"
    "    and $10,$11,$7\n"
    "    bne $10,$0,.LMcChromaAverageHalfHRow\n"
    "    daddu $11,$0,$0\n"
    "    lw $5,20($4)\n"
    "    lw $6,24($4)\n"
    "    lw $14,0($4)\n"
    "    addiu $5,$5,64\n"
    "    addiu $6,$6,64\n"
    "    addiu $14,$14,128\n"
    "    bne $12,$0,.LMcChromaAverageHalfHPlane\n"
    "    daddu $12,$0,$0\n"
    "    jr $31\n"
    "    sll $0,$0,0x0\n"
    "    .type sceMpegMcLumaAverageHalfHV, @function\n"
    "sceMpegMcLumaAverageHalfHV:\n"
    "    pnor $25,$0,$0\n"
    "    psrlh $25,$25,0xf\n"
    "    psllh $25,$25,0x1\n"
    "    lw $5,20($4)\n"
    "    lw $6,24($4)\n"
    "    lw $7,8($4)\n"
    "    lw $14,0($4)\n"
    "    lw $13,4($4)\n"
    "    lw $24,16($4)\n"
    "    addiu $12,$0,1\n"
    "    lq $8,0($5)\n"
    "    lq $9,0($6)\n"
    "    mtsab $13,0\n"
    "    qfsrv $10,$9,$8\n"
    "    qfsrv $15,$8,$9\n"
    "    pextlb $8,$0,$10\n"
    "    pextub $9,$0,$10\n"
    "    addiu $11,$0,-1\n"
    "    mtsab $12,0\n"
    "    qfsrv $15,$15,$10\n"
    "    pextlb $10,$0,$15\n"
    "    pextub $15,$0,$15\n"
    "    paddh $8,$8,$10\n"
    "    beq $7,$0,.LMcLumaAverageHalfHVBelow\n"
    "    paddh $9,$9,$15\n"
    ".LMcLumaAverageHalfHVRow:\n"
    "    addu $5,$5,$24\n"
    "    addu $6,$6,$24\n"
    "    lq $10,0($5)\n"
    "    lq $15,0($6)\n"
    "    mtsab $13,0\n"
    "    qfsrv $2,$15,$10\n"
    "    qfsrv $3,$10,$15\n"
    "    pextlb $10,$0,$2\n"
    "    addi $7,$7,-1\n"
    "    pextub $15,$0,$2\n"
    "    mtsab $12,0\n"
    "    qfsrv $3,$3,$2\n"
    "    pextlb $2,$0,$3\n"
    "    pextub $3,$0,$3\n"
    "    paddh $10,$10,$2\n"
    "    paddh $15,$15,$3\n"
    "    paddh $2,$8,$10\n"
    "    paddh $3,$9,$15\n"
    "    por $8,$10,$0\n"
    "    por $9,$15,$0\n"
    "    paddh $2,$2,$25\n"
    "    paddh $3,$3,$25\n"
    "    psrlh $2,$2,0x2\n"
    "    psrlh $3,$3,0x2\n"
    "    lq $10,0($14)\n"
    "    lq $15,16($14)\n"
    "    paddh $2,$2,$10\n"
    "    paddh $3,$3,$15\n"
    "    pcgth $10,$2,$0\n"
    "    psrlh $10,$10,0xf\n"
    "    paddh $10,$2,$10\n"
    "    psrlh $2,$10,0x1\n"
    "    pcgth $10,$3,$0\n"
    "    psrlh $10,$10,0xf\n"
    "    paddh $10,$3,$10\n"
    "    psrlh $3,$10,0x1\n"
    "    sq $2,0($14)\n"
    "    sll $10,$24,0x1\n"
    "    sq $3,16($14)\n"
    "    bgtz $7,.LMcLumaAverageHalfHVRow\n"
    "    addu $14,$14,$10\n"
    ".LMcLumaAverageHalfHVBelow:\n"
    "    addiu $5,$5,128\n"
    "    addiu $6,$6,128\n"
    "    lw $7,12($4)\n"
    "    and $10,$11,$7\n"
    "    bne $10,$0,.LMcLumaAverageHalfHVRow\n"
    "    daddu $11,$0,$0\n"
    "    jr $31\n"
    "    sll $0,$0,0x0\n"
    "    .type sceMpegMcChromaAverageHalfHV, @function\n"
    "sceMpegMcChromaAverageHalfHV:\n"
    "    pnor $25,$0,$0\n"
    "    psrlh $25,$25,0xf\n"
    "    psllh $25,$25,0x1\n"
    "    lw $5,20($4)\n"
    "    lw $6,24($4)\n"
    "    lw $14,0($4)\n"
    "    lw $13,4($4)\n"
    "    lw $12,16($4)\n"
    "    addiu $24,$0,1\n"
    "    addiu $11,$0,1\n"
    ".LMcChromaAverageHalfHVPlane:\n"
    "    lw $7,8($4)\n"
    "    ld $8,0($5)\n"
    "    ld $9,0($6)\n"
    "    pcpyld $8,$9,$8\n"
    "    mtsab $13,0\n"
    "    qfsrv $8,$8,$8\n"
    "    pextlb $9,$0,$8\n"
    "    addu $5,$5,$12\n"
    "    ori $11,$11,0x8000\n"
    "    mtsab $24,0\n"
    "    qfsrv $10,$0,$8\n"
    "    pextlb $8,$0,$10\n"
    "    beq $7,$0,.LMcChromaAverageHalfHVBelow\n"
    "    paddh $15,$9,$8\n"
    ".LMcChromaAverageHalfHVRow:\n"
    "    addu $6,$6,$12\n"
    "    ld $8,0($5)\n"
    "    ld $9,0($6)\n"
    "    pcpyld $8,$9,$8\n"
    "    mtsab $13,0\n"
    "    qfsrv $8,$8,$8\n"
    "    pextlb $9,$0,$8\n"
    "    addi $7,$7,-1\n"
    "    addu $5,$5,$12\n"
    "    mtsab $24,0\n"
    "    qfsrv $10,$0,$8\n"
    "    pextlb $8,$0,$10\n"
    "    paddh $10,$9,$8\n"
    "    paddh $9,$10,$15\n"
    "    por $15,$10,$0\n"
    "    paddh $10,$9,$25\n"
    "    psrlh $10,$10,0x2\n"
    "    lq $8,0($14)\n"
    "    paddh $10,$10,$8\n"
    "    pcgth $9,$10,$0\n"
    "    psrlh $9,$9,0xf\n"
    "    paddh $10,$10,$9\n"
    "    sll $8,$12,0x1\n"
    "    psrlh $10,$10,0x1\n"
    "    sq $10,0($14)\n"
    "    bgtz $7,.LMcChromaAverageHalfHVRow\n"
    "    addu $14,$14,$8\n"
    ".LMcChromaAverageHalfHVBelow:\n"
    "    psrah $10,$11,0xf\n"
    "    addiu $5,$5,320\n"
    "    lw $7,12($4)\n"
    "    addiu $6,$6,320\n"
    "    and $10,$10,$7\n"
    "    bne $10,$0,.LMcChromaAverageHalfHVRow\n"
    "    andi $11,$11,0x7fff\n"
    "    lw $5,20($4)\n"
    "    lw $6,24($4)\n"
    "    lw $14,0($4)\n"
    "    addiu $5,$5,64\n"
    "    addiu $6,$6,64\n"
    "    addiu $14,$14,128\n"
    "    andi $10,$11,0x1\n"
    "    bne $10,$0,.LMcChromaAverageHalfHVPlane\n"
    "    andi $11,$11,0xfffe\n"
    "    jr $31\n"
    "    sll $0,$0,0x0\n"
    "    sll $0,$0,0x0\n"
    "    .type sceMpegMcAddBlock, @function\n"
    "sceMpegMcAddBlock:\n"
    "    addiu $12,$0,24\n"
    "    lui $10,%hi(sceMpegMcClampLimit)\n"
    "    addiu $10,$10,%lo(sceMpegMcClampLimit)\n"
    "    lq $11,0($10)\n"
    ".LMcAddBlockRow:\n"
    "    lq $8,0($5)\n"
    "    addi $12,$12,-1\n"
    "    lq $13,0($6)\n"
    "    addiu $4,$4,16\n"
    "    lq $9,16($5)\n"
    "    paddh $8,$8,$13\n"
    "    lq $2,16($6)\n"
    "    pminh $8,$8,$11\n"
    "    paddh $9,$9,$2\n"
    "    pmaxh $8,$8,$0\n"
    "    pminh $9,$9,$11\n"
    "    addiu $5,$5,32\n"
    "    pmaxh $9,$9,$0\n"
    "    addiu $6,$6,32\n"
    "    ppacb $10,$9,$8\n"
    "    bne $12,$0,.LMcAddBlockRow\n"
    "    sq $10,-16($4)\n"
    "    jr $31\n"
    "    sll $0,$0,0x0\n"
    "    sll $0,$0,0x0\n"
    "    .type sceMpegMcPutBlock, @function\n"
    "sceMpegMcPutBlock:\n"
    "    addiu $12,$0,24\n"
    "    lui $10,%hi(sceMpegMcClampLimit)\n"
    "    addiu $10,$10,%lo(sceMpegMcClampLimit)\n"
    "    lq $11,0($10)\n"
    ".LMcPutBlockRow:\n"
    "    lq $8,0($5)\n"
    "    addi $12,$12,-1\n"
    "    pminh $8,$8,$11\n"
    "    lq $9,16($5)\n"
    "    pmaxh $8,$8,$0\n"
    "    pminh $9,$9,$11\n"
    "    addiu $5,$5,32\n"
    "    pmaxh $9,$9,$0\n"
    "    addiu $4,$4,16\n"
    "    ppacb $10,$9,$8\n"
    "    bne $12,$0,.LMcPutBlockRow\n"
    "    sq $10,-16($4)\n"
    "    sll $0,$0,0x0\n"
    "    sll $0,$0,0x0\n"
    "sceMpegMcClampLimit:\n"
    "    .word 0xff00ff\n"
    "    .word 0xff00ff\n"
    "    .word 0xff00ff\n"
    "    .word 0xff00ff\n"
    "    jr $31\n"
    "    sll $0,$0,0x0\n"
    "    .set pop\n");

// Rounds half a motion vector component away from zero for positive values, as the dual prime
// derivation does.
static inline int sceMpegHalfRoundUp(int nValue) {
    return (nValue + (nValue > 0)) >> 1;
}

// 0x0060a268
// Derives the dual prime vectors from the transmitted vector and the differential.
static void sceMpegDualPrimeVectors(int *pDmv, const int *pDmVector, int nMvx, int nMvy) {
    if (g_nMpegPictureStructure == kFramePicture) {
        if (g_nMpegTopFieldFirst != 0) {
            pDmv[0] = sceMpegHalfRoundUp(nMvx) + pDmVector[0];
            pDmv[1] = sceMpegHalfRoundUp(nMvy) + pDmVector[1] - 1;
            pDmv[2] = sceMpegHalfRoundUp(3 * nMvx) + pDmVector[0];
            pDmv[3] = sceMpegHalfRoundUp(3 * nMvy) + pDmVector[1] + 1;
        } else {
            pDmv[0] = sceMpegHalfRoundUp(3 * nMvx) + pDmVector[0];
            pDmv[1] = sceMpegHalfRoundUp(3 * nMvy) + pDmVector[1] - 1;
            pDmv[2] = sceMpegHalfRoundUp(nMvx) + pDmVector[0];
            pDmv[3] = sceMpegHalfRoundUp(nMvy) + pDmVector[1] + 1;
        }
        return;
    }
    pDmv[0] = sceMpegHalfRoundUp(nMvx) + pDmVector[0];
    pDmv[1] = sceMpegHalfRoundUp(nMvy) + pDmVector[1];
    if (g_nMpegPictureStructure == kTopField) {
        pDmv[1] = pDmv[1] - 1;
    } else {
        pDmv[1] = pDmv[1] + 1;
    }
}

// Splits a block of nHeight rows starting nOffset rows into a staged macroblock of nSize rows into
// the rows above the boundary and the rows below it. A half-pel vertical prediction reads one row
// more.
static inline void sceMpegSplitMcRows(MpegMcDescriptor *pDescriptor,
                                      int nOffset,
                                      int nHeight,
                                      int nFieldShift,
                                      int nHalfY,
                                      int nSize) {
    if (nHalfY != 0) {
        if (nOffset + (nHeight << nFieldShift) < nSize) {
            pDescriptor->mRows = nHeight;
            pDescriptor->mRowsBelow = 0;
            return;
        }
        pDescriptor->mRows = (nSize >> nFieldShift) - (nOffset >> nFieldShift) - 1;
    } else {
        if (nOffset + (nHeight << nFieldShift) < nSize + 1) {
            pDescriptor->mRows = nHeight;
            pDescriptor->mRowsBelow = 0;
            return;
        }
        pDescriptor->mRows = (nSize >> nFieldShift) - (nOffset >> nFieldShift);
    }
    pDescriptor->mRowsBelow = nHeight - pDescriptor->mRows;
}

// 0x00608cb8
// Queues one prediction of the current macroblock, comprising the reference macroblocks to stage
// and the luma and chroma kernel jobs that read them.
static void sceMpegQueuePrediction(MpegSeqTable *pRef,
                               int nSourceField,
                               int nDestField,
                               int nYOffset,
                               int nHeight,
                               int nX,
                               int nY,
                               int nDx,
                               int nDy,
                               int nFieldShift,
                               int nAverage) {
    MpegMcBuffer *buffer = &g_mpegIpuTable.mBuffers[g_mpegIpuTable.mCurrent];
    const int slot = buffer->mPredictionCount;
    MpegMcDescriptor *luma = &buffer->mLuma[slot];
    MpegMcDescriptor *chroma = &buffer->mChroma[slot];
    const int staging = buffer->mStaging + slot * kStagingBytes;
    const int x = (nDx >> 1) + nX;
    const int y = (nFieldShift != 0 ? (nDy >> 1) << 1 : nDy >> 1) + nY + nYOffset + nSourceField;
    const int column = x >> kLumaRowShift;
    const int row = y >> kLumaRowShift;
    const int macroblock = column * pRef->mMbHeight + row;
    const int lumaOffset = y - (row << kLumaRowShift);
    const int lumaKernel = (nAverage << 2) | ((nDx & 1) << 1) | (nDy & 1);
    const int chromaDx = nDx / 2;
    const int chromaDy = nDy / 2;
    const int chromaX = (chromaDx >> 1) + (nX >> 1);
    const int chromaY = (nFieldShift != 0 ? (chromaDy >> 1) << 1 : chromaDy >> 1) + (nY >> 1) +
        (nYOffset >> 1) + nSourceField;
    const int chromaColumn = chromaX >> kChromaRowShift;
    const int chromaRow = chromaY >> kChromaRowShift;
    const int chromaOffset = chromaY - (chromaRow << kChromaRowShift);
    const int chromaKernel = (nAverage << 2) | ((chromaDx & 1) << 1) | (chromaDy & 1);
    const int chromaSource =
        staging + ((chromaColumn - column) * 2 + (chromaRow - row)) * kMacroblockBytes;

    luma->mShift = x - (column << kLumaRowShift);
    luma->mDest = kMpegMcOutputBase + ((nDestField + nYOffset) << kLumaDestRowShift);
    sceMpegSplitMcRows(luma, lumaOffset, nHeight, nFieldShift, nDy & 1, kLumaSize);
    luma->mStride = kLumaSize << nFieldShift;
    luma->mSourceLeft = staging + (lumaOffset << kLumaRowShift);
    luma->mSourceRight = staging + (lumaOffset << kLumaRowShift) + kStagingRightColumn;

    chroma->mShift = chromaX - (chromaColumn << kChromaRowShift);
    chroma->mDest = kMpegMcOutputBase + kChromaDestOffset +
        ((nDestField + (nYOffset >> 1)) << kChromaDestRowShift);
    sceMpegSplitMcRows(chroma, chromaOffset, nHeight >> 1, nFieldShift, chromaDy & 1, kChromaSize);
    chroma->mStride = kChromaSize << nFieldShift;
    chroma->mSourceLeft = chromaSource + (chromaOffset << kChromaRowShift) + kChromaPlaneOffset;
    chroma->mSourceRight =
        chromaSource + (chromaOffset << kChromaRowShift) + kStagingRightColumn + kChromaPlaneOffset;

    buffer->mRefLeft[slot] = pRef->mBuffer + macroblock * kMacroblockBytes;
    buffer->mLumaKernels[slot] = g_mpegLumaKernels[lumaKernel];
    buffer->mRefRight[slot] = pRef->mBuffer + (macroblock + pRef->mMbHeight) * kMacroblockBytes;
    buffer->mChromaKernels[slot] = g_mpegChromaKernels[chromaKernel];
    buffer->mPredictionCount = slot + 1;
}

// 0x00608608
// Queues every prediction the macroblock's type and motion type call for. A backward prediction
// averages with a forward one queued first.
static void sceMpegQueuePredictions(int nX,
                               int nY,
                               int nMbType,
                               int nMotionType,
                               int pPmv[2][2][2],
                               int pMvfs[2][2],
                               int *pDmVector) {
    int dmv[4];
    MpegSeqTable *fieldRefs[2][2];
    int formed = 0;
    int bottom;
    int other;

    g_mpegIpuTable.mBuffers[g_mpegIpuTable.mCurrent].mPredictionCount = 0;
    if ((nMbType & kMbMotionForward) != 0 || g_nMpegPictureCodingType == kPictureP) {
        if (g_nMpegPictureStructure == kFramePicture) {
            if (nMotionType == kMcFrame || (nMbType & kMbMotionForward) == 0) {
                sceMpegQueuePrediction(g_mpegTables[0], 0, 0, 0, kLumaSize, nX, nY, pPmv[0][0][0],
                                   pPmv[0][0][1], 0, 0);
            } else if (nMotionType == kMcField) {
                sceMpegQueuePrediction(g_mpegTables[0], pMvfs[0][0], 0, 0, kHalfLumaSize, nX, nY,
                                   pPmv[0][0][0], pPmv[0][0][1] >> 1, 1, 0);
                sceMpegQueuePrediction(g_mpegTables[0], pMvfs[1][0], 1, 0, kHalfLumaSize, nX, nY,
                                   pPmv[1][0][0], pPmv[1][0][1] >> 1, 1, 0);
            } else if (nMotionType == kMcDualPrime) {
                sceMpegDualPrimeVectors(dmv, pDmVector, pPmv[0][0][0], pPmv[0][0][1] >> 1);
                sceMpegQueuePrediction(g_mpegTables[0], 0, 0, 0, kHalfLumaSize, nX, nY, pPmv[0][0][0],
                                   pPmv[0][0][1] >> 1, 1, 0);
                sceMpegQueuePrediction(g_mpegTables[0], 1, 0, 0, kHalfLumaSize, nX, nY, dmv[0], dmv[1],
                                   1, 1);
                sceMpegQueuePrediction(g_mpegTables[0], 1, 1, 0, kHalfLumaSize, nX, nY, pPmv[0][0][0],
                                   pPmv[0][0][1] >> 1, 1, 0);
                sceMpegQueuePrediction(g_mpegTables[0], 0, 1, 0, kHalfLumaSize, nX, nY, dmv[2], dmv[3],
                                   1, 1);
            } else {
                sceMpegReportErrorFormatted("(a) invalid motion_type(%d)-0", nMotionType);
            }
        } else {
            bottom = g_nMpegPictureStructure == kBottomField;
            fieldRefs[0][0] = g_mpegTables[3];
            fieldRefs[0][1] = g_mpegTables[6];
            fieldRefs[1][0] = g_mpegTables[4];
            fieldRefs[1][1] = g_mpegTables[7];
            // The second field of a P frame predicts from the first field of the same frame.
            other = 0;
            if (g_nMpegPictureCodingType == kPictureP && g_mpegSecondFieldPending != 0) {
                other = bottom != pMvfs[0][0];
            }
            if (nMotionType == kMcField || (nMbType & kMbMotionForward) == 0) {
                sceMpegQueuePrediction(fieldRefs[other][pMvfs[0][0]], 0, 0, 0, kLumaSize, nX, nY,
                                   pPmv[0][0][0], pPmv[0][0][1], 0, 0);
            } else if (nMotionType == kMc16x8) {
                sceMpegQueuePrediction(fieldRefs[other][pMvfs[0][0]], 0, 0, 0, kHalfLumaSize, nX, nY,
                                   pPmv[0][0][0], pPmv[0][0][1], 0, 0);
                other = 0;
                if (g_nMpegPictureCodingType == kPictureP && g_mpegSecondFieldPending != 0) {
                    other = bottom != pMvfs[1][0];
                }
                sceMpegQueuePrediction(fieldRefs[other][pMvfs[1][0]], 0, 0, kHalfLumaSize,
                                   kHalfLumaSize, nX, nY, pPmv[1][0][0], pPmv[1][0][1], 0, 0);
            } else if (nMotionType == kMcDualPrime) {
                other = g_mpegSecondFieldPending != 0;
                sceMpegDualPrimeVectors(dmv, pDmVector, pPmv[0][0][0], pPmv[0][0][1]);
                sceMpegQueuePrediction(fieldRefs[0][bottom], 0, 0, 0, kLumaSize, nX, nY, pPmv[0][0][0],
                                   pPmv[0][0][1], 0, 0);
                sceMpegQueuePrediction(fieldRefs[other][!bottom], 0, 0, 0, kLumaSize, nX, nY, dmv[0],
                                   dmv[1], 0, 1);
            } else {
                sceMpegReportErrorFormatted("(b) invalid motion_type(%d)-1", nMotionType);
            }
        }
        formed = 1;
    }
    if ((nMbType & kMbMotionBackward) == 0) {
        return;
    }
    if (g_nMpegPictureStructure == kFramePicture) {
        if (nMotionType == kMcFrame) {
            sceMpegQueuePrediction(g_mpegTables[1], 0, 0, 0, kLumaSize, nX, nY, pPmv[0][1][0],
                               pPmv[0][1][1], 0, formed);
        } else {
            sceMpegQueuePrediction(g_mpegTables[1], pMvfs[0][1], 0, 0, kHalfLumaSize, nX, nY,
                               pPmv[0][1][0], pPmv[0][1][1] >> 1, 1, formed);
            sceMpegQueuePrediction(g_mpegTables[1], pMvfs[1][1], 1, 0, kHalfLumaSize, nX, nY,
                               pPmv[1][1][0], pPmv[1][1][1] >> 1, 1, formed);
        }
    } else if (nMotionType == kMcField) {
        sceMpegQueuePrediction(pMvfs[0][1] != 0 ? g_mpegTables[7] : g_mpegTables[4], 0, 0, 0,
                           kLumaSize, nX, nY, pPmv[0][1][0], pPmv[0][1][1], 0, formed);
    } else if (nMotionType == kMc16x8) {
        sceMpegQueuePrediction(pMvfs[0][1] != 0 ? g_mpegTables[7] : g_mpegTables[4], 0, 0, 0,
                           kHalfLumaSize, nX, nY, pPmv[0][1][0], pPmv[0][1][1], 0, formed);
        sceMpegQueuePrediction(pMvfs[1][1] != 0 ? g_mpegTables[7] : g_mpegTables[4], 0, 0,
                           kHalfLumaSize, kHalfLumaSize, nX, nY, pPmv[1][1][0], pPmv[1][1][1], 0,
                           formed);
    } else {
        sceMpegReportErrorFormatted("(c) invalid motion_type(%d)-2", nMotionType);
    }
}

// 0x006082d0
// Prepares the reconstruction of one macroblock. The routine stages its references by DMA to the
// scratchpad, records whether it is intra, and locates it in the destination frame or field.
static int sceMpegStartMotionCompensation(int nAddress,
                              int nIncrement,
                              int nMbType,
                              int nMotionType,
                              int pPmv[2][2][2],
                              int pMvfs[2][2],
                              int *pDmVector) {
    volatile unsigned int *pToSprChcr = (volatile unsigned int *)(uintptr_t)kToSprChcrAddress;
    const int row = nAddress / g_nMpegMbWidth;
    const int column = nAddress % g_nMpegMbWidth;
    const int intra = nMbType & kMbIntra;
    MpegMcBuffer *buffer;
    MpegSeqTable *frame;
    volatile unsigned long long *pTag;
    unsigned int left;
    unsigned int right;
    int count;
    int i;

    if (intra != 0) {
        while (((*pToSprChcr >> 8) & 1) != 0) {
        }
        g_mpegIpuTable.mBuffers[g_mpegIpuTable.mCurrent].mDmaPending = 0;
    } else {
        if ((unsigned int)(nMotionType - 1) >= 3u) {
            sceMpegReportErrorFormatted("Invalid modion type -- ignored(%d)", nMotionType);
            g_nMpegDecodeError = 1;
            return 0;
        }
        sceMpegQueuePredictions(column << kLumaRowShift, row << kLumaRowShift, nMbType, nMotionType,
                           pPmv, pMvfs, pDmVector);
        while (((*pToSprChcr >> 8) & 1) != 0) {
        }
        pTag = (volatile unsigned long long *)(uintptr_t)(
            ((unsigned int)(uintptr_t)g_mpegMcChain & kPhysicalAddressMask) | kUncachedSegment);
        count = g_mpegIpuTable.mBuffers[g_mpegIpuTable.mCurrent].mPredictionCount;
        for (i = 0; i < count; ++i) {
            buffer = &g_mpegIpuTable.mBuffers[g_mpegIpuTable.mCurrent];
            left = (unsigned int)buffer->mRefLeft[i] & kPhysicalAddressMask;
            right = (unsigned int)buffer->mRefRight[i] & kPhysicalAddressMask;
            pTag[0] = (unsigned long long)left << 32 | kDmaTagRef | kStagingPairQwords;
            pTag[2] = (unsigned long long)right << 32 |
                (i == count - 1 ? kDmaTagRefe : kDmaTagRef) | kStagingPairQwords;
            pTag += 4;
        }
        __asm__ __volatile__("sync" : : : "memory");
        buffer = &g_mpegIpuTable.mBuffers[g_mpegIpuTable.mCurrent];
        *(volatile int *)(uintptr_t)kToSprSadrAddress = buffer->mStaging;
        *(volatile int *)(uintptr_t)kToSprTadrAddress = (int)(uintptr_t)g_mpegMcChain;
        *(volatile int *)(uintptr_t)kToSprQwcAddress = 0;
        *pToSprChcr = kDmaChcrChainToSpr;
        buffer->mDmaPending = 1;
    }
    buffer = &g_mpegIpuTable.mBuffers[g_mpegIpuTable.mCurrent];
    buffer->mFollowsCoded = (nIncrement == 1 && (nMbType & kMbPattern) != 0) ? 1 : 0;
    buffer->mIntra = intra;
    if (g_nMpegPictureStructure == kFramePicture) {
        frame = g_mpegCurrentTables[kCurrentFrame];
    } else if (g_nMpegPictureStructure == kBottomField) {
        frame = g_mpegCurrentTables[kCurrentBottomField];
    } else {
        frame = g_mpegCurrentTables[kCurrentTopField];
    }
    buffer->mOutput = frame->mBuffer + (column * frame->mMbHeight + row) * kMacroblockBytes;
    return 1;
}

// 0x006090d8
// Runs the queued kernels of one buffer and writes its macroblock to the frame.
static void sceMpegFinishMacroblock(int nBuffer) {
    MpegMcBuffer *buffer = &g_mpegIpuTable.mBuffers[nBuffer];
    int i;

    if (buffer->mDmaPending != 0) {
        for (i = 0; i < buffer->mPredictionCount; ++i) {
            buffer->mLumaKernels[i](&buffer->mLuma[i]);
            buffer->mChromaKernels[i](&buffer->mChroma[i]);
        }
    }
    if (buffer->mIntra != 0 && buffer->mNotCoded != 0) {
        sceMpegRaiseError("intra && skip MB");
    }
    if (buffer->mIntra != 0) {
        sceMpegMcPutBlock(buffer->mOutput, buffer->mCoefficients);
    } else if (buffer->mNotCoded == 0) {
        sceMpegMcAddBlock(buffer->mOutput, kMpegMcOutputBase, buffer->mCoefficients);
    } else {
        sceMpegMcPutBlock(buffer->mOutput, kMpegMcOutputBase);
    }
}

// Reloads the bit reader words from the IPU after a command.
static inline void sceMpegReloadShiftWords(void) {
    const long long top = *(volatile long long *)(uintptr_t)kIpuTopAddress;
    const unsigned int bitPointer = *(volatile unsigned int *)(uintptr_t)kIpuBitPointerAddress;

    g_mpegShiftAccum = (int)top;
    if (top < 0) {
        g_mpegShiftBudget = (int)((0u - (bitPointer & kIpuBitPointerMask)) & kIpuBitPointerMask);
    } else {
        g_mpegShiftBudget = kIpuBitsPerWord;
    }
}

// 0x0060a060
// Waits for the block the IPU is decoding to drain. On a decode error the IPU and its output
// channel are reset and the routine reports 0.
static int sceMpegWaitBlockDecode(void) {
    volatile unsigned int *pControl = (volatile unsigned int *)(uintptr_t)kIpuControlAddress;
    volatile unsigned int *pFromQwc = (volatile unsigned int *)(uintptr_t)kIpuFromQwcAddress;
    volatile unsigned int *pEnableWrite =
        (volatile unsigned int *)(uintptr_t)kDmacEnableWriteAddress;
    volatile unsigned int *pEnableRead = (volatile unsigned int *)(uintptr_t)kDmacEnableReadAddress;
    StreamEntry entry;

    sceMpegWaitIpuIdle();
    if (*pFromQwc != 0 && (*pControl & kIpuErrorCodeDetected) == 0) {
        do {
            // The IPU stalls for input once the input channel is idle and empty.
            if (*(volatile unsigned int *)(uintptr_t)kIpuToQwcAddress == 0 &&
                (*(volatile unsigned int *)(uintptr_t)kIpuToChcrAddress & kDmaChcrStart) == 0) {
                sceMpegReportNoData(g_decoderInstance);
            }
        } while (*pFromQwc != 0 && (*pControl & kIpuErrorCodeDetected) == 0);
    }
    sceMpegReloadShiftWords();
    if ((*pControl & kIpuErrorCodeDetected) == 0) {
        return 1;
    }
    sceMpegRaiseError("Error code detected(BDEC)");
    entry.key = 2;
    entry.templateBits = 0;
    entry.callback = NULL;
    entry.data = NULL;
    sceMpegInvokeCallbackSlot(g_decoderInstance, &entry);
    *pControl = kIpuReset;
    entry.key = 3;
    sceMpegInvokeCallbackSlot(g_decoderInstance, &entry);
    DIntr();
    *pEnableWrite = *pEnableRead | kDmacSuspend;
    *(volatile unsigned int *)(uintptr_t)kIpuFromChcrAddress = 0;
    // The store sits in the delay slot of the call that re-enables interrupts and therefore lands
    // first.
    *pEnableWrite = *pEnableRead & ~kDmacSuspend;
    EIntr();
    *pFromQwc = 0;
    return 0;
}

// 0x0060b418
// Issues a VDEC command on table nTable and returns the decoded symbol. A zero result word marks a
// VLC error in the error flag.
static int sceMpegDecodeVlc(int nTable) {
    volatile unsigned int *pControl = (volatile unsigned int *)(uintptr_t)kIpuControlAddress;
    volatile long long *pResult = (volatile long long *)(uintptr_t)kIpuCommandAddress;
    unsigned int command;
    unsigned int count;
    long long result;

    if ((*pControl & kIpuBusyMask) == kIpuBusyValue) {
        count = 0;
        do {
            if (count++ >= kIpuWatchdogLimit) {
                sceMpegReportNoData(g_decoderInstance);
                count = 0;
            }
        } while ((*pControl & kIpuBusyMask) == kIpuBusyValue);
    }
    command = ((unsigned int)nTable << kIpuVdecTableShift) | kIpuCommandVdec;
    *(volatile unsigned int *)(uintptr_t)kIpuCommandAddress = command;
    result = *pResult;
    // Yes, the binary shifts the command arithmetically here, unlike sceMpegGetBits().
    g_mpegIpuBusyFlag = (int)g_mpegNibbleTable[(int)command >> 28];
    count = 0;
    while (result < 0) {
        if (count++ >= kIpuWatchdogLimit) {
            sceMpegReportNoData(g_decoderInstance);
            count = 0;
        }
        result = *pResult;
    }
    sceMpegReloadShiftWords();
    g_nMpegDecodeError = (int)result == 0;
    return (short)result;
}

// 0x0060a248
static int sceMpegDecodeDmVector(void) {
    return sceMpegDecodeVlc(kVdecDmVector);
}

// 0x0060a3f0
// Decodes macroblock_address_increment, folding escapes and skipping stuffing.
static int sceMpegMacroblockAddressIncrement(void) {
    int increment = 0;
    int code;
    int peek;

    for (;;) {
        code = sceMpegDecodeVlc(kVdecMacroblockAddressIncrement);
        if (code == kMbaiStuffing) {
            continue;
        }
        if (code == kMbaiEscape) {
            increment += kMbaiEscapeIncrement;
            continue;
        }
        if (code != 0) {
            return increment + code;
        }
        // The peek happens before the MPEG-2 test.
        peek = sceMpegPeekBits(kMbaiPeekBits);
        if (g_nMpegIsMpeg2 != 0 && peek == kMpeg1StuffingCode) {
            sceMpegSkipBits(kMbaiPeekBits);
            continue;
        }
        sceMpegReportErrorFormatted("Invalid macroblock_address_increment code(0x%08x)", code);
        g_nMpegDecodeError = 1;
        return 1;
    }
}

// 0x0060b9f0
// Reads the rest of the slice header. The image always returns zero. The caller uses the result as
// the slice vertical position extension.
static int sceMpegSliceHeader(void) {
    g_nMpegQuantiserScaleCode = sceMpegGetBits(5);
    if (sceMpegGetBits(1) != 0) {
        g_nMpegIntraSlice = sceMpegGetBits(1);
        sceMpegSkipBits(7);
        sceMpegSkipExtraInformation();
    } else {
        g_nMpegIntraSlice = 0;
    }
    return 0;
}

// 0x0060a628
// Starts a slice. The routine finds its start code, reads its header and first increment, and
// resets the predictors. The macroblock count argument is not read.
static int sceMpegStartSlice(int nMacroblockCount,
                              int *pAddress,
                              int *pIncrement,
                              int pPmv[2][2][2]) {
    int code;
    int extension;
    int increment;
    int r;
    int s;
    int t;

    (void)nMacroblockCount;
    g_nMpegDecodeError = 0;
    sceMpegNextStartCode();
    code = sceMpegPeekBits(32);
    if ((unsigned int)(code - kSliceStartCodeFirst) >= (unsigned int)kSliceStartCodeCount) {
        sceMpegReportErrorFormatted("slice_start_code(0x%08x) out of range", code);
        return kSliceSkipPicture;
    }
    sceMpegSkipBits(32);
    extension = sceMpegSliceHeader();
    increment = sceMpegMacroblockAddressIncrement();
    *pIncrement = increment;
    if (g_nMpegDecodeError != 0) {
        sceMpegRaiseError("_sliceA0(): error happens");
        return kSliceError;
    }
    *pAddress = ((extension << 7) + (code & 0xff) - 1) * g_nMpegMbWidth + increment - 1;
    *pIncrement = 1;
    g_nMpegResetDcPredictor = 1;
    for (r = 0; r < 2; ++r) {
        for (s = 0; s < 2; ++s) {
            for (t = 0; t < 2; ++t) {
                pPmv[r][s][t] = 0;
            }
        }
    }
    return kSliceDone;
}

// 0x0060af48
// Applies one decoded motion vector component to its predictor.
static void sceMpegDecodeMotionComponent(int *pPred,
                               int nRSize,
                               int nMotionCode,
                               int nMotionResidual,
                               int nFullPel) {
    const int limit = 16 << nRSize;
    int vector = *pPred;

    if (nFullPel != 0) {
        vector >>= 1;
    }
    if (nMotionCode > 0) {
        vector += ((nMotionCode - 1) << nRSize) + nMotionResidual + 1;
        if (vector >= limit) {
            vector -= limit << 1;
        }
    } else if (nMotionCode < 0) {
        vector -= ((~nMotionCode) << nRSize) + nMotionResidual + 1;
        if (vector < -limit) {
            vector += limit << 1;
        }
    }
    *pPred = nFullPel != 0 ? vector << 1 : vector;
}

// 0x0060b150
// Decodes one motion vector into pPmv, with its dual prime differential when nDmv is set.
static void sceMpegMotionVector(int *pPmv,
                               int *pDmVector,
                               int nHRSize,
                               int nVRSize,
                               int nDmv,
                               int nMvScale,
                               int nFullPel) {
    int code;
    int residual;

    code = sceMpegDecodeVlc(kVdecMotionCode);
    residual = (nHRSize != 0 && code != 0) ? sceMpegGetBits(nHRSize) : 0;
    sceMpegDecodeMotionComponent(&pPmv[0], nHRSize, code, residual, nFullPel);
    if (nDmv != 0) {
        pDmVector[0] = sceMpegDecodeDmVector();
    }
    code = sceMpegDecodeVlc(kVdecMotionCode);
    residual = (nVRSize != 0 && code != 0) ? sceMpegGetBits(nVRSize) : 0;
    if (nMvScale != 0) {
        pPmv[1] >>= 1;
    }
    sceMpegDecodeMotionComponent(&pPmv[1], nVRSize, code, residual, nFullPel);
    if (nMvScale != 0) {
        pPmv[1] <<= 1;
    }
    if (nDmv != 0) {
        pDmVector[1] = sceMpegDecodeDmVector();
    }
}

// 0x0060afd0
// Decodes the motion vectors of direction s.
static void sceMpegMotionVectors(int pPmv[2][2][2],
                               int *pDmVector,
                               int pMvfs[2][2],
                               int s,
                               int nCount,
                               int nMvFormat,
                               int nHRSize,
                               int nVRSize,
                               int nDmv,
                               int nMvScale) {
    if (nCount == 1) {
        if (nMvFormat == 0 && nDmv == 0) {
            pMvfs[1][s] = pMvfs[0][s] = sceMpegGetBits(1);
        }
        sceMpegMotionVector(pPmv[0][s], pDmVector, nHRSize, nVRSize, nDmv, nMvScale, 0);
        pPmv[1][s][0] = pPmv[0][s][0];
        pPmv[1][s][1] = pPmv[0][s][1];
        return;
    }
    pMvfs[0][s] = sceMpegGetBits(1);
    sceMpegMotionVector(pPmv[0][s], pDmVector, nHRSize, nVRSize, nDmv, nMvScale, 0);
    pMvfs[1][s] = sceMpegGetBits(1);
    sceMpegMotionVector(pPmv[1][s], pDmVector, nHRSize, nVRSize, nDmv, nMvScale, 0);
}

// 0x0060aa20
// Decodes one coded macroblock's header and vectors and starts the IPU on its block data. Returns
// 0 after an error.
static int sceMpegCodedMacroblock(int *pMbType,
                              int *pMotionType,
                              int *pDctType,
                              int pPmv[2][2][2],
                              int pMvfs[2][2],
                              int *pDmVector) {
    volatile unsigned int *pControl = (volatile unsigned int *)(uintptr_t)kIpuControlAddress;
    MpegMcBuffer *buffer;
    int type;
    int motionType;
    int count;
    int format;
    int dmv;
    int mvScale;

    *pControl = (*pControl & ~(kIpuPictureTypeMask << kIpuPictureTypeShift)) |
        ((unsigned int)g_nMpegPictureCodingType << kIpuPictureTypeShift);
    type = sceMpegDecodeVlc(kVdecMacroblockType);
    *pMbType = type;
    if (type == 0) {
        sceMpegRaiseError("Invalid macroblock_type code: 0");
        g_nMpegDecodeError = 1;
        return 0;
    }
    if ((type & (kMbMotionForward | kMbMotionBackward)) != 0) {
        if (g_nMpegPictureStructure == kFramePicture && g_nMpegFramePredFrameDct != 0) {
            *pMotionType = kMcFrame;
        } else {
            *pMotionType = sceMpegGetBits(2);
        }
    } else if ((type & kMbIntra) != 0 && g_nMpegConcealmentMotionVectors != 0) {
        *pMotionType = g_nMpegPictureStructure == kFramePicture ? kMcFrame : kMcField;
    }
    motionType = *pMotionType;
    if (g_nMpegPictureStructure == kFramePicture) {
        count = motionType == kMcField ? 2 : 1;
        format = motionType == kMcFrame;
    } else {
        count = motionType == kMc16x8 ? 2 : 1;
        format = 0;
    }
    dmv = motionType == kMcDualPrime;
    mvScale = 0;
    if (format == 0) {
        mvScale = g_nMpegPictureStructure == kFramePicture;
    }
    if (g_nMpegPictureStructure == kFramePicture && g_nMpegFramePredFrameDct == 0 && (type & (kMbIntra | kMbPattern)) != 0) {
        *pDctType = sceMpegGetBits(1);
    } else {
        *pDctType = 0;
    }
    if ((type & kMbQuant) != 0) {
        g_nMpegQuantiserScaleCode = sceMpegGetBits(5);
    }
    if ((type & kMbMotionForward) != 0 || ((type & kMbIntra) != 0 && g_nMpegConcealmentMotionVectors != 0)) {
        if (g_nMpegIsMpeg2 != 0) {
            sceMpegMotionVectors(pPmv, pDmVector, pMvfs, 0, count, format, g_anMpegFCodes[0] - 1,
                               g_anMpegFCodes[1] - 1, dmv, mvScale);
        } else {
            sceMpegMotionVector(pPmv[0][0], pDmVector, g_nMpegForwardFCode - 1, g_nMpegForwardFCode - 1, 0, 0,
                               g_nMpegFullPelForwardVector);
        }
    }
    if (g_nMpegDecodeError != 0) {
        return 0;
    }
    if ((type & kMbMotionBackward) != 0) {
        if (g_nMpegIsMpeg2 != 0) {
            sceMpegMotionVectors(pPmv, pDmVector, pMvfs, 1, count, format, g_anMpegFCodes[2] - 1,
                               g_anMpegFCodes[3] - 1, 0, mvScale);
        } else {
            sceMpegMotionVector(pPmv[0][1], pDmVector, g_nMpegBackwardFCode - 1, g_nMpegBackwardFCode - 1, 0, 0,
                               g_nMpegFullPelBackwardVector);
        }
    }
    if (g_nMpegDecodeError != 0) {
        return 0;
    }
    if ((type & kMbIntra) != 0 && g_nMpegConcealmentMotionVectors != 0) {
        sceMpegSkipBits(1); // The marker bit after the concealment vectors.
    }
    buffer = &g_mpegIpuTable.mBuffers[g_mpegIpuTable.mCurrent];
    if ((type & (kMbIntra | kMbPattern)) != 0) {
        *(volatile unsigned int *)(uintptr_t)kIpuFromMadrAddress =
            ((unsigned int)buffer->mCoefficients & kPhysicalAddressMask) | kDmaScratchpadFlag;
        *(volatile unsigned int *)(uintptr_t)kIpuFromQwcAddress = kCoefficientQwords;
        *(volatile unsigned int *)(uintptr_t)kIpuFromChcrAddress = kDmaChcrStart;
        sceMpegWaitIpuIdle();
        sceMpegIssueIpuCommand(((unsigned int)(type & kMbIntra) << kBdecIntraShift) |
                           ((unsigned int)g_nMpegQuantiserScaleCode << kBdecQuantiserShift) |
                           ((unsigned int)g_nMpegResetDcPredictor << kBdecResetDcShift) | kIpuCommandBdec |
                           ((unsigned int)*pDctType << kBdecDctTypeShift));
    } else {
        buffer->mNotCoded = 1;
    }
    g_nMpegResetDcPredictor = 0;
    if (g_nMpegDecodeError != 0) {
        return 0;
    }
    if ((type & kMbIntra) == 0) {
        g_nMpegResetDcPredictor = 1;
    } else if (g_nMpegConcealmentMotionVectors == 0) {
        pPmv[0][0][0] = 0;
        pPmv[0][0][1] = 0;
        pPmv[0][1][0] = 0;
        pPmv[0][1][1] = 0;
        pPmv[1][0][0] = 0;
        pPmv[1][0][1] = 0;
        pPmv[1][1][0] = 0;
        pPmv[1][1][1] = 0;
    }
    if (g_nMpegPictureCodingType == kPictureP && (type & (kMbIntra | kMbMotionForward)) == 0) {
        pPmv[0][0][0] = 0;
        pPmv[0][0][1] = 0;
        pPmv[1][0][0] = 0;
        pPmv[1][0][1] = 0;
        if (g_nMpegPictureStructure == kFramePicture) {
            *pMotionType = kMcFrame;
        } else {
            *pMotionType = kMcField;
            pMvfs[0][0] = g_nMpegPictureStructure == kBottomField;
        }
    }
    return 1;
}

// 0x0060a950
// Sets up a skipped macroblock. Returns 0 in an I picture, which may not skip.
static int sceMpegSkippedMacroblock(int pPmv[2][2][2], int *pMotionType, int pMvfs[2][2], int *pMbType) {
    int result = 1;

    g_nMpegResetDcPredictor = 1;
    g_mpegIpuTable.mBuffers[g_mpegIpuTable.mCurrent].mNotCoded = 1;
    if (g_nMpegPictureCodingType == kPictureP) {
        pPmv[0][0][0] = 0;
        pPmv[1][0][1] = 0;
        pPmv[1][0][0] = 0;
        pPmv[0][0][1] = 0;
    }
    if (g_nMpegPictureStructure == kFramePicture) {
        *pMotionType = kMcFrame;
    } else {
        *pMotionType = kMcField;
        pMvfs[0][0] = g_nMpegPictureStructure == kBottomField;
        pMvfs[0][1] = g_nMpegPictureStructure == kBottomField;
    }
    if (g_nMpegPictureCodingType == kPictureI) {
        sceMpegRaiseError("skiped macroblock in I picure is not allowed");
        result = 0;
    }
    *pMbType &= ~kMbIntra;
    return result;
}

// 0x0060a750
// Decodes one slice. Each macroblock's kernels run while the IPU decodes the next one. Returns one
// of kSliceDone, kSliceError, kSliceSkipPicture, or kSliceNext.
static int sceMpegDecodeSlice(int nCounter, int nMacroblockCount) {
    int pmv[2][2][2];
    int address = 0;
    int increment = 0;
    // The image does not initialise these locals. A value is read only after the macroblock that
    // sets it.
    int mvfs[2][2] = {{0, 0}, {0, 0}};
    int dmVector[2] = {0, 0};
    int mbType = 0;
    int motionType = 0;
    int dctType = 0;
    int result;

    (void)nCounter; // Passed down from the picture path and never read.
    result = sceMpegStartSlice(nMacroblockCount, &address, &increment, pmv);
    if (result != kSliceDone) {
        return result;
    }
    g_nMpegDecodeError = 0;
    for (;;) {
        if (address >= nMacroblockCount) {
            return kSliceDone;
        }
        g_mpegIpuTable.mBuffers[g_mpegIpuTable.mCurrent].mNotCoded = 0;
        if (sceMpegWaitBlockDecode() == 0) {
            return kSliceSkipPicture;
        }
        if (increment == 0) {
            if (sceMpegPeekBits(23) == 0 || g_nMpegDecodeError != 0) {
                g_nMpegDecodeError = 0;
                return kSliceNext;
            }
            increment = sceMpegMacroblockAddressIncrement();
            if (g_nMpegDecodeError != 0) {
                g_nMpegDecodeError = 0;
                return kSliceError;
            }
        }
        if (address >= nMacroblockCount) {
            sceMpegRaiseError("Too many macroblocks in picture");
            return kSliceSkipPicture;
        }
        if (increment == 1) {
            if (sceMpegCodedMacroblock(&mbType, &motionType, &dctType, pmv, mvfs, dmVector) == 0) {
                g_nMpegDecodeError = 0;
                return kSliceError;
            }
        } else if (sceMpegSkippedMacroblock(pmv, &motionType, mvfs, &mbType) == 0) {
            g_nMpegDecodeError = 0;
            return kSliceSkipPicture;
        }
        if (sceMpegStartMotionCompensation(address, increment, mbType, motionType, pmv, mvfs, dmVector) == 0) {
            g_nMpegDecodeError = 0;
            return kSliceSkipPicture;
        }
        if (address != 0) {
            sceMpegFinishMacroblock(g_mpegIpuTable.mCurrent ^ 1);
        }
        ++address;
        --increment;
        g_mpegIpuTable.mCurrent ^= 1;
    }
}

// 0x0060a500
// Decodes the slices of one picture and finishes its last macroblock. Returns 1 when the picture
// decoded completely.
static int sceMpegDecodeSlices(int nCounter) {
    volatile unsigned int *pToSprChcr = (volatile unsigned int *)(uintptr_t)kToSprChcrAddress;
    int count = g_nMpegMbWidth * g_nMpegMbHeight;
    int result;

    g_mpegIpuTable.mPictureResetWord = 0;
    g_mpegIpuTable.mCurrent = 0;
    if (g_nMpegPictureStructure != kFramePicture) {
        count >>= 1;
    }
    do {
        result = sceMpegDecodeSlice(nCounter, count);
    } while (result == kSliceError || result == kSliceNext);
    sceMpegWaitIpuIdle();
    if (sceMpegWaitBlockDecode() == 0) {
        result = kSliceSkipPicture;
    }
    while (((*pToSprChcr >> 8) & 1) != 0) {
    }
    if (result == kSliceDone) {
        sceMpegFinishMacroblock(g_mpegIpuTable.mCurrent == 0);
    }
    if ((unsigned int)(result - 1) < 2u) {
        sceMpegRaiseError("= Skip to the next picture =");
    }
    return result == kSliceDone;
}

// Output states of the work area's +0x08 word.
enum {
    kOutputIdle = 0,
    kOutputDecoding = 1,
    kOutputShown = 2,
};

// sceMpegNextPictureHeader() results the picture path dispatches on. The picture types are
// picture_coding_type.
enum {
    kPictureSequenceEnd = 0,
    kPictureCodingIntra = 1,
    kPictureCodingDc = 4,
    kPictureDispatchCount = 5,
};

// The buffer check reports a picture larger than the caller's buffer in this many bytes.
enum {
    kBufferMessageSize = 256,
};

// Pending time stamp states of the work area's +0xf8 word.
enum {
    kPendingStampArmed = 1,
    kPendingStampReady = 2,
};

static inline volatile unsigned int *MpegRegister(unsigned int nAddress) {
    return (volatile unsigned int *)(uintptr_t)nAddress;
}

static inline int MpegIpuBusy(void) {
    return (*MpegRegister(kIpuControlAddress) & kIpuBusyValue) != 0;
}

// Reports a callback that has only its type. The image does not initialise the rest of the stack
// entry, and the reconstruction zeroes it as sceMpegReportNoData() does.
static inline int MpegInvokeTypedCallback(void *pDecoder, int nType) {
    StreamEntry entry;

    entry.key = (unsigned long long)nType;
    entry.templateBits = 0;
    entry.callback = NULL;
    entry.data = NULL;
    return sceMpegInvokeCallbackSlot(pDecoder, &entry);
}

static inline MpegWork *MpegInstanceWork(void) {
    return (MpegWork *)((sceMpeg *)g_decoderInstance)->pContext;
}

// 0x00638598
static int sceMpegToIpuHandler(int nChannel) {
    unsigned int remaining;
    unsigned int address;

    (void)nChannel;
    *MpegRegister(kDmacStatusAddress) = kDmacStatusToIpu;
    ++g_nToIpuInterrupts;
    remaining = g_nToIpuRemainingQwc;
    if (remaining == 0) {
        return 1;
    }
    address = g_nToIpuNextAddress;
    if (remaining > kToIpuChunkQwc) {
        *MpegRegister(kIpuToMadrAddress) = address;
        *MpegRegister(kIpuToQwcAddress) = kToIpuChunkQwc;
        *MpegRegister(kIpuToChcrAddress) = kDmaChcrStartFromMemory;
        g_nToIpuNextAddress = (address + kToIpuChunkBytes) & kPhysicalAddressMask;
        g_nToIpuRemainingQwc = remaining - kToIpuChunkQwc;
    } else {
        *MpegRegister(kIpuToMadrAddress) = address;
        *MpegRegister(kIpuToQwcAddress) = remaining;
        *MpegRegister(kIpuToChcrAddress) = kDmaChcrStartFromMemory;
        g_nToIpuRemainingQwc = 0;
    }
    return 0;
}

// 0x00638290
static int sceMpegFromIpuHandler(int nChannel) {
    unsigned int address;
    int remaining;

    (void)nChannel;
    *MpegRegister(kDmacStatusAddress) = kDmacStatusFromIpu;
    ++g_nFromIpuInterrupts;
    if (*MpegRegister(kIpuFromQwcAddress) != 0 ||
        (*MpegRegister(kIpuFromChcrAddress) & kDmaChcrStart) != 0) {
        g_nColourConvertError = 1;
        return 0; // The image skips the interrupt enable on this path.
    }
    if (g_nFromIpuInterrupts < g_nColourConvertChunks - 1) {
        address = g_nColourConvertNextAddress;
        *MpegRegister(kIpuFromMadrAddress) = address;
        *MpegRegister(kIpuFromQwcAddress) = kColourConvertChunkQwc;
        *MpegRegister(kIpuFromChcrAddress) = kDmaChcrStart;
        *MpegRegister(kIpuCommandAddress) =
            kIpuCommandColourConvert | kColourConvertChunkMacroblocks;
        g_nColourConvertNextAddress = (address + kColourConvertChunkBytes) & kPhysicalAddressMask;
    } else if (g_nFromIpuInterrupts == g_nColourConvertChunks - 1) {
        remaining = g_nColourConvertRemaining -
                    g_nFromIpuInterrupts * kColourConvertChunkMacroblocks;
        g_nColourConvertRemaining = remaining;
        *MpegRegister(kIpuFromMadrAddress) = g_nColourConvertNextAddress;
        *MpegRegister(kIpuFromQwcAddress) = (unsigned int)remaining << kMacroblockQwcShift;
        *MpegRegister(kIpuFromChcrAddress) = kDmaChcrStart;
        *MpegRegister(kIpuCommandAddress) = (unsigned int)remaining | kIpuCommandColourConvert;
    }
    ExitHandler();
    return 0;
}

// 0x006381a8
static void sceMpegConvertColours(unsigned int nDestination, int nMacroblocks) {
    const unsigned int qwc = (unsigned int)nMacroblocks << kMacroblockQwcShift;

    while (MpegIpuBusy()) {
    }
    *MpegRegister(kIpuFromMadrAddress) = nDestination & kPhysicalAddressMask;
    *MpegRegister(kIpuFromQwcAddress) = qwc;
    *MpegRegister(kIpuFromChcrAddress) = kDmaChcrStart;
    sceMpegIssueIpuCommand((unsigned int)nMacroblocks | kIpuCommandColourConvert);
    MpegInvokeTypedCallback(g_decoderInstance, kMpegCbBackground);
    while (((*MpegRegister(kIpuFromChcrAddress) >> kDmaChcrStartBit) & 1) != 0) {
    }
    while (MpegIpuBusy()) {
    }
}

// 0x006383d8
static void sceMpegConvertColoursChunked(unsigned int nDestination, int nMacroblocks) {
    int handlerId;

    g_nColourConvertRemaining = nMacroblocks;
    g_nColourConvertNextAddress = (nDestination + kColourConvertChunkBytes) & kPhysicalAddressMask;
    g_nFromIpuInterrupts = 0;
    g_nColourConvertError = 0;
    g_nColourConvertChunks = nMacroblocks / kColourConvertChunkMacroblocks + 1;
    while (MpegIpuBusy()) {
    }
    handlerId = AddDmacHandler(kDmaChannelFromIpu, sceMpegFromIpuHandler, 0);
    *MpegRegister(kDmacStatusAddress) = kDmacStatusFromIpu;
    EnableDmac(kDmaChannelFromIpu);
    *MpegRegister(kIpuFromMadrAddress) = nDestination & kPhysicalAddressMask;
    *MpegRegister(kIpuFromQwcAddress) = kColourConvertChunkQwc;
    *MpegRegister(kIpuFromChcrAddress) = kDmaChcrStart;
    *MpegRegister(kIpuCommandAddress) = kIpuCommandColourConvert | kColourConvertChunkMacroblocks;
    MpegInvokeTypedCallback(g_decoderInstance, kMpegCbBackground);
    while (g_nFromIpuInterrupts < g_nColourConvertChunks) {
    }
    if (g_nColourConvertError != 0) {
        sceMpegRaiseError("CSC handler error\n");
    }
    while (MpegIpuBusy()) {
    }
    DisableDmac(kDmaChannelFromIpu);
    RemoveDmacHandler(kDmaChannelFromIpu, handlerId);
}

// 0x00638670
static void sceMpegConvertPicture(MpegSeqTable *pTable) {
    MpegWork *work;
    int macroblocks;
    unsigned int qwc;
    unsigned int address;
    int handlerId;

    work = MpegInstanceWork();
    macroblocks = pTable->mMbWidth * pTable->mMbHeight;
    MpegInvokeTypedCallback(g_decoderInstance, kMpegCbStopDma);
    if ((*MpegRegister(kIpuControlAddress) & kIpuControlErrorBit) != 0) {
        *MpegRegister(kIpuControlAddress) = kIpuControlReset;
    }
    while (MpegIpuBusy()) {
    }
    sceMpegIssueIpuCommand(kIpuCommandBitstreamClear);
    while (MpegIpuBusy()) {
    }
    qwc = (unsigned int)(macroblocks * kMacroblockQwc);
    address = (unsigned int)pTable->mBuffer & kPhysicalAddressMask;
    g_nToIpuNextAddress = address;
    g_nToIpuRemainingQwc = qwc;
    if (qwc > kToIpuChunkQwc) {
        handlerId = AddDmacHandler(kDmaChannelToIpu, sceMpegToIpuHandler, 0);
        *MpegRegister(kDmacStatusAddress) = kDmacStatusToIpu;
        EnableDmac(kDmaChannelToIpu);
        address = g_nToIpuNextAddress;
        *MpegRegister(kIpuToMadrAddress) = address;
        *MpegRegister(kIpuToQwcAddress) = kToIpuChunkQwc;
        *MpegRegister(kIpuToChcrAddress) = kDmaChcrStartFromMemory;
        g_nToIpuNextAddress = (address + kToIpuChunkBytes) & kPhysicalAddressMask;
        g_nToIpuRemainingQwc -= kToIpuChunkQwc;
        if (macroblocks < kColourConvertLimit) {
            sceMpegConvertColours((unsigned int)work->mPictureAddress, macroblocks);
        } else {
            sceMpegConvertColoursChunked((unsigned int)work->mPictureAddress, macroblocks);
        }
        DisableDmac(kDmaChannelToIpu);
        RemoveDmacHandler(kDmaChannelToIpu, handlerId);
    } else {
        *MpegRegister(kIpuToMadrAddress) = address;
        *MpegRegister(kIpuToQwcAddress) = qwc;
        *MpegRegister(kIpuToChcrAddress) = kDmaChcrStartFromMemory;
        g_nToIpuRemainingQwc = 0;
        if (macroblocks < kColourConvertLimit) {
            sceMpegConvertColours((unsigned int)work->mPictureAddress, macroblocks);
        } else {
            sceMpegConvertColoursChunked((unsigned int)work->mPictureAddress, macroblocks);
        }
    }
    MpegInvokeTypedCallback(g_decoderInstance, kMpegCbRestartDma);
}

// 0x0060c900
static int sceMpegCheckPictureBufferSize(MpegSeqTable *pTable) {
    MpegWork *work;
    char message[kBufferMessageSize];
    int fits;

    work = MpegInstanceWork();
    if (work->mOutputHeight == 0) {
        fits = !(work->mOutputMacroblocks < pTable->mMbWidth * pTable->mMbHeight);
    } else if (work->mOutputWidth < pTable->mWidth) {
        fits = 0;
    } else {
        fits = !(work->mOutputHeight < pTable->mHeight);
    }
    if (fits == 0) {
        sprintf(message,
                "Too small buffer size for %dx%d picture\n",
                pTable->mWidth,
                pTable->mHeight);
        sceMpegRaiseError(message);
    }
    return fits;
}

// 0x0060c9a0
static void sceMpegCopyRawPicture(MpegSeqTable *pTable) {
    MpegWork *work;
    unsigned int source;
    unsigned int destinationBase;
    unsigned int destination;
    unsigned int nextSource;
    unsigned int nextDestination;
    int sourceStride;
    int destinationStride;
    int rowQwc;
    int passes;
    int pass;
    int row;

    work = MpegInstanceWork();
    source = (unsigned int)pTable->mBuffer & kPhysicalAddressMask;
    destinationBase = (unsigned int)work->mPictureAddress & kPhysicalAddressMask;
    if (g_nMpegPictureStructure == kPictureStructureFrame || work->mOutputHeight == 0) {
        sourceStride = pTable->mMbHeight * kMacroblockBytes;
        if (work->mOutputHeight != 0) {
            destinationStride = (work->mOutputHeight >> kQwcShift) * kMacroblockBytes;
        } else {
            destinationStride = sourceStride;
        }
        passes = 1;
    } else {
        destinationStride = (work->mOutputHeight >> kQwcShift) * kHalfMacroblockBytes;
        sourceStride = (pTable->mMbHeight >> 1) * kMacroblockBytes;
        passes = 2;
    }
    rowQwc = sourceStride >> kQwcShift;
    for (pass = 0; pass < passes; ++pass) {
        destination = destinationBase;
        for (row = 0; row < pTable->mMbWidth; ++row) {
            *MpegRegister(kToSprSadrAddress) = 0;
            *MpegRegister(kToSprMadrAddress) = source;
            *MpegRegister(kToSprQwcAddress) = (unsigned int)rowQwc;
            *MpegRegister(kToSprChcrAddress) = kDmaChcrStartFromMemory;
            nextSource = source + (unsigned int)sourceStride;
            nextDestination = destination + (unsigned int)destinationStride;
            while ((*MpegRegister(kToSprChcrAddress) & kDmaChcrStart) != 0) {
            }
            *MpegRegister(kFromSprSadrAddress) = 0;
            *MpegRegister(kFromSprMadrAddress) = destination;
            *MpegRegister(kFromSprQwcAddress) = (unsigned int)rowQwc;
            *MpegRegister(kFromSprChcrAddress) = kDmaChcrStart;
            while ((*MpegRegister(kFromSprChcrAddress) & kDmaChcrStart) != 0) {
            }
            while (*MpegRegister(kFromSprQwcAddress) != 0) {
            }
            destination = nextDestination;
            source = nextSource;
        }
        destinationBase += (unsigned int)(work->mOutputMacroblocks * kHalfMacroblockBytes);
    }
}

// 0x0060cb90
static void sceMpegFinishPictureOutput(void) {
    MpegWork *work;

    work = MpegInstanceWork();
    if (work->mOutputState != kOutputShown) {
        work->mOutputState = kOutputShown;
        work->mFrameCountBase = g_mpegPictureCounter;
    }
    g_pictureWaitFlag = 1;
}

// 0x0060cbc8
static void sceMpegGetPictureStamps(MpegSeqTable *pTable,
                                    long long *pPts,
                                    long long *pDts,
                                    unsigned long long *pFlags) {
    MpegWork *work;
    long long pts;
    long long oddFields;
    long long weight;
    int base;
    int fields;
    int counter;
    int rounding;
    int half;

    work = MpegInstanceWork();
    if (work->mUseDefaultPtsGap == 0) {
        *pPts = pTable->mPts;
    } else {
        pts = pTable->mPts;
        base = work->mLastPts;
        if (pts >= 0 || base < 0) {
            *pPts = pts;
        } else {
            // Interpolate from the last stamp by half the frame period for each field shown.
            fields = (int)work->mDisplayFieldCount;
            oddFields = fields & 1;
            counter = work->mOddGapPictures;
            weight = oddFields * (long long)(work->mDefaultPtsGap & 1);
            rounding = (int)(weight * (counter & 1));
            half = (int)(((long long)work->mDefaultPtsGap * fields) >> 1);
            *pPts = (int)((unsigned int)base + (unsigned int)half + (unsigned int)rounding);
            if (oddFields * (long long)(work->mDefaultPtsGap & 1) != 0) {
                work->mOddGapPictures = counter + 1;
            }
        }
    }
    if (work->mPendingPtsState == kPendingStampReady && work->mPendingPts >= 0) {
        *pPts = work->mPendingPts;
        work->mPendingPtsState = 0;
        work->mPendingPts = -1;
    }
    *pDts = pTable->mDts;
    *pFlags = ((unsigned long long)(long long)pTable->mProgressiveSequence
               << kPictureFlagProgressiveSequenceShift) |
              (unsigned long long)(long long)pTable->mPictureCodingType |
              ((unsigned long long)(long long)pTable->mRepeatFirstField
               << kPictureFlagRepeatFirstFieldShift) |
              ((unsigned long long)(long long)pTable->mTopFieldFirst
               << kPictureFlagTopFieldFirstShift) |
              ((unsigned long long)(long long)pTable->mProgressiveFrame
               << kPictureFlagProgressiveFrameShift) |
              ((unsigned long long)(long long)pTable->mPictureStructure
               << kPictureFlagStructureShift);
}

// 0x0060cd60
static void sceMpegOutputFramePicture(MpegSeqTable *pTable, int nIndex) {
    sceMpeg *decoder;
    MpegWork *work;
    int i;

    (void)nIndex; // The callers pass the counter less one, and the image never reads it.
    decoder = (sceMpeg *)g_decoderInstance;
    work = (MpegWork *)decoder->pContext;
    sceMpegGetPictureStamps(pTable, &decoder->pts, &decoder->dts, &decoder->flags);
    work->mLastPts = (int)decoder->pts;
    work->mDisplayFieldCount = (unsigned int)
        g_mpegFieldCountTable[(decoder->flags >> kPictureFlagFieldCountShift) &
                              kPictureFlagFieldCountMask];
    work->mDisplayHorizontalSize = pTable->mDisplayHorizontalSize;
    work->mDisplayVerticalSize = pTable->mDisplayVerticalSize;
    for (i = 0; i < kFrameCentreOffsetCount; ++i) {
        work->mFrameCentreHorizontalOffset[i] = pTable->mFrameCentreHorizontalOffset[i];
        work->mFrameCentreVerticalOffset[i] = pTable->mFrameCentreVerticalOffset[i];
    }
    if (sceMpegCheckPictureBufferSize(pTable) == 0 || pTable->mDecoded != 1) {
        return;
    }
    if (work->mConvertColours != 0) {
        sceMpegConvertPicture(pTable);
    } else {
        sceMpegCopyRawPicture(pTable);
    }
    sceMpegFinishPictureOutput();
}

// 0x0060ce78
static void sceMpegOutputFieldPicture(MpegSeqTable *pFirst, MpegSeqTable *pSecond, int nIndex) {
    sceMpeg *decoder;
    MpegWork *work;
    MpegSeqTable *earlier;
    MpegSeqTable *later;
    unsigned long long topFieldFirst;

    (void)nIndex; // The callers pass the counter less one, and the image never reads it.
    decoder = (sceMpeg *)g_decoderInstance;
    work = (MpegWork *)decoder->pContext;
    if (g_nMpegPictureStructure == kPictureStructureBottomField) {
        earlier = pFirst;
        later = pSecond;
        topFieldFirst = kPictureFlagTopFieldFirst;
    } else {
        earlier = pSecond;
        later = pFirst;
        topFieldFirst = 0;
    }
    sceMpegGetPictureStamps(earlier, &decoder->pts, &decoder->dts, &decoder->flags);
    work->mDisplayFieldCount = 1;
    work->mLastPts = (int)decoder->pts;
    sceMpegGetPictureStamps(later, &decoder->pts2nd, &decoder->dts2nd, &decoder->flags2nd);
    work->mDisplayFieldCount = 1;
    work->mLastPts = (int)decoder->pts2nd;
    decoder->flags |= topFieldFirst;
    decoder->flags2nd |= topFieldFirst;
    // The image copies only the first two offsets, alternating between the two fields.
    work->mDisplayHorizontalSize = earlier->mDisplayHorizontalSize;
    work->mDisplayVerticalSize = earlier->mDisplayVerticalSize;
    work->mFrameCentreHorizontalOffset[0] = earlier->mFrameCentreHorizontalOffset[0];
    work->mFrameCentreHorizontalOffset[1] = later->mFrameCentreHorizontalOffset[1];
    work->mFrameCentreVerticalOffset[0] = earlier->mFrameCentreVerticalOffset[0];
    work->mFrameCentreVerticalOffset[1] = later->mFrameCentreVerticalOffset[1];
    if (sceMpegCheckPictureBufferSize(pFirst) == 0 || pFirst->mDecoded != 1 ||
        pSecond->mDecoded != 1) {
        return;
    }
    // The two fields interleave in the first table's buffer. The table's row count therefore
    // doubles for output.
    pFirst->mMbHeight *= 2;
    if (work->mConvertColours != 0) {
        sceMpegConvertPicture(pFirst);
    } else {
        sceMpegCopyRawPicture(pFirst);
    }
    pFirst->mMbHeight >>= 1;
    sceMpegFinishPictureOutput();
}

// 0x0060c468
static void sceMpegOutputPicture(int nCounter, int bOutput) {
    MpegWork *work;

    work = MpegInstanceWork();
    if (bOutput != 0) {
        if (g_nMpegPictureStructure == kPictureStructureFrame) {
            if (g_nMpegPictureCodingType == kPictureCodingBidirectional) {
                sceMpegOutputFramePicture(g_mpegTables[kTableFrameBidirectional], nCounter - 1);
            } else {
                sceMpegOutputFramePicture(g_mpegTables[kTableFrameForward], nCounter - 1);
            }
        } else if (g_nMpegPictureCodingType == kPictureCodingBidirectional) {
            sceMpegOutputFieldPicture(g_mpegTables[kTableTopBidirectional],
                                      g_mpegTables[kTableBottomBidirectional],
                                      nCounter - 1);
        } else {
            sceMpegOutputFieldPicture(
                g_mpegTables[kTableTopForward], g_mpegTables[kTableBottomForward], nCounter - 1);
        }
    }
    if (work->mPendingPtsState == kPendingStampArmed) {
        work->mPendingPtsState = kPendingStampReady;
    }
}

// 0x0060dcf0
static void sceMpegCheckSecondField(int nCounter) {
    if (g_mpegSecondFieldPending != 0) {
        sceMpegRaiseError("the second field is missing");
        g_mpegSecondFieldPending = 0;
        return;
    }
    if (g_nMpegPictureStructure == kPictureStructureFrame) {
        sceMpegOutputFramePicture(g_mpegTables[kTableFrameBackward], nCounter - 1);
    } else {
        sceMpegOutputFieldPicture(
            g_mpegTables[kTableTopBackward], g_mpegTables[kTableBottomBackward], nCounter - 1);
    }
    g_mpegSecondFieldPending = 0;
}

static inline void MpegSwapTables(int nFirst, int nSecond) {
    MpegSeqTable *table;

    table = g_mpegTables[nFirst];
    g_mpegTables[nFirst] = g_mpegTables[nSecond];
    g_mpegTables[nSecond] = table;
}

static inline int MpegTableComplete(int nIndex) {
    return g_mpegTables[nIndex]->mDecoded == 1;
}

// 0x0060c520
static int sceMpegSelectPictureTables(int bKeepReferences) {
    MpegWork *work;
    MpegSeqTable *target;
    MpegSeqTable *other;
    int structure;
    int codingType;
    int threshold;
    int ready;
    int i;

    work = MpegInstanceWork();
    structure = g_nMpegPictureStructure;
    codingType = g_nMpegPictureCodingType;
    threshold = (structure == kPictureStructureFrame) ? 2 : 4;
    ready = 0;
    target = NULL;
    if (codingType == kPictureCodingBidirectional) {
        g_mpegCurrentTables[kCurrentFrame] = g_mpegTables[kTableFrameBidirectional];
        g_mpegCurrentTables[kCurrentTopField] = g_mpegTables[kTableTopBidirectional];
        g_mpegCurrentTables[kCurrentBottomField] = g_mpegTables[kTableBottomBidirectional];
        if (work->mDecodeCounts[kPictureCountIntra] +
                work->mDecodeCounts[kPictureCountPredicted] >=
            threshold) {
            work->mForceBrokenLink = 0;
            g_nMpegBrokenLink = 0;
            g_nMpegClosedGop = 0;
        }
        if ((work->mForceBrokenLink != 0 || g_nMpegBrokenLink != 0) && g_nMpegClosedGop == 0) {
            g_mpegTables[kTableFrameForward]->mDecoded = 0;
            g_mpegTables[kTableTopForward]->mDecoded = 0;
            g_mpegTables[kTableBottomForward]->mDecoded = 0;
        }
        work->mForceBrokenLink = 0;
        g_nMpegBrokenLink = 0;
        if (g_nMpegPictureStructure == kPictureStructureFrame) {
            if (MpegTableComplete(kTableFrameForward) || g_nMpegClosedGop != 0) {
                ready = MpegTableComplete(kTableFrameBackward);
            }
        } else if ((MpegTableComplete(kTableTopForward) &&
                    MpegTableComplete(kTableBottomForward)) ||
                   g_nMpegClosedGop != 0) {
            if (MpegTableComplete(kTableTopBackward)) {
                ready = MpegTableComplete(kTableBottomBackward);
            }
        }
    } else {
        if (bKeepReferences == 0) {
            MpegSwapTables(kTableFrameForward, kTableFrameBackward);
            MpegSwapTables(kTableTopForward, kTableTopBackward);
            MpegSwapTables(kTableBottomForward, kTableBottomBackward);
        }
        g_mpegCurrentTables[kCurrentFrame] = g_mpegTables[kTableFrameBackward];
        g_mpegCurrentTables[kCurrentTopField] = g_mpegTables[kTableTopBackward];
        g_mpegCurrentTables[kCurrentBottomField] = g_mpegTables[kTableBottomBackward];
        if (structure == kPictureStructureFrame) {
            if (codingType != kPictureCodingPredicted || MpegTableComplete(kTableFrameForward)) {
                ready = 1;
            }
        } else {
            other = (structure != kPictureStructureTopField) ? g_mpegTables[kTableTopBackward]
                                                             : g_mpegTables[kTableBottomBackward];
            if (g_nMpegPictureCodingType != kPictureCodingPredicted) {
                ready = 1;
            } else if (bKeepReferences != 0 && other->mDecoded == 1) {
                ready = 1;
            } else if (MpegTableComplete(kTableTopForward) &&
                       MpegTableComplete(kTableBottomForward)) {
                ready = 1;
            }
        }
    }
    switch (g_nMpegPictureStructure) {
    case kPictureStructureBottomField:
        target = g_mpegCurrentTables[kCurrentBottomField];
        break;
    case kPictureStructureTopField:
        target = g_mpegCurrentTables[kCurrentTopField];
        break;
    case kPictureStructureFrame:
        target = g_mpegCurrentTables[kCurrentFrame];
        break;
    default:
        break; // The image stores through a null table here.
    }
    target->mPictureCodingType = g_nMpegPictureCodingType;
    target->mPictureStructure = g_nMpegPictureStructure;
    target->mProgressiveSequence = g_nMpegProgressiveSequence;
    target->mProgressiveFrame = g_nMpegProgressiveFrame;
    target->mTopFieldFirst = g_nMpegTopFieldFirst;
    target->mRepeatFirstField = g_nMpegRepeatFirstField;
    target->mDecoded = 0;
    for (i = 0; i < kFrameCentreOffsetCount; ++i) {
        target->mFrameCentreHorizontalOffset[i] = g_anMpegFrameCentreHorizontalOffsets[i];
        target->mFrameCentreVerticalOffset[i] = g_anMpegFrameCentreVerticalOffsets[i];
    }
    target->mPts = (long long)g_llMpegNextPts;
    target->mDts = (long long)g_llMpegNextDts;
    target->mDisplayHorizontalSize = g_nMpegDisplayHorizontalSize;
    target->mDisplayVerticalSize = g_nMpegDisplayVerticalSize;
    return ready;
}

// 0x0060c388
static int sceMpegDecodePictureBody(int nCounter, int nIndex) {
    MpegSeqTable *target;
    int decoded;

    (void)nIndex; // The callers pass the picture index, and the image never reads it.
    if (g_nMpegPictureStructure == kPictureStructureFrame && g_mpegSecondFieldPending != 0) {
        sceMpegRaiseError("odd number of field pictures");
        g_mpegSecondFieldPending = 0;
    }
    switch (g_nMpegPictureStructure) {
    case kPictureStructureBottomField:
        target = g_mpegCurrentTables[kCurrentBottomField];
        break;
    case kPictureStructureTopField:
        target = g_mpegCurrentTables[kCurrentTopField];
        break;
    case kPictureStructureFrame:
        target = g_mpegCurrentTables[kCurrentFrame];
        break;
    default:
        sceMpegRaiseError("unknown picture sutructure");
        target = g_mpegCurrentTables[kCurrentFrame];
        break;
    }
    decoded = sceMpegDecodeSlices(nCounter);
    if (decoded != 0) {
        target->mDecoded = 1;
    }
    return decoded;
}

// 0x005e0ff0
static int sceMpegFlushLastPicture(void *pDecoder) {
    sceMpeg *decoder;
    MpegWork *work;

    decoder = (sceMpeg *)pDecoder;
    work = (MpegWork *)decoder->pContext;
    if (work->mPictureIndex == 0 || work->mOutputState == kOutputIdle) {
        return 0;
    }
    sceMpegCheckSecondField(g_mpegPictureCounter);
    decoder->frameCount = g_mpegPictureCounter - work->mFrameCountBase;
    work->mPictureIndex = 0;
    return 1;
}

static inline void MpegStartOutput(sceMpeg *pDecoder, MpegWork *pWork) {
    if (pWork->mOutputState == kOutputIdle) {
        pDecoder->frameCount = 0;
        pWork->mOutputState = kOutputDecoding;
    }
}

// 0x005e0d18
static int sceMpegDecodeFramePicture(void *pDecoder, int nCount, int nLimit) {
    sceMpeg *decoder;
    MpegWork *work;
    int skipped;
    int decoded;

    decoder = (sceMpeg *)pDecoder;
    work = (MpegWork *)decoder->pContext;
    skipped = 0;
    if (nLimit == -1 || nCount < nLimit) {
        MpegStartOutput(decoder, work);
        decoded = 0;
        if (sceMpegSelectPictureTables(0) != 0) {
            decoded = sceMpegDecodePictureBody(g_mpegPictureCounter, work->mPictureIndex) != 0;
        }
    } else {
        skipped = 1;
        decoded = sceMpegSelectPictureTables(1);
        sceMpegReportNoData(pDecoder);
    }
    sceMpegOutputPicture(g_mpegPictureCounter, work->mPictureIndex);
    if (g_nMpegPictureStructure != kPictureStructureFrame && skipped == 0) {
        g_mpegSecondFieldPending = !g_mpegSecondFieldPending;
    }
    decoder->frameCount = g_mpegPictureCounter - work->mFrameCountBase;
    if (g_mpegSecondFieldPending == 0) {
        ++g_mpegPictureCounter;
        ++work->mPictureIndex;
    }
    return decoded;
}

// 0x005e0e80
static int sceMpegDecodeFieldPicture(void *pDecoder, int nCount, int nLimit) {
    sceMpeg *decoder;
    MpegWork *work;
    int wanted;
    int decoded;
    int expected;

    decoder = (sceMpeg *)pDecoder;
    work = (MpegWork *)decoder->pContext;
    g_mpegSecondFieldPending = 0;
    wanted = nLimit == -1 || nCount < nLimit;
    MpegStartOutput(decoder, work);
    if (sceMpegSelectPictureTables(0) != 0 && wanted != 0) {
        (void)sceMpegDecodePictureBody(g_mpegPictureCounter, work->mPictureIndex); // Discarded.
    }
    g_mpegSecondFieldPending = 1;
    if (sceMpegNextPictureHeader() == 0) {
        sceMpegFlushLastPicture(pDecoder);
        work->mCompleted = 1;
        return 0;
    }
    expected = (work->mFirstFieldStructure != kPictureStructureTopField) ? kPictureStructureTopField
                                                                : kPictureStructureBottomField;
    if (g_nMpegPictureStructure != expected) {
        return -1;
    }
    decoded = 0;
    if (sceMpegSelectPictureTables(1) != 0 && wanted != 0) {
        decoded = sceMpegDecodePictureBody(g_mpegPictureCounter, work->mPictureIndex) != 0;
    }
    sceMpegOutputPicture(g_mpegPictureCounter, work->mPictureIndex);
    g_mpegSecondFieldPending = 0;
    decoder->frameCount = g_mpegPictureCounter - work->mFrameCountBase;
    ++g_mpegPictureCounter;
    ++work->mPictureIndex;
    if (wanted == 0) {
        sceMpegReportNoData(pDecoder);
    }
    return decoded;
}

// 0x005e0e40
static int sceMpegDecodeCodedPicture(void *pDecoder, int nCount, int nLimit) {
    if (g_nMpegPictureStructure == kPictureStructureFrame) {
        return sceMpegDecodeFramePicture(pDecoder, nCount, nLimit);
    }
    return sceMpegDecodeFieldPicture(pDecoder, nCount, nLimit);
}

// 0x005e0b90
static int decodePictureInner(void *pDecoder) {
    MpegWork *work;
    int state;
    int result;

    work = (MpegWork *)((sceMpeg *)pDecoder)->pContext;
    work->mCompleted = 0;
    if (((unsigned int)work->mPictureAddress & kPictureAlignMask) != 0) {
        sceMpegReportErrorFormatted("image buffer needs to be aligned to 64byte boundary(0x%08x)",
                                    work->mPictureAddress);
        return -1;
    }
    g_pictureWaitFlag = 0;
    state = kPictureCodingIntra;
    result = 0;
    do {
        // A skipped field pair reruns the last picture type without reading a new header.
        if (result != -1) {
            do {
                state = sceMpegNextPictureHeader();
            } while (state != kPictureSequenceEnd && g_nMpegPictureStructure != work->mFirstFieldStructure &&
                     g_nMpegIsMpeg2 != 0);
        }
        switch (state) {
        case kPictureSequenceEnd:
            sceMpegFlushLastPicture(pDecoder);
            work->mCompleted = 1;
            break;
        case kPictureCodingIntra:
            work->mDecodeCounts[kPictureCountBidirectional] = 0;
            work->mDecodeCounts[kPictureCountPredicted] = 0;
            work->mDecodeCounts[kPictureCountIntra] = 0;
            result =
                sceMpegDecodeCodedPicture(pDecoder, 0, work->mDecodeLimits[kPictureCountIntra]);
            ++work->mDecodeCounts[kPictureCountIntra];
            break;
        case kPictureCodingPredicted:
            result = sceMpegDecodeCodedPicture(pDecoder,
                                               work->mDecodeCounts[kPictureCountPredicted],
                                               work->mDecodeLimits[kPictureCountPredicted]);
            ++work->mDecodeCounts[kPictureCountPredicted];
            break;
        case kPictureCodingBidirectional:
        case kPictureCodingDc:
            result = sceMpegDecodeCodedPicture(pDecoder,
                                               work->mDecodeCounts[kPictureCountBidirectional],
                                               work->mDecodeLimits[kPictureCountBidirectional]);
            ++work->mDecodeCounts[kPictureCountBidirectional];
            break;
        default:
            break;
        }
    } while (g_pictureWaitFlag == 0);
    return 1;
}

// 0x005e07b0
int sceMpegGetPicture(void *pDecoder, void *pPicture, int nMacroblocks) {
    MpegWork *work;
    uintptr_t picture;

    picture = (uintptr_t)pPicture;
    picture = picture & (uintptr_t)kPhysicalAddressMask;
    picture = picture | (uintptr_t)kUncachedSegment;
    work = (MpegWork *)((sceMpeg *)pDecoder)->pContext;
    work->mConvertColours = 1;
    work->mPictureAddress = (int)picture;
    work->mOutputMacroblocks = nMacroblocks;
    work->mOutputWidth = 0;
    // The second clear falls in the call delay slot and therefore lands before the decode body.
    work->mOutputHeight = 0;
    return decodePictureInner(pDecoder);
}

// 0x005e07f8
int sceMpegGetPictureRAW8(void *pDecoder, void *pPicture, int nMacroblocks) {
    MpegWork *work;
    uintptr_t picture;

    picture = (uintptr_t)pPicture;
    picture = picture & (uintptr_t)kPhysicalAddressMask;
    picture = picture | (uintptr_t)kUncachedSegment;
    work = (MpegWork *)((sceMpeg *)pDecoder)->pContext;
    work->mOutputMacroblocks = nMacroblocks;
    work->mPictureAddress = (int)picture;
    work->mOutputWidth = 0;
    work->mConvertColours = 0;
    // The second clear falls in the call delay slot and therefore lands before the decode body.
    work->mOutputHeight = 0;
    return decodePictureInner(pDecoder);
}

// 0x005e0840
int sceMpegGetPictureRAW8xy(void *pDecoder, void *pPicture, int nMbWidth, int nMbHeight) {
    MpegWork *work;
    uintptr_t picture;

    picture = (uintptr_t)pPicture;
    picture = picture & (uintptr_t)kPhysicalAddressMask;
    picture = picture | (uintptr_t)kUncachedSegment;
    work = (MpegWork *)((sceMpeg *)pDecoder)->pContext;
    work->mOutputHeight = nMbHeight << 4;
    work->mPictureAddress = (int)picture;
    work->mOutputMacroblocks = nMbWidth * nMbHeight;
    work->mOutputWidth = nMbWidth << 4;
    // The busy clear falls in the call delay slot and therefore lands before the decode body.
    work->mConvertColours = 0;
    return decodePictureInner(pDecoder);
}

// 0x005e0530
void *sceMpegCreateDecoderContext(void *pDecoder, void *pWork, int nWorkSize) {
    sceMpeg *decoder;
    uintptr_t work;
    uintptr_t aligned;
    int rest;
    MpegWork *context;
    MpegRing *ring;
    int i;

    decoder = (sceMpeg *)pDecoder;
    work = (uintptr_t)pWork;
    aligned = (work + (uintptr_t)kAlignMask) & ~(uintptr_t)kAlignMask;
    rest = nWorkSize - (int)(aligned - work);
    if ((unsigned int)rest < kMinWorkSize) {
        sceMpegRaiseError("The size of work area is too small");
        return NULL;
    }
    context = (MpegWork *)aligned;
    ring = &context->mRing;
    sceMpegResetRingPointers(ring, (void *)(aligned + (uintptr_t)kMinWorkSize), rest - kMinWorkSize);
    decoder->width = 0;
    decoder->height = 0;
    decoder->frameCount = 0;
    decoder->pts = -1;
    decoder->dts = -1;
    decoder->flags = 0;
    decoder->pts2nd = -1;
    decoder->dts2nd = -1;
    decoder->flags2nd = 0;
    decoder->pContext = context;
    for (i = 0; i < kFrameCentreOffsetCount; ++i) {
        context->mFrameCentreHorizontalOffset[i] = 0;
        context->mFrameCentreVerticalOffset[i] = 0;
    }
    context->mDisplayHorizontalSize = 0;
    context->mDisplayVerticalSize = 0;
    context->mFirstFieldStructure = 0;
    context->mPictureAddress = 0;
    context->mOutputWidth = 0;
    context->mOutputHeight = 0;
    context->mOutputMacroblocks = 0;
    context->mForceBrokenLink = 0;
    context->mPendingPtsState = 0;
    context->mSlots[0].callback = NULL;
    context->mSlots[1].callback = NULL;
    context->mSlots[4].callback = NULL;
    context->mSlots[5].callback = NULL;
    context->mSlots[6].callback = NULL;
    context->mPendingPts = -1;
    context->mSlots[2].callback = g_defaultSlotTwo;
    context->mSlots[3].callback = g_defaultSlotThree;
    // The second default falls in the allocator call delay slot and still lands here.
    context->mStreamTable = (StreamEntry *)sceMpegCheckWorkAreaSize(ring, kStreamAllocSize, kStreamAllocAlign);
    context->mStreamCount = 0;
    context->mFirstFrameBuffer = 0;
    context->mSecondFrameBuffer = 0;
    context->mThirdFrameBuffer = 0;
    context->mUseDefaultPtsGap = 0;
    context->mDefaultPtsGap = 0;
    context->mDisplayFieldCount = 0;
    context->mOddGapPictures = 0;
    context->mFrameCountBase = 0;
    context->mDecodeLimits[kPictureCountBidirectional] = -1;
    context->mConvertColours = 1;
    g_decoderInstance = decoder;
    context->mLastPts = -1;
    context->mDecodeLimits[kPictureCountIntra] = -1;
    context->mDecodeLimits[kPictureCountPredicted] = -1;
    // The last busy clear falls in the disable call delay slot and still lands here.
    sceMpegResetMcBuffers();
    sceMpegReset(decoder);
    sceMpegClearRefBuff(decoder);
    for (i = 0; i < kTableCount; ++i) {
        g_mpegTables[i] = &g_mpegSeqAreas[i];
    }
    return (void *)(uintptr_t)sceMpegCommitWritePointer(ring);
}

// 0x005e0990
void *sceMpegSetCallbackSlot(void *pDecoder, int nSlot, void *pfnCallback, void *pData) {
    MpegWork *work;
    void *old;

    work = (MpegWork *)((sceMpeg *)pDecoder)->pContext;
    work->mSlots[nSlot].data = pData;
    old = work->mSlots[nSlot].callback;
    work->mSlots[nSlot].callback = pfnCallback;
    return old;
}

// 0x005e09b8
int sceMpegInvokeCallbackSlot(void *pDecoder, void *pEntry) {
    MpegWork *work;
    StreamEntry *entry;
    unsigned int key;
    MpegSlot *slot;
    int (*callback)(void *pDecoder, void *pEntry, void *pData);

    if (pDecoder == NULL) {
        return 0;
    }
    work = (MpegWork *)((sceMpeg *)pDecoder)->pContext;
    if (work == NULL) {
        return 0;
    }
    entry = (StreamEntry *)pEntry;
    key = (unsigned int)entry->key;
    // The image scales the key with no bounds check, so the slot index is equally unchecked.
    slot = &work->mSlots[key];
    if (slot->callback == NULL) {
        return 0;
    }
    callback = (int (*)(void *, void *, void *))slot->callback;
    return callback(pDecoder, pEntry, slot->data);
}

// 0x005e0770
int sceMpegDelete(void *pDecoder) {
    (void)pDecoder;
    return 1;
}

// 0x005e0890
void sceMpegSetDecodeMode(void *pDecoder, int nIntra, int nPredicted, int nBidirectional) {
    MpegWork *work;

    work = (MpegWork *)((sceMpeg *)pDecoder)->pContext;
    work->mDecodeLimits[kPictureCountBidirectional] = nBidirectional;
    work->mDecodeLimits[kPictureCountIntra] = nIntra;
    // The second value falls in the return delay slot and still lands here.
    work->mDecodeLimits[kPictureCountPredicted] = nPredicted;
}

// 0x005e08d8
int sceMpegIsRefBuffEmpty(void *pDecoder) {
    MpegWork *work;
    unsigned int value;

    work = (MpegWork *)((sceMpeg *)pDecoder)->pContext;
    value = (unsigned int)work->mPictureIndex;
    return value < 1u;
}

// 0x005caa78
sceMpegCallback sceMpegAddStrCallback(
    void *pDecoder, int nType, int nChannel, sceMpegCallback pfnCallback, void *pData) {
    MpegWork *work;
    StreamEntry *table;
    int count;
    unsigned long long key;
    unsigned long long templateBits;
    StreamEntry *entry;
    int index;
    sceMpegCallback found;

    work = (MpegWork *)((sceMpeg *)pDecoder)->pContext;
    key = buildStreamKey(nType, nChannel);
    table = work->mStreamTable;
    count = work->mStreamCount;
    index = 0;
    found = NULL;
    if (count > 0) {
        for (index = 0; index < count; ++index) {
            if (table[index].key == key) {
                found = table[index].callback;
                break;
            }
        }
    }
    if (index >= kMaxStreamCallbacks) {
        return found;
    }
    // Yes, the image indexes the template table with no bounds check on the stream type.
    templateBits = g_streamTemplates[nType][1];
    entry = &table[index];
    entry->key = key;
    entry->templateBits = templateBits;
    entry->callback = pfnCallback;
    entry->data = pData;
    work->mStreamCount = count + 1;
    return found;
}
