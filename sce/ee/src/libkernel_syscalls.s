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

# 0x005366e0
    SYSCALL_STUB SetGsCrt, 2

# 0x00536700
    SYSCALL_STUB Exit, 4

# 0x005367c0
    SYSCALL_STUB AddIntcHandler, 16

# 0x005367e0
    SYSCALL_STUB RemoveIntcHandler, 17

# 0x005367f0
    SYSCALL_STUB AddDmacHandler, 18

# 0x00536810
    SYSCALL_STUB RemoveDmacHandler, 19

# 0x00536820
    SYSCALL_STUB _EnableIntc, 20

# 0x00536830
    SYSCALL_STUB _DisableIntc, 21

# 0x00536840
    SYSCALL_STUB _EnableDmac, 22

# 0x00536850
    SYSCALL_STUB _DisableDmac, 23

# 0x00536860
    SYSCALL_STUB SetAlarm, 252

# 0x005368e0
    SYSCALL_STUB CreateThread, 32

# 0x005368f0
    SYSCALL_STUB DeleteThread, 33

# 0x00536900
    SYSCALL_STUB StartThread, 34

# 0x00536920
    SYSCALL_STUB ExitDeleteThread, 36

# 0x00536930
    SYSCALL_STUB TerminateThread, 37

# 0x00536970
    SYSCALL_STUB ChangeThreadPriority, 41

# 0x00536990
    SYSCALL_STUB RotateThreadReadyQueue, 43

# 0x005369d0
    SYSCALL_STUB GetThreadId, 47

# 0x005369e0
    SYSCALL_STUB ReferThreadStatus, 48

# 0x00536a00
    SYSCALL_STUB SleepThread, 50

# 0x00536a10
    SYSCALL_STUB WakeupThread, 51

# 0x00536a20
    SYSCALL_STUB _iWakeupThread, -52

# 0x00536a50
    SYSCALL_STUB SuspendThread, 55

# 0x00536ac0
    SYSCALL_STUB EndOfHeap, 62

# 0x00536ae0
    SYSCALL_STUB CreateSema, 64

# 0x00536af0
    SYSCALL_STUB DeleteSema, 65

# 0x00536b00
    SYSCALL_STUB SignalSema, 66

# 0x00536b10
    SYSCALL_STUB iSignalSema, -67

# 0x00536b20
    SYSCALL_STUB WaitSema, 68

# 0x00536b30
    SYSCALL_STUB PollSema, 69

# 0x00536b90
    SYSCALL_STUB GetOsdConfigParam, 75

# 0x00536d60
    SYSCALL_STUB FlushCache, 100

# 0x00536df0
    SYSCALL_STUB GetOsdConfigParam2, 111

# 0x00536e00
    SYSCALL_STUB GsGetIMR, 112

# 0x00536e20
    SYSCALL_STUB GsPutIMR, 113

# 0x00536e50
    SYSCALL_STUB SetVSyncFlag, 115

# 0x00536f10
    SYSCALL_STUB Deci2Call, 124

# 0x00589138
    SYSCALL_STUB SetSyscall, 116

# 0x00589148
    SYSCALL_STUB KernelCopy, 90

# 0x00589190
    SYSCALL_STUB GetEntryAddress, 91
