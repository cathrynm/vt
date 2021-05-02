	.export _ih
	.export _trip
	.export _jfsymbol
	.export _jfsymbol_memoryIndex
	.import SP_save
	.export _SHFLOK_save
	.export _LMARGN_save
_SHFLOK_save = SP_save+1
_LMARGN_save = SP_save+2
_ih:	LDA #$01
	STA _trip
	PLA
	RTI
_trip: .byte 0
_jfsymbol_memoryIndex: .byte 0
	.byte 0
	.byte 0

_jfsymbol:
	ldy $701
	cpy #$44
	bcc jferror
	jsr	$07eb
	beq	jferror
	sty _jfsymbol_memoryIndex
	rts
jferror:
	lda #0
	tax
	rts
