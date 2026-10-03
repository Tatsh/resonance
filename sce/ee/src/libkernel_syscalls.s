# The kernel system call stubs the game uses. Each loads its call number into v1 and traps. A
# negative number selects the form that is safe inside an interrupt handler.

    .set noreorder

    .macro SYSCALL_STUB name, number
    .globl \name
    .type \name, @function
    .ent \name
\name:
    addiu $v1, $zero, \number
    syscall
    jr $ra
    nop
    .end \name
    .size \name, . - \name
    .endm

    .text

# NTSC-U/C: 0x005366e0, PAL: 0x00575fa0
    SYSCALL_STUB SetGsCrt, 2

# NTSC-U/C: 0x00536700, PAL: 0x00575fc0
    SYSCALL_STUB Exit, 4

# NTSC-U/C: 0x005367c0, PAL: 0x00576080
    SYSCALL_STUB AddIntcHandler, 16

# NTSC-U/C: 0x005367e0, PAL: 0x005760a0
    SYSCALL_STUB RemoveIntcHandler, 17

# NTSC-U/C: 0x005367f0, PAL: 0x005760b0
    SYSCALL_STUB AddDmacHandler, 18

# NTSC-U/C: 0x00536810, PAL: 0x005760d0
    SYSCALL_STUB RemoveDmacHandler, 19

# NTSC-U/C: 0x00536820, PAL: 0x005760e0
    SYSCALL_STUB _EnableIntc, 20

# NTSC-U/C: 0x00536830, PAL: 0x005760f0
    SYSCALL_STUB _DisableIntc, 21

# NTSC-U/C: 0x00536840, PAL: 0x00576100
    SYSCALL_STUB _EnableDmac, 22

# NTSC-U/C: 0x00536850, PAL: 0x00576110
    SYSCALL_STUB _DisableDmac, 23

# NTSC-U/C: 0x00536860, PAL: 0x00576120
    SYSCALL_STUB SetAlarm, 252

# NTSC-U/C: 0x005368e0, PAL: 0x005761a0
    SYSCALL_STUB CreateThread, 32

# NTSC-U/C: 0x005368f0, PAL: 0x005761b0
    SYSCALL_STUB DeleteThread, 33

# NTSC-U/C: 0x00536900, PAL: 0x005761c0
    SYSCALL_STUB StartThread, 34

# NTSC-U/C: 0x00536920, PAL: 0x005761e0
    SYSCALL_STUB ExitDeleteThread, 36

# NTSC-U/C: 0x00536930, PAL: 0x005761f0
    SYSCALL_STUB TerminateThread, 37

# NTSC-U/C: 0x00536970, PAL: 0x00576230
    SYSCALL_STUB ChangeThreadPriority, 41

# NTSC-U/C: 0x00536990, PAL: 0x00576250
    SYSCALL_STUB RotateThreadReadyQueue, 43

# NTSC-U/C: 0x005369d0, PAL: 0x00576290
    SYSCALL_STUB GetThreadId, 47

# NTSC-U/C: 0x005369e0, PAL: 0x005762a0
    SYSCALL_STUB ReferThreadStatus, 48

# NTSC-U/C: 0x00536a00, PAL: 0x005762c0
    SYSCALL_STUB SleepThread, 50

# NTSC-U/C: 0x00536a10, PAL: 0x005762d0
    SYSCALL_STUB WakeupThread, 51

# NTSC-U/C: 0x00536a20, PAL: 0x005762e0
    SYSCALL_STUB _iWakeupThread, -52

# NTSC-U/C: 0x00536a50, PAL: 0x00576310
    SYSCALL_STUB SuspendThread, 55

# NTSC-U/C: 0x00536ac0, PAL: 0x00576380
    SYSCALL_STUB EndOfHeap, 62

# NTSC-U/C: 0x00536ae0, PAL: 0x005763a0
    SYSCALL_STUB CreateSema, 64

# NTSC-U/C: 0x00536af0, PAL: 0x005763b0
    SYSCALL_STUB DeleteSema, 65

# NTSC-U/C: 0x00536b00, PAL: 0x005763c0
    SYSCALL_STUB SignalSema, 66

# NTSC-U/C: 0x00536b10, PAL: 0x005763d0
    SYSCALL_STUB iSignalSema, -67

# NTSC-U/C: 0x00536b20, PAL: 0x005763e0
    SYSCALL_STUB WaitSema, 68

# NTSC-U/C: 0x00536b30, PAL: 0x005763f0
    SYSCALL_STUB PollSema, 69

# NTSC-U/C: 0x00536b90, PAL: 0x00576450
    SYSCALL_STUB GetOsdConfigParam, 75

# NTSC-U/C: 0x00536d60, PAL: 0x00576620
    SYSCALL_STUB FlushCache, 100

# NTSC-U/C: 0x00536df0, PAL: 0x005766b0
    SYSCALL_STUB GetOsdConfigParam2, 111

# NTSC-U/C: 0x00536e00, PAL: 0x005766c0
    SYSCALL_STUB GsGetIMR, 112

# NTSC-U/C: 0x00536e20, PAL: 0x005766e0
    SYSCALL_STUB GsPutIMR, 113

# NTSC-U/C: 0x00536e50, PAL: 0x00576710
    SYSCALL_STUB SetVSyncFlag, 115

# NTSC-U/C: 0x00536f10, PAL: 0x005767d0
    SYSCALL_STUB Deci2Call, 124

# NTSC-U/C: 0x00589138, PAL: 0x005cc3b0
    SYSCALL_STUB SetSyscall, 116

# NTSC-U/C: 0x00589148, PAL: 0x005cc3c0
    SYSCALL_STUB KernelCopy, 90

# NTSC-U/C: 0x00589190, PAL: 0x005cc408
    SYSCALL_STUB GetEntryAddress, 91
