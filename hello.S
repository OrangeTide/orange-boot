.text
.code16
.globl _start
.org 0

_disk_start:
	jmp _start
.org 3
	.ascii "XXXXXXXX"	/* manufacturer description */
	.word 512		/* bytes per block */
	.byte 0x00		/* blocks per allocation unit */
	.word 1			/* number of reserved blocks */
	.byte 2			/* number of file allocation tables */
	.word 2			/* number of root directory entries */
	.word 2880		/* total number of blocks on entire disk */
	.byte 0xf0		/* media descriptor: 0xf0 = 1.44M or HD */
	.word 0			/* number of blocks per file allocation table */
	.word 0			/* number of blocks per track */
	.word 2			/* number of heads */
	.long 0			/* number of hidden blocks */
	.long 2880		/* total number of blocks in entire disk */
	.word 0			/* physical drive number */
	.byte 0			/* extended boot record siganture */
	.long 0xdeadbeef	/* volume serial number */
	.ascii "NO NAME    "	/* volume label */
	.ascii "FAT12   "	/* file system identifier */

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
