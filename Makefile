all : vtv.xex vtx.xex vtd.xex vtc.xex vtr.xex

clean: 
	make clean -f MakeV
	make clean -f MakeX
	make clean -f MakeD
	make clean -f MakeC
	make clean -f MakeR

vtv.xex:
	make -f MakeV

vtx.xex :  
	make -f MakeX

vtd.xex:
	make -f MakeD

vtc.xex:
	make -f MakeC

vtr.xex:
	make -f MakeR

