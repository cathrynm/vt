	.export _ih
	.export _trip
_ih:	LDA #$01
	STA _trip
	PLA
	RTI
_trip: .byte 0
