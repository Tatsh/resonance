# Import tables of EZMIDI.IRX. The IOP loader finds each table by its magic word, resolves the
# library by name and version, and rewrites every two-instruction stub below the header into a
# jump to the export with the index that the stub's second instruction records.

	.set	noreorder
	.set	noat
	.text

# Each library header is the magic word, a link word the loader fills, the version, and the name
# padded to eight bytes.

# Emits one stub. The loader patches its jr into a j to the export numbered \index.
	.macro	import name, index
	.globl	\name
	.type	\name, @function
\name:
	jr	$31
	addiu	$0, $0, \index
	.endm

# Two zero words close a table.
	.macro	import_end
	.word	0
	.word	0
	.endm

	.globl	libsd_stub
libsd_stub:
	.word	0x41e00000
	.word	0
	.word	0x00000104
	.ascii	"libsd\0\0\0"
	import	sceSdSetParam, 5
	import	sceSdGetParam, 6
	import	sceSdSetSwitch, 7
	import	sceSdGetSwitch, 8
	import	sceSdSetAddr, 9
	import	sceSdNote2Pitch, 13
	import	sceSdVoiceTrans, 17
	import	sceSdVoiceTransStatus, 19
	import_end

	.globl	intrman_stub
intrman_stub:
	.word	0x41e00000
	.word	0
	.word	0x00000102
	.ascii	"intrman\0"
	import	EnableIntr, 6
	import	CpuEnableIntr, 9
	import_end

	.globl	sifcmd_stub
sifcmd_stub:
	.word	0x41e00000
	.word	0
	.word	0x00000101
	.ascii	"sifcmd\0\0"
	import	sceSifInitRpc, 14
	import	sceSifRegisterRpc, 17
	import	sceSifSetRpcQueue, 19
	import	sceSifRpcLoop, 22
	import_end

	.globl	sifman_stub
sifman_stub:
	.word	0x41e00000
	.word	0
	.word	0x00000101
	.ascii	"sifman\0\0"
	import	sceSifInit, 5
	import	sceSifCheckInit, 29
	import_end

	.globl	stdio_stub
stdio_stub:
	.word	0x41e00000
	.word	0
	.word	0x00000103
	.ascii	"stdio\0\0\0"
	import	printf, 4
	import_end

	.globl	sysclib_stub
sysclib_stub:
	.word	0x41e00000
	.word	0
	.word	0x00000102
	.ascii	"sysclib\0"
	import	memcpy, 12
	import_end

	.globl	thbase_stub
thbase_stub:
	.word	0x41e00000
	.word	0
	.word	0x00000101
	.ascii	"thbase\0\0"
	import	CreateThread, 4
	import	StartThread, 6
	import	GetThreadId, 20
	import	SleepThread, 24
	import	iWakeupThread, 26
	import	GetSystemTime, 34
	import	USec2SysClock, 39
	import_end

	.globl	timrman_stub
timrman_stub:
	.word	0x41e00000
	.word	0
	.word	0x00000102
	.ascii	"timrman\0"
	import	AllocHardTimer, 4
	import	FreeHardTimer, 6
	import	SetTimerHandler, 20
	import	SetupHardTimer, 22
	import	StartHardTimer, 23
	import	StopHardTimer, 24
	import_end
