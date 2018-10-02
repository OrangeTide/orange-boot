.text
.code16
.globl _start
.org 0
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
	.space 510-(.-_start)
	.word 0xAA55
