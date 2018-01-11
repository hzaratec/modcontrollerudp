
.PHONY: all exe clean

all: exe

exe:
	make -C controller all
	make -C sha all
	
clean:
	make -C controller clean
	make -C sha clean



