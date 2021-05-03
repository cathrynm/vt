	.export _ih
	.export _trip
	.export _jfsymbol
	.export _jfsymbol_memoryIndex
	.import SP_save
	.export _SHFLOK_save
	.export _LMARGN_save
	.segment "VBXE"
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
; start address is 1c00, this is below $2000 and mem bank is at $2000

	.export _writeVBXE
	.export _vbxeAddr
	.export _memacReg
	.export _vbxeData
	.export _vbxeReadAddr
	.export _readVBXE
_vbxeData:
	.byte 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0
	.byte 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0
	.byte 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0
	.byte 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0
	.byte 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0
	.byte 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0
	.byte 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0
	.byte 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0
_vbxeAddr	= vbxeLoop+4
_memacReg	= writememac+1
_vbxeReadAddr = readVbxeAddr + 1
_writeVBXE:
	jsr writememac
	ldy		#0
vbxeLoop:
	lda	_vbxeData,y
	sta $ffff,y
	iny
	dex
	bne vbxeLoop
	lda #0
	jsr writememac
	rts
writememac:
	sta $ffff
	rts
_readVBXE:
	jsr writememac
readVbxeAddr:
	lda $ffff
	tay
	lda #0
	jsr writememac
	tya
	rts
