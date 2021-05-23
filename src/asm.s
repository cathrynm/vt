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
	.export _ASMEND
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
.export _readBuffer
_readBuffer: 
; needs to be below 0x4000
; 256 bytes.  How do  you allocate .bs in ca65?
	.byte 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0
	.byte 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0
	.byte 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0
	.byte 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0
	.byte 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0

	.byte 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0
	.byte 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0
	.byte 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0
	.byte 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0
	.byte 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0


	.byte 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0
	.byte 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0
	.byte 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0
.export _copyBankToBufferFrom
.export _copyBankToBufferTo
.export _copyBankToBuffer
_copyBankToBufferFrom = copyBankToBufferFrom + 1
_copyBankToBufferTo = copyBankToBufferTo + 1
_copyBankToBuffer:
 	stx    $d301
 	tax
 	ldy #0
copyBankToBufferFrom:
	lda    $ffff,y
copyBankToBufferTo:
    sta    $ffff,y
    iny
    dex
    bne copyBankToBufferFrom
    lda #$ff
    sta $d301
	rts
.export _copyReadBufferToBankTo
.export _copyReadBufferToBank
_copyReadBufferToBankTo = copyReadBufferToBankTo + 1
_copyReadBufferToBank:
 	stx    $d301
 	tax
 	ldy #0
copyReadBufferToBankLoop:
	lda    _readBuffer,y
copyReadBufferToBankTo:
    sta    $ffff,y
    iny
    dex
    bne copyReadBufferToBankLoop
    lda #$ff
    sta $d301
	rts
_ASMEND: