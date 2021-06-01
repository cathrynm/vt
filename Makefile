all : vtv.xex vtx.xex

vtv.xex:
	make -f MakeV

vtx.xex :  
	make -f MakeX

