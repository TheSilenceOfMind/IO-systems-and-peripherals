SRC = src

obj-m = ch_drv.o
ch_drv-objs = $(SRC)/ch_drv.o $(SRC)/kfs.o $(SRC)/util.o

PWD = $(shell pwd)
UNAME = $(shell uname -r)

all:
	make -C /lib/modules/$(UNAME)/build M="$(PWD)" modules

clean:
	make -C /lib/modules/$(UNAME)/build M="$(PWD)" clean
