obj-m = ch_drv.o
PWD = $(shell pwd)
UNAME = $(shell uname -r)

all:
	make -C /lib/modules/$(UNAME)/build M="$(PWD)" modules 

clean:
	make -C /lib/modules/$(UNAME)/build M="$(PWD)" clean
