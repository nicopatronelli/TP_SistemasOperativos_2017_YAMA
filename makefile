env:
	make lib

rmenv:
	make rmlib

lib:
	cd SHARED/ && make install

rmlib:
	cd SHARED/ && make uninstall

all:
	cd FILE_SYSTEM/ && make all
	cd YAMA/ && make all
	cd MASTER/ && make all
	cd DATANODE/ && make all
	cd WORKER/ && make all

clean:
	cd FILE_SYSTEM/ && make clean
	cd YAMA/ && make clean
	cd WORKER/ && make clean
	cd DATANODE/ && make clean
	cd MASTER/ && make clean
