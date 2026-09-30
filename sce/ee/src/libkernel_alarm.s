# The alarm extension the game installs in the kernel. InstallSyscallPatch() copies the patch to
# kAlarmPatchBase and the handler trampoline to kAlarmHandlerTrampoline. Every address the patch
# uses is the address of the copy. The alarms count horizontal blanks on timer 3.

    .set noreorder
    .set noat

# Registers 8 to 15 are numbered. Their names differ between the ABIs.

    .equ kAlarmPatchBase, 0x80076000
    .equ kAlarmCount, 0x80076700
    .equ kAlarmIdMask, 0x80076708
    .equ kAlarmEntryTable, 0x80076710
    .equ kAlarmQueue, 0x80076740
    .equ kAlarmSavedRa, 0x80076c40
    .equ kAlarmSavedSp, 0x80076c50
    .equ kAlarmHandlerTrampoline, 0x00082000
    .equ kAlarmHandlerStack, 0x00081fc0
    .equ kAlarmGetEntryAddressOffset, 0x0
    .equ kAlarmUnwrapTimeOffset, 0x38
    .equ kAlarmFindSlotOffset, 0x58
    .equ kAlarmInsertOffset, 0x160
    .equ kAlarmReleaseOffset, 0x2a0
    .equ kAlarmSetOffset, 0x440
    .equ kAlarmSetCompareOffset, 0x460
    .equ kAlarmInterruptOffset, 0x488
    .equ kAlarmCallHandlerOffset, 0x680
    .equ kAlarmReturnFromHandlerOffset, 0x6c0
    .equ kAlarmDataOffset, 0x700
    .equ kAlarmPatchSize, 1856
    .equ kAlarmTrampolineSize, 40
    .equ kAlarmEntrySize, 20
    .equ kAlarmMax, 64
    .equ kAlarmEntryCount, 6
    .equ kTimerPeriod, 0x10000
    .equ kT3ModeStop, 0x83
    .equ kT3ModeIdle, 0x483
    .equ kT3ModeRun, 0x583
    .equ kIntcStatTimer3, 0x1000
    .equ kStatusIe, 0x01
    .equ kStatusExl, 0x02
    .equ kStatusUserMode, 0x10
    .equ kStatusKsuMask, 0x18
    .equ kT3Count, 0xb0001800
    .equ kT3Mode, 0xb0001810
    .equ kT3Comp, 0xb0001820
    .equ kIntcStat, 0x1000f000
    .equ kSyscallSetAlarm, 252
    .equ kSyscallReleaseAlarm, 253
    .equ kSyscallISetAlarm, 254
    .equ kSyscallIReleaseAlarm, 255
    .equ kSyscallAlarmInterrupt, 300
    .equ kSyscallAlarmReturn, 8
    .equ kSyscallAlarmReturnUser, -8

    .data
    .align 3

# 0x007690b8
    .globl g_alarmPatch
g_alarmPatch:
AlarmPatchStart:
# 0x80076000
# Returns the kernel address of an alarm system call from the entry table, or zero.
    .org AlarmPatchStart + kAlarmGetEntryAddressOffset
AlarmGetEntryAddress:
    lui	$v0,%hi(kAlarmEntryTable)
    daddu	$a1,$zero,$zero
    addiu	$v1,$v0,%lo(kAlarmEntryTable)
    nop
.L0:
    lw	$v0,0($v1)
    bne	$a0,$v0,.L1
    addiu	$a1,$a1,1
    jr	$ra
    lw	$v0,4($v1)
.L1:
    sltiu	$v0,$a1,kAlarmEntryCount
    bne	$v0,$zero,.L0
    addiu	$v1,$v1,8
    jr	$ra
    daddu	$v0,$zero,$zero

# 0x80076038
# Returns a 16-bit timer value, adding one timer period when it lies before the base value.
    .org AlarmPatchStart + kAlarmUnwrapTimeOffset
AlarmUnwrapTime:
    slt	$a0,$a1,$a0
    beq	$a0,$zero,.L2
    lui	$v0,kTimerPeriod >> 16
    jr	$ra
    or	$v0,$a1,$v0
.L2:
    jr	$ra
    daddu	$v0,$a1,$zero
    nop

# 0x80076058
# Returns the queue index for a target time and moves every later alarm up one entry.
    .org AlarmPatchStart + kAlarmFindSlotOffset
AlarmFindSlot:
    addiu	$sp,$sp,-128
    sd	$s3,48($sp)
    lui	$s3,%hi(kAlarmCount)
    sd	$s6,96($sp)
    sd	$s5,80($sp)
    daddu	$s6,$a0,$zero
    sd	$s0,0($sp)
    daddu	$s5,$a1,$zero
    lw	$v0,%lo(kAlarmCount)($s3)
    daddu	$s0,$zero,$zero
    sd	$ra,112($sp)
    sd	$s4,64($sp)
    sd	$s2,32($sp)
    blez	$v0,.L6
    sd	$s1,16($sp)
    lui	$s4,%hi(kAlarmQueue)
    addiu	$s2,$zero,kAlarmEntrySize
    nop
.L3:
    addiu	$s1,$s4,%lo(kAlarmQueue)
    mult	$v0,$s0,$s2
    daddu	$a0,$s6,$zero
    addu	$v0,$v0,$s1
    jal	kAlarmPatchBase + kAlarmUnwrapTimeOffset
    lhu	$a1,0($v0)
    slt	$v0,$s5,$v0
    beq	$v0,$zero,.L5
    lw	$v0,%lo(kAlarmCount)($s3)
    addiu	$a0,$v0,-1
    slt	$v1,$a0,$s0
    bne	$v1,$zero,.L6
    mult	$v0,$a0,$s2
    addu	$v1,$v0,$s1
.L4:
    ldl	$a1,7($v1)
    ldr	$a1,0($v1)
    ldl	$a2,15($v1)
    ldr	$a2,8($v1)
    lw	$a3,16($v1)
    sdl	$a1,27($v1)
    sdr	$a1,20($v1)
    sdl	$a2,35($v1)
    sdr	$a2,28($v1)
    sw	$a3,36($v1)
    addiu	$a0,$a0,-1
    addiu	$v1,$v1,-20
    slt	$v0,$a0,$s0
    nop
    beq	$v0,$zero,.L4
    nop
    beq	$zero,$zero,.L7
    daddu	$v0,$s0,$zero
.L5:
    addiu	$s0,$s0,1
    slt	$v0,$s0,$v0
    bne	$v0,$zero,.L3
    addiu	$s2,$zero,kAlarmEntrySize
.L6:
    daddu	$v0,$s0,$zero
.L7:
    ld	$ra,112($sp)
    ld	$s6,96($sp)
    ld	$s5,80($sp)
    ld	$s4,64($sp)
    ld	$s3,48($sp)
    ld	$s2,32($sp)
    ld	$s1,16($sp)
    ld	$s0,0($sp)
    jr	$ra
    addiu	$sp,$sp,128
    nop

# 0x80076160
# Queues an alarm that fires a number of horizontal blanks from now and returns its identifier,
# or -1 when all 64 identifiers are in use.
    .org AlarmPatchStart + kAlarmInsertOffset
AlarmInsert:
    addiu	$sp,$sp,-144
    lui	$v0,kT3Count >> 16
    sd	$s7,112($sp)
    ori	$v0,$v0,kT3Count & 0xffff
    sd	$s6,96($sp)
    daddu	$s7,$a2,$zero
    sd	$s5,80($sp)
    daddu	$s6,$a1,$zero
    sd	$s3,48($sp)
    lui	$s5,%hi(kAlarmCount)
    sd	$ra,128($sp)
    andi	$s3,$a0,0xffff
    sd	$s2,32($sp)
    sd	$s1,16($sp)
    sd	$s0,0($sp)
    sd	$s4,64($sp)
    lw	$v1,%lo(kAlarmCount)($s5)
    lw	$s4,0($v0)
    slti	$v1,$v1,kAlarmMax
    bne	$v1,$zero,.L9
    addu	$s3,$s3,$s4
    beq	$zero,$zero,.L12
    addiu	$v0,$zero,-1
.L8:
    daddu	$s2,$v1,$zero
    dsllv	$v0,$v0,$v1
    or	$v0,$a0,$v0
    beq	$zero,$zero,.L11
    sd	$v0,%lo(kAlarmIdMask)($a1)
.L9:
    lui	$a1,%hi(kAlarmIdMask)
    daddu	$v1,$zero,$zero
    ld	$a0,%lo(kAlarmIdMask)($a1)
    dsrlv	$v0,$a0,$v1
.L10:
    andi	$v0,$v0,0x1
    beq	$v0,$zero,.L8
    addiu	$v0,$zero,1
    addiu	$v1,$v1,1
    slti	$v0,$v1,kAlarmMax
    bne	$v0,$zero,.L10
    dsrlv	$v0,$a0,$v1
    addiu	$s2,$zero,-1
.L11:
    bltz	$s2,.L12
    daddu	$v0,$s2,$zero
    daddu	$s1,$gp,$zero
    daddu	$a1,$s3,$zero
    jal	kAlarmPatchBase + kAlarmFindSlotOffset
    daddu	$a0,$s4,$zero
    addiu	$a0,$zero,kAlarmEntrySize
    lui	$8,%hi(kAlarmQueue)
    mult	$v0,$v0,$a0
    addiu	$v1,$8,%lo(kAlarmQueue)
    lw	$a1,%lo(kAlarmCount)($s5)
    addiu	$s0,$v1,4
    addiu	$a1,$a1,1
    addu	$a0,$v0,$v1
    addu	$a3,$v1,$v0
    addu	$s0,$v0,$s0
    sh	$s4,2($a0)
    sh	$s3,0($a0)
    daddu	$a2,$a3,$zero
    sw	$s2,0($s0)
    daddu	$v1,$a2,$zero
    sw	$s1,16($a3)
    lhu	$a0,%lo(kAlarmQueue)($8)
    sw	$s6,8($a2)
    sw	$s7,12($v1)
    jal	kAlarmPatchBase + kAlarmSetCompareOffset
    sw	$a1,%lo(kAlarmCount)($s5)
    lw	$v0,0($s0)
.L12:
    ld	$ra,128($sp)
    ld	$s7,112($sp)
    ld	$s6,96($sp)
    ld	$s5,80($sp)
    ld	$s4,64($sp)
    ld	$s3,48($sp)
    ld	$s2,32($sp)
    ld	$s1,16($sp)
    ld	$s0,0($sp)
    jr	$ra
    addiu	$sp,$sp,144
    nop

# 0x800762a0
# Removes a queued alarm and returns the time it had waited, or -1 when the identifier is not
# queued or the alarm is firing.
    .org AlarmPatchStart + kAlarmReleaseOffset
AlarmRelease:
    addiu	$sp,$sp,-48
    lui	$12,%hi(kAlarmCount)
    sd	$s1,16($sp)
    daddu	$13,$a0,$zero
    lw	$v0,%lo(kAlarmCount)($12)
    daddu	$s1,$12,$zero
    sd	$ra,32($sp)
    addiu	$a2,$zero,-1
    blez	$v0,.L21
    sd	$s0,0($sp)
    blez	$v0,.L21
    daddu	$8,$zero,$zero
    lui	$11,%hi(kAlarmQueue)
    addiu	$v1,$zero,kAlarmEntrySize
.L13:
    addiu	$a1,$11,%lo(kAlarmQueue)
    mult	$a0,$8,$v1
    addu	$v0,$a1,$a0
    lw	$v1,4($v0)
    bne	$13,$v1,.L20
    lw	$v0,%lo(kAlarmCount)($12)
    lui	$v1,kT3Comp >> 16
    addu	$a0,$a0,$a1
    ori	$v1,$v1,kT3Comp & 0xffff
    lhu	$a1,0($a0)
    lw	$v0,0($v1)
    bne	$a1,$v0,.L14
    addiu	$v1,$zero,kAlarmEntrySize
    lui	$v0,kIntcStat >> 16
    ori	$v0,$v0,kIntcStat & 0xffff
    lw	$v1,0($v0)
    andi	$v1,$v1,kIntcStatTimer3
    bne	$v1,$zero,.L22
    addiu	$v0,$zero,-1
    addiu	$v1,$zero,kAlarmEntrySize
.L14:
    lw	$9,%lo(kAlarmCount)($12)
    mult	$v1,$8,$v1
    addiu	$a0,$11,%lo(kAlarmQueue)
    addiu	$v0,$9,-1
    daddu	$a3,$8,$zero
    slt	$v0,$8,$v0
    addu	$v1,$v1,$a0
    beq	$v0,$zero,.L16
    lhu	$s0,2($v1)
    lui	$10,%hi(kAlarmIdMask)
.L15:
    addiu	$v1,$a3,1
    addiu	$a1,$zero,kAlarmEntrySize
    mult	$v0,$v1,$a1
    mult	$a0,$a3,$a1
    addiu	$a2,$11,%lo(kAlarmQueue)
    daddu	$a3,$v1,$zero
    addu	$a1,$v0,$a2
    addu	$a0,$a0,$a2
    addiu	$v0,$9,-1
    ldl	$v1,7($a1)
    ldr	$v1,0($a1)
    ldl	$a2,15($a1)
    ldr	$a2,8($a1)
    lw	$14,16($a1)
    sdl	$v1,7($a0)
    sdr	$v1,0($a0)
    sdl	$a2,15($a0)
    sdr	$a2,8($a0)
    slt	$v0,$a3,$v0
    bne	$v0,$zero,.L15
    sw	$14,16($a0)
    beq	$zero,$zero,.L17
    addiu	$v0,$zero,1
.L16:
    lui	$10,%hi(kAlarmIdMask)
    addiu	$v0,$zero,1
.L17:
    lw	$a0,%lo(kAlarmCount)($12)
    ld	$v1,%lo(kAlarmIdMask)($10)
    dsllv	$v0,$v0,$13
    nor	$v0,$zero,$v0
    addiu	$a0,$a0,-1
    and	$v1,$v1,$v0
    sw	$a0,%lo(kAlarmCount)($12)
    bne	$8,$zero,.L18
    sd	$v1,%lo(kAlarmIdMask)($10)
    jal	kAlarmPatchBase + kAlarmSetCompareOffset
    lhu	$a0,%lo(kAlarmQueue)($11)
.L18:
    lw	$v0,%lo(kAlarmCount)($s1)
    bne	$v0,$zero,.L19
    addiu	$v1,$zero,kT3ModeStop
    lui	$v0,kT3Mode >> 16
    ori	$v0,$v0,kT3Mode & 0xffff
    sw	$v1,0($v0)
.L19:
    lui	$v0,kT3Count >> 16
    daddu	$a0,$s0,$zero
    ori	$v0,$v0,kT3Count & 0xffff
    jal	kAlarmPatchBase + kAlarmUnwrapTimeOffset
    lw	$a1,0($v0)
    beq	$zero,$zero,.L21
    subu	$a2,$v0,$s0
.L20:
    addiu	$8,$8,1
    slt	$v0,$8,$v0
    bne	$v0,$zero,.L13
    addiu	$v1,$zero,kAlarmEntrySize
.L21:
    sync
    daddu	$v0,$a2,$zero
.L22:
    ld	$ra,32($sp)
    ld	$s1,16($sp)
    ld	$s0,0($sp)
    jr	$ra
    addiu	$sp,$sp,48

# 0x80076440
# The SetAlarm and iSetAlarm system call.
    .org AlarmPatchStart + kAlarmSetOffset
AlarmSet:
    addiu	$sp,$sp,-16
    sd	$ra,0($sp)
    jal	kAlarmPatchBase + kAlarmInsertOffset
    andi	$a0,$a0,0xffff
    sync
    ld	$ra,0($sp)
    jr	$ra
    addiu	$sp,$sp,16

# 0x80076460
# Programs timer 3 to interrupt at a target count.
    .org AlarmPatchStart + kAlarmSetCompareOffset
AlarmSetCompare:
    lui	$v0,kT3Comp >> 16
    ori	$v0,$v0,kT3Comp & 0xffff
    sw	$a0,0($v0)
    sync
    lui	$v0,kT3Mode >> 16
    addiu	$v1,$zero,kT3ModeRun
    ori	$v0,$v0,kT3Mode & 0xffff
    jr	$ra
    sw	$v1,0($v0)
    nop

# 0x80076488
# The timer 3 interrupt handler. Runs every alarm whose target has arrived and programs the next.
    .org AlarmPatchStart + kAlarmInterruptOffset
AlarmInterrupt:
    addiu	$sp,$sp,-176
    daddu	$8,$zero,$zero
    sd	$ra,160($sp)
    sd	$s7,144($sp)
    sd	$s6,128($sp)
    sd	$s5,112($sp)
    sd	$s4,96($sp)
    sd	$s3,80($sp)
    sd	$s2,64($sp)
    sd	$s1,48($sp)
    sd	$s0,32($sp)
    lui	$s1,%hi(kAlarmCount)
    lui	$s2,%hi(kAlarmQueue)
    nop
.L23:
    lw	$v0,%lo(kAlarmCount)($s1)
    slt	$v0,$8,$v0
    beq	$v0,$zero,.L24
    addiu	$v1,$zero,kAlarmEntrySize
    addiu	$a0,$s2,%lo(kAlarmQueue)
    mult	$v1,$8,$v1
    lhu	$a1,%lo(kAlarmQueue)($s2)
    addu	$v1,$v1,$a0
    lhu	$v0,0($v1)
    beq	$a1,$v0,.L23
    addiu	$8,$8,1
    jal	kAlarmPatchBase + kAlarmSetCompareOffset
    daddu	$a0,$v0,$zero
.L24:
    lui	$v0,%hi(kAlarmQueue)
    daddu	$s6,$s1,$zero
    addiu	$s4,$v0,%lo(kAlarmQueue)
    addiu	$s3,$zero,kAlarmEntrySize
    lui	$s5,%hi(kAlarmIdMask)
    beq	$zero,$zero,.L26
    addiu	$s7,$zero,1
.L25:
    lhu	$v0,%lo(kAlarmQueue)($s2)
    bne	$v1,$v0,.L30
    lw	$v0,%lo(kAlarmCount)($s1)
.L26:
    lw	$v0,%lo(kAlarmCount)($s6)
    daddu	$8,$zero,$zero
    addiu	$a2,$s2,%lo(kAlarmQueue)
    ldl	$v1,7($a2)
    ldr	$v1,0($a2)
    ldl	$a0,15($a2)
    ldr	$a0,8($a2)
    lw	$a1,16($a2)
    sdl	$v1,7($sp)
    sdr	$v1,0($sp)
    sdl	$a0,15($sp)
    sdr	$a0,8($sp)
    sw	$a1,16($sp)
    addiu	$v0,$v0,-1
    blez	$v0,.L28
    sw	$v0,%lo(kAlarmCount)($s6)
    lw	$9,%lo(kAlarmCount)($s1)
    lw	$10,16($sp)
    lw	$a2,4($sp)
    lhu	$a3,0($sp)
    nop
.L27:
    mult	$v1,$8,$s3
    addiu	$v0,$8,1
    daddu	$8,$v0,$zero
    addu	$a1,$v1,$s4
    mult	$v1,$v0,$s3
    addu	$a0,$v1,$s4
    ldl	$11,7($a0)
    ldr	$11,0($a0)
    ldl	$12,15($a0)
    ldr	$12,8($a0)
    lw	$13,16($a0)
    sdl	$11,7($a1)
    sdr	$11,0($a1)
    sdl	$12,15($a1)
    sdr	$12,8($a1)
    slt	$v1,$8,$9
    bne	$v1,$zero,.L27
    sw	$13,16($a1)
    beq	$zero,$zero,.L29
    nop
.L28:
    lw	$10,16($sp)
    lw	$a2,4($sp)
    lhu	$a3,0($sp)
.L29:
    daddu	$s0,$gp,$zero
    daddu	$gp,$10,$zero
    ld	$v1,%lo(kAlarmIdMask)($s5)
    dsllv	$v0,$s7,$a2
    nor	$v0,$zero,$v0
    lui	$a0,kAlarmHandlerTrampoline >> 16
    and	$v1,$v1,$v0
    lw	$a1,8($sp)
    lw	$8,12($sp)
    ori	$a0,$a0,kAlarmHandlerTrampoline & 0xffff
    jal	kAlarmPatchBase + kAlarmCallHandlerOffset
    sd	$v1,%lo(kAlarmIdMask)($s5)
    daddu	$gp,$s0,$zero
    lw	$v0,%lo(kAlarmCount)($s1)
    bgtz	$v0,.L25
    lhu	$v1,0($sp)
    lw	$v0,%lo(kAlarmCount)($s1)
.L30:
    blez	$v0,.L31
    addiu	$v1,$zero,kT3ModeIdle
    jal	kAlarmPatchBase + kAlarmSetCompareOffset
    lhu	$a0,%lo(kAlarmQueue)($s2)
    beq	$zero,$zero,.L32
    nop
.L31:
    lui	$v0,kT3Mode >> 16
    ori	$v0,$v0,kT3Mode & 0xffff
    sw	$v1,0($v0)
.L32:
    sync
    ei
    ld	$ra,160($sp)
    ld	$s7,144($sp)
    ld	$s6,128($sp)
    ld	$s5,112($sp)
    ld	$s4,96($sp)
    ld	$s3,80($sp)
    ld	$s2,64($sp)
    ld	$s1,48($sp)
    ld	$s0,32($sp)
    jr	$ra
    addiu	$sp,$sp,176

# 0x80076680
# Exits the kernel to run an alarm handler. The handler runs in user mode through the trampoline.
# The trampoline returns through system call 8.
    .org AlarmPatchStart + kAlarmCallHandlerOffset
AlarmCallHandler:
    lui	$k0,%hi(kAlarmSavedRa)
    sw	$ra,%lo(kAlarmSavedRa)($k0)
    lui	$k0,%hi(kAlarmSavedSp)
    sw	$sp,%lo(kAlarmSavedSp)($k0)
    mtc0	$a0,$14
    sync.p
    daddu	$v1,$a1,$zero
    daddu	$a0,$a2,$zero
    daddu	$a1,$a3,$zero
    daddu	$a2,$8,$zero
    mfc0	$k0,$12
    ori	$k0,$k0,kStatusUserMode | kStatusExl
    mtc0	$k0,$12
    sync.p
    eret
    nop

# 0x800766c0
# System call 8. Restores kernel mode and returns to the caller of AlarmCallHandler.
    .org AlarmPatchStart + kAlarmReturnFromHandlerOffset
AlarmReturnFromHandler:
    mfc0	$at,$12
    addiu	$k0,$zero,~(kStatusKsuMask | kStatusExl | kStatusIe)
    and	$at,$at,$k0
    mtc0	$at,$12
    sync.p
    lui	$k0,%hi(kAlarmSavedRa)
    lw	$ra,%lo(kAlarmSavedRa)($k0)
    lui	$k0,%hi(kAlarmSavedSp)
    jr	$ra
    lw	$sp,%lo(kAlarmSavedSp)($k0)

    .org AlarmPatchStart + kAlarmDataOffset
    .word 0 # The number of queued alarms.
    .word 0
    .dword 0 # The identifiers in use, one bit each.
    .word kSyscallSetAlarm, kAlarmPatchBase + kAlarmSetOffset
    .word kSyscallISetAlarm, kAlarmPatchBase + kAlarmSetOffset
    .word kSyscallReleaseAlarm, kAlarmPatchBase + kAlarmReleaseOffset
    .word kSyscallIReleaseAlarm, kAlarmPatchBase + kAlarmReleaseOffset
    .word kSyscallAlarmInterrupt, kAlarmPatchBase + kAlarmInterruptOffset
    .word kSyscallAlarmReturn, kAlarmPatchBase + kAlarmReturnFromHandlerOffset
    .org AlarmPatchStart + kAlarmPatchSize

# 0x007697f8
# Calls an alarm handler in user mode on its own stack and returns to the kernel afterwards.
    .globl g_alarmTrampoline
g_alarmTrampoline:
AlarmTrampolineStart:
    lui $sp, kAlarmHandlerStack >> 16
    jalr $v1
    addiu $sp, $sp, kAlarmHandlerStack & 0xffff
    addiu $v1, $zero, kSyscallAlarmReturnUser
    syscall
    .org AlarmTrampolineStart + kAlarmTrampolineSize
