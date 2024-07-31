# This file is free software; C 2024 Rainer Müller

# PORT+0:	D7		D6		D5		D4		D3		D2		D1		D0
#	(WR)	ENABLE	RTS2-	RTS1-	RTS0-	RESET-	SPICS-	SPICLK	SPISO

# PORT+1:	BUSY-	ACK-	PE		SLCT	ERROR-	 X		 X		 X
#	(RD)	SPISI	INT-	RXBF0-	RXBF1-
#			inv.

# PORT+2:	---		---		---		IRQENA	SLCT-	INIT-	AUTOFD-	STROBE-
#	(R/W)   ---		---		---		???		---		---		---		---
#											inv.			inv.	inv.

	.lcomm	portadr,2

	.section .note.GNU-stack,"",@progbits

	.text

# unsigned char spiinit(short port);
	.globl	spiinit
	.type	spiinit, @function

spiinit:
	mov		%di, %dx			# dx = portadr
	mov		%dx, portadr(%rip)

	mov		$10, %ecx
reswait:
	mov		$244, %al			# HW reset 2515
	outb	%al, %dx
	loop	reswait
	mov		$252, %al			# end of reset
	outb	%al, %dx

	add		$2, %dx
	mov		$0, %al				# init control port
	outb	%al, %dx
		
# unsigned char spistat(void);
	.globl	spistat
	.type	spistat, @function

spistat:
	mov		portadr(%rip), %dx	# dx = portadr
	inc		%dx		
	inb		%dx, %al			# return status port
	movzx	%al, %eax
	ret


# void spitransfer(char *txbuf, char *rxbuf, int len);
	.globl	spitransfer
	.type	spitransfer, @function

spitransfer:
	xchg	%rdi, %rsi			# rsi = txbuf, rdi = rxbuf
	mov		%edx, %ecx			# cx = len
	mov		portadr(%rip), %dx	# dx = portadr		

	mov		$248, %al			# start of SPICS
	outb	%al, %dx
allbytes:
	lodsb	(%rsi)
	mov		%al, %ah
	mov		%rcx, %r8
	mov		$8, %ecx

allbits:
	xor		%al, %al
	rcl		$1, %ah
	adc		$248, %al
	outb	%al, %dx			# CLK=0, DAT=D

	or		$2, %al
	outb	%al, %dx			# CLK=1, DAT=D

	shr		$1,	%ah
	inc		%dx		
	inb		%dx, %al			# read SI
	dec		%dx		
	shl		$1, %ax				# save SI bit
	
	loop	allbits

	mov		%r8, %rcx		
	or		%rdi, %rdi
	jz		endloop
	mov		%ah, %al
	not		%al					# SI pin inverted
	stosb	(%rdi)
endloop:
	loop	allbytes

	mov		$252, %al			# end of SPICS
	outb	%al, %dx
	ret
