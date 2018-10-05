.text
.code16
.globl _start
.org 0

_disk_start:
	jmp _start
.org 3
	.ascii "XXXXXXXX"	/* manufacturer description */
bytes_per_block:
	.word 512		/* bytes per block */
	.byte 0x01		/* sectors per cluster (blocks per allocation unit) */
	.word 1			/* number of reserved blocks */
	.byte 2			/* number of file allocation tables */
dir_entries:
	.word 224		/* number of root directory entries */
total_sectors:
	.word 2880		/* total number of blocks on entire disk */
	.byte 0xf0		/* media descriptor: 0xf0 = 1.44M or HD */
sectors_per_fat:
	.word 9			/* number of blocks per file allocation table */
sectors_per_track:
	.word 18		/* number of blocks per track */
head_count:
	.word 2			/* number of heads */
hidden_sectors:
	.long 0			/* number of hidden blocks */
total_sectors_long:
	.long 0			/* total number of blocks in entire disk, if total_sectors exceeds 65535 */
	.byte 0			/* physical drive number */
	.byte 1			/* extended boot record siganture */
	.byte 0x29		/* EBPB exists */
	.long 0xdeadbeef	/* volume serial number */
	.ascii "NO NAME    "	/* volume label */
	.byte 0
	.ascii "FAT12  "	/* file system identifier */

_start:
/* print a message */
	movw $_hello_msg, %bx
	call _print_string

/* infinite loop */
1:
	hlt
	jmp 1b

_hello_msg:
	.asciz "Hello World!\r\n"

_print_string: /* print string */
	pusha
1:
	movb (%bx), %al
	incw %bx
	cmpb $0, %al
	je 2f
	movb $0x0E, %ah
	int $0x10
	jmp 1b
2: /* out */
	popa
	ret

/* signature AA55 */
	.space 510-(.-_disk_start)
	.word 0xAA55
