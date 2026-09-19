#pragma once

#include <libsdr.h>
#include <vector>

#include "os/hxstr.h"

/**
 * Voice and sound-bank driver that Ps2HardSynth is a thin class over.
 *
 * Titled after `midi_main.cpp`, the string at `0x0081cc98` that the module bills its heap releases
 * to. The module spans `0x00461a88` through `0x00465200`, the zone allocator ending just below it,
 * and has the attested behaviour
 * of the sound subsystem: the SPU2 voices, the sound banks, and the script-facing command
 * dispatcher. Neither Synth nor Ps2HardSynth has any of it.
 *
 * There is no voice table. Every routine here that reports or configures a voice queries the SPU2
 * itself through libsdr's remote-call trampoline, `sceSdRemote()`, with the `rSd*` function code
 * and the `SD_*` entry encoding inline at the call site. The image has no `sceSd*` wrapper
 * function, so the SDK release of this era supplied the whole API as macros over that one entry
 * point. Correspondence is exact at both levels: the function codes are `rSdInit`, `rSdSetParam`,
 * `rSdGetParam`, `rSdGetSwitch`, `rSdSetAddr`, `rSdGetAddr`, `rSdSetCoreAttr`, `rSdGetCoreAttr` and
 * `rSdSetEffectAttr`, and the entries carry libsd's `0x80` core-level flag on exactly the
 * core-level registers.
 *
 * The two bank transfers are classes rather than routines. CallbackXferBdToIop streams a BD bank in
 * 0x2000-byte chunks and CallbackXferHdToIop moves an HD bank in one go, and both derive from
 * AsyncCallback. The globals below are what the two share with the rest of the module.
 */

/**
 * Run one script-facing synth command.
 *
 * Three command numbers are recognised. Command 0 does nothing. Command 1 reports the voices
 * through DumpSynthVoices(). Command 2 submits driver selector 0xd0 with no command block. Anything
 * else reports `Unrecognized synth cmd %d`, and the command number is retained in its second
 * argument register from entry so that the report can print it.
 *
 * The script layer exposes this as `synth_cmd`, which is the one attested title in the module.
 *
 * @param nCommand The command to run.
 * @ghidraAddress 0x00464ad0
 */
void SynthCommand(int nCommand);

/**
 * Submit one command to the sound driver.
 *
 * Every routine in the module funnels through this, twenty call sites in all, each passing a
 * selector word and either a command block or nothing. Observed selectors are 0xd0 from
 * SynthCommand() and from the tail of DumpSynthVoices(), 0x1070 and 0x1050 from a bank transfer,
 * 0x10e0 from ConfigureSpu2Effects(), and 0x8130 from `0x004642c8`.
 *
 * Two bits of the selector steer the send. Bit 0x1000 ships a whole 0x80-byte SoundDriverCommand
 * from the caller's block; without it the block pointer travels as the single word of a 0x10-byte
 * request, which is what makes a null block valid. Bit 0x8000 skips the busy flag and sends a
 * different mode. On entry the routine spins until the previous request has been collected.
 *
 * The title comes from what the twenty call sites have in common rather than from any one of them.
 *
 * @param nSelector The command selector.
 * @param pCommand The command block, or null.
 * @return The first word of the reply buffer at `0x008e5b80`. Every call site in the module
 *         discards it.
 * @ghidraAddress 0x005f96c8
 */
int SubmitSoundDriverRequest(int nSelector, void *pCommand);

/**
 * Move a buffer from main memory into the sound driver's memory on the IOP.
 *
 * Fills the descriptor at `0x008e5be8` with the three arguments and a zero fourth word, then issues
 * the transfer and spins until it reports completion.
 *
 * @param nIopAddress The destination on the IOP.
 * @param pSource The source in main memory.
 * @param nLength The number of bytes to move.
 * @return Zero once the transfer has completed, or -1 when it could not be started.
 * @ghidraAddress 0x005f97d0
 */
int XferToIop(int nIopAddress, const void *pSource, int nLength);

/** Bytes of payload a command block has above its five-word header. */
constexpr int kSoundDriverCommandPayloadSize = 0x6c;

/**
 * Command block the sound driver reads a bank transfer out of.
 *
 * Two blocks of this shape exist, one per selector, and the shape is recovered from the two
 * together rather than from either alone. The chunk block writes mStagingAddress and clears
 * mBankAddress; the bank block writes mBankAddress and never touches mStagingAddress. All three of
 * mLength, mDest and mTag sit at the same offset in both, and both have a payload at `+0x14`, which
 * is what identifies the header as shared.
 *
 * The 0x80-byte total is the consumer's figure rather than the writers'. SubmitSoundDriverRequest()
 * sends exactly 0x80 bytes from the caller's block for a selector with bit 0x1000 set, which both
 * bank selectors have. The chunk path clearing 0x6c bytes above the header agrees with it.
 */
struct SoundDriverCommand {
    int mBankAddress;    // +0x00 where the bank lives on the IOP; the chunk path clears it
    int mStagingAddress; // +0x04 buffer the chunk was moved to; the bank path never writes it
    int mLength;         // +0x08
    int mDest;           // +0x0c where the driver writes the data
    int mTag;            // +0x10 copied from g_nSynthXferTag
    char mPayload[kSoundDriverCommandPayloadSize]; // +0x14 the path for a bank, cleared for a chunk
};

/**
 * Block that describes one chunk of a sound bank, submitted under selector 0x1070.
 *
 * @ghidraAddress 0x00894cc0
 */
extern SoundDriverCommand g_chunkCommand;

/** Staging buffers on the IOP that chunk transfers alternate between. */
constexpr int kIopStagingBufferCount = 2;

/**
 * Addresses of the staging buffers on the IOP.
 *
 * The table itself may be longer. Only the first two entries are reachable, because the index below
 * is masked to one bit.
 *
 * @ghidraAddress 0x00894748
 */
extern int g_anIopStagingAddress[kIopStagingBufferCount];

/**
 * Staging buffer the next chunk transfer will use.
 *
 * @ghidraAddress 0x006e9b80
 */
extern int g_nIopStagingIndex;

/**
 * Address on the IOP a sound bank is moved to.
 *
 * CallbackXferHdToIop::Done() moves the whole bank there, and the routine that starts a BD transfer
 * copies it into the first word of the bank-complete block.
 *
 * @ghidraAddress 0x006e9b84
 */
extern int g_nBankIopAddress;

/**
 * One loaded bank's claim on a region of sound memory.
 *
 * The record is 0xc bytes, measured from the stride the two walks advance by and confirmed by the
 * static initialiser at `0x00464170`, which zeroes the vector as three pointers and whose destruct
 * branch frees the same range with that stride.
 *
 * The three field titles come from the arguments LoadSoundBank() passes when it records a slot. The
 * tag is what stops the bank again: releasing a slot submits mTag under selector 0x8130, which is
 * also the word every command block is stamped with at `+0x10`.
 */
struct BankSlot {
    int mTag;        // +0x00
    int mDest;       // +0x04 the region in sound memory this bank claims, which the walks match on
    int mIopAddress; // +0x08 zero for a slot that claims nothing
};

/**
 * Every bank currently claiming sound memory.
 *
 * @ghidraAddress 0x006e9bb8
 */
extern std::vector<BankSlot> g_bankSlots;

/**
 * Record that a bank claims a region of sound memory, releasing whatever claimed it before.
 *
 * A slot whose mDest matches takes the new tag, and its previous tag is submitted under selector
 * 0x8130 first when it claimed anything. A slot already using the new tag under a different
 * destination is released. A destination with no slot is appended.
 *
 * @param nTag The bank's tag.
 * @param nDest The region in sound memory.
 * @param nIopAddress Where the bank lives on the IOP.
 * @ghidraAddress 0x00461a88
 */
void RegisterBankSlot(int nTag, int nDest, int nIopAddress);

/**
 * Release the bank claiming a region of sound memory.
 *
 * The walk stops at the first slot whose mDest matches, whether or not that slot claimed anything.
 *
 * The image has no reference to this routine. LoadSoundBank() reaches the same body inline, so the
 * out-of-line copy exists and nothing calls it.
 *
 * @param nDest The region in sound memory.
 * @ghidraAddress 0x004642c8
 */
void ReleaseBankSlotAt(int nDest);

/**
 * Load a BD and HD bank pair and record the claim.
 *
 * Performs no work when both paths already match what is loaded. The placement argument selects
 * where the pair goes: a negative value leaves both destinations as they are, 0 or 1 selects
 * kBankFixedDestAddress and the first IOP address, 2 selects the first destination buffer, and 3
 * rotates through both destination buffers and through the IOP addresses. Anything above 3 leaves
 * the IOP address alone while still selecting a destination.
 *
 * A placement other than 3 restores the destination the call found, so only the rotating placement
 * leaves the choice behind for the next caller.
 *
 * @param pszBdPath The BD bank.
 * @param pszHdPath The HD bank.
 * @param nTag The tag both banks are recorded and stamped with.
 * @param nPlacement Where the pair goes.
 * @ghidraAddress 0x004620b0
 */
void LoadSoundBank(char *pszBdPath, char *pszHdPath, int nTag, int nPlacement);

/**
 * Report the uncompressed length of a file.
 *
 * The routine belongs to the file layer and is declared here so both bank starters can call it. It
 * opens the path, measures it from the ark directory record, the gzip trailer, or the file size,
 * and reports the length without reporting the handle, which is why a caller that needs the file
 * opens it again.
 *
 * @param pszPath The file to measure.
 * @return The uncompressed length, or zero or less when the file could not be opened.
 * @ghidraAddress 0x00555800
 */
int GetUncompressedFileLength(char *pszPath);

/**
 * Start a chunked BD bank transfer and report the bank's size.
 *
 * Fills the bank block, measures the file, then opens it and queues the first chunk against a new
 * CallbackXferBdToIop. The measured length reaches the block whether or not the measurement
 * succeeded.
 *
 * @param pszPath The bank to read.
 * @return The bank's size, or -1 when the file could not be measured.
 * @ghidraAddress 0x00461f28
 */
int StartBdBankXfer(char *pszPath);

/**
 * Start an HD bank transfer.
 *
 * Unlike the BD transfer this queues the whole file as one read against g_hdXfer. It selects the
 * IOP address the same way LoadSoundBank() selects a destination, and reports `Can't alloc heap`
 * when the selected address is negative, which is what a failed allocation on the IOP side leaves
 * there. The read buffer is the bank's size plus 0x40, rounded up to a 64-byte boundary.
 *
 * The body is not reconstructed, for the same reason as StartBdBankXfer().
 *
 * @param pszPath The bank to read.
 * @param nPlacement Where the bank goes on the IOP.
 * @return Zero once the read is queued, or -1 on either failure.
 * @ghidraAddress 0x00461c68
 */
int StartHdBankXfer(char *pszPath, int nPlacement);

/**
 * Path of the BD bank currently loaded.
 *
 * Both this and g_hdBankName are declared rather than defined. The static initialiser at
 * `0x00464170` zeroes each as two words with no constructor call, which is a default constructor
 * inlined from the header, and HxStr has no default constructor declared yet.
 *
 * @ghidraAddress 0x006e9b90
 */
extern HxStr g_bdBankName;

/**
 * Path of the HD bank currently loaded.
 *
 * @ghidraAddress 0x006e9b98
 */
extern HxStr g_hdBankName;

/** Destination in sound memory a placement of 0 or 1 selects. */
constexpr int kBankFixedDestAddress = 0x5010;

/** Placement that selects the first destination buffer. */
constexpr int kBankPlacementFirstBuffer = 2;

/** Placement that rotates through the destination buffers and the IOP addresses. */
constexpr int kBankPlacementRotate = 3;

/**
 * Bytes of g_szHdBankPath.
 *
 * Bounded by the next known object at `0x00894cc0` rather than measured.
 */
constexpr int kHdBankPathSize = 0x80;

/** Destination buffers the rotating placement alternates between. */
constexpr int kBankDestBufferCount = 2;

/**
 * Destinations in sound memory a bank pair may go to.
 *
 * @ghidraAddress 0x006e9ba8
 */
extern int g_anBankDestAddress[kBankDestBufferCount];

/**
 * Destination buffer the next rotating placement will use.
 *
 * @ghidraAddress 0x006e9bb0
 */
extern int g_nBankDestIndex;

/**
 * Addresses on the IOP a bank may go to.
 *
 * Three rather than two. The first is what a placement of 0 or 1 selects, and the rotation runs
 * through all three once and then cycles between the second and the third.
 */
constexpr int kBankIopAddressCount = 3;

/**
 * Addresses on the IOP a bank may go to.
 *
 * @ghidraAddress 0x00894750
 */
extern int g_anBankIopAddress[kBankIopAddressCount];

/**
 * Entry of g_anBankIopAddress the next rotating placement will use.
 *
 * @ghidraAddress 0x0089475c
 */
extern int g_nBankIopIndex;

/**
 * Path of the HD bank, copied in by StartHdBankXfer().
 *
 * Nothing recovered reads it. The size is bounded by the next known object at `0x00894cc0` rather
 * than measured.
 *
 * @ghidraAddress 0x00894c40
 */
extern char g_szHdBankPath[];

/**
 * Address in sound memory a bank is written to.
 *
 * The routine that starts a transfer copies it into the bank block's mDest, and a BD transfer takes
 * its own starting destination from there and advances one chunk at a time.
 *
 * @ghidraAddress 0x006e9ba4
 */
extern int g_nBankDestAddress;

/**
 * Word every command block is stamped with at `+0x10`.
 *
 * What the driver does with it is unrecovered, and the writer has not been identified.
 *
 * @ghidraAddress 0x006e9bb4
 */
extern int g_nSynthXferTag;

/**
 * Called once after every chunk is submitted and once when a bank transfer finishes.
 *
 * Both call sites test the hook against null first. It takes no arguments.
 *
 * @ghidraAddress 0x006e9bc4
 */
extern void (*g_pfnBankLoadProgress)();

/**
 * Install the bank-load progress hook.
 *
 * @param pfnProgress The hook, which may be null.
 * @ghidraAddress 0x00464378
 */
void SetBankLoadProgressHook(void (*pfnProgress)());

/**
 * Non-zero while an HD bank transfer occupies the shared command block.
 *
 * CallbackXferBdToIop defers to it, and CallbackXferHdToIop::Done() clears it. The routine that
 * sets it is unrecovered.
 *
 * @ghidraAddress 0x006e9dc8
 */
extern int g_nHdXferInFlight;

/**
 * Block that describes a whole bank, submitted under selector 0x1050 when the last chunk lands.
 *
 * The routine that starts a transfer fills it once, at the start, and the completion path submits
 * it unchanged. Its payload is the bank's path, copied in with strcpy(), and the cache is flushed
 * straight afterwards because the driver reads the block from the IOP side.
 *
 * @ghidraAddress 0x00894bc0
 */
extern SoundDriverCommand g_bankCommand;

/**
 * Buffer the HD bank transfer read into, released once the transfer reports.
 *
 * @ghidraAddress 0x006e9dcc
 */
extern void *g_pHdXferBuffer;

/**
 * Buffer the BD bank transfer reads into, released once the last chunk has been moved.
 *
 * This is the allocation as it came back from the heap. The read itself uses the pointer rounded up
 * to a 64-byte boundary, which CallbackXferBdToIop stores separately.
 *
 * @ghidraAddress 0x006e9dd0
 */
extern void *g_pBdXferBuffer;

/**
 * Report the state of both SPU2 cores and all 48 voices to the log.
 *
 * Writes one `Core    %2.2d: MMix %x eff %d vmix L %x %x R %x %x end %x` line per core, one
 * `  Voice %2.2d at %x env %x - end %d mix %d %d %d %d` line per voice, and finally
 * `Using %d voices total`. Every value is read back from the hardware, so the report is the live
 * mixer state rather than anything the driver retains. A voice counts towards the total when its
 * ENDX bit is clear, which is the sample not having run out.
 *
 * @param bActiveOnly Non-zero to report only the voices whose ENDX bit is clear. SynthCommand()
 *                    passes 1, and zero reports all 24 voices of each core.
 * @ghidraAddress 0x00462558
 */
void DumpSynthVoices(int bActiveOnly);

/**
 * Bind libsdr and bring both SPU2 cores up.
 *
 * Sets the two mixer routings, silences the effect send, opens both master volumes fully, and gives
 * each core a 128 KB effect work area at the top of sound RAM, core 0 taking the highest block.
 * Core 1's mixer routing takes the external input that core 0's does not, which is what chains the
 * two cores.
 *
 * InitSynthDriver() is the only caller.
 *
 * @ghidraAddress 0x004649f8
 */
void InitSpu2Cores();

/**
 * Apply the configured reverb to both SPU2 cores.
 *
 * Per core, the routine enables the effect only when the argument is non-zero and the configuration
 * also enables it for that core; otherwise it clears both effect volumes and turns the core
 * attribute off. Either way it then reopens both master volumes fully. When effects are enabled it
 * fills a `sceSdEffectAttr` from five configuration values, ORs `SD_EFFECT_MODE_CLEAR` into the
 * mode, and both depths are the configured value shifted left by eight.
 *
 * Enabling also submits a further command block to the sound driver under selector 0x10e0, built
 * from eleven more configuration values.
 *
 * The body is not reconstructed. It reads every value through the two variadic configuration
 * queries at `0x00509110` and `0x005093e0`, whose signatures belong to the data-array layer.
 *
 * @param bEnable Zero to force the effect off on both cores.
 * @ghidraAddress 0x00462340
 */
void ConfigureSpu2Effects(int bEnable);

/**
 * Bring the voice and bank driver up.
 *
 * CreatePs2HardSynth() calls this immediately after construction, which is the seam between the
 * Synth class and this module.
 *
 * @ghidraAddress 0x004647a8
 */
void InitSynthDriver();

/**
 * Advance the streaming voice buffers.
 *
 * Performs no work while the stream at `0x006f9c58` is absent. MainLoop::Poll() drives this once
 * per frame.
 *
 * @ghidraAddress 0x00464bc8
 */
void PollSynthStream();

/**
 * Advance the driver's pending events.
 *
 * One of MainLoop's two periodic timers drives this.
 *
 * @ghidraAddress 0x004648c8
 */
void PollSynthEvents();
