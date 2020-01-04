obj-m := iofs.o
iofs-objs := kiofs.o super.o inode.o dir.o file.o
CFLAGS_kiofs.o := -DDEBUG
CFLAGS_super.o := -DDEBUG
CFLAGS_inode.o := -DDEBUG
CFLAGS_dir.o := -DDEBUG
CFLAGS_file.o := -DDEBUG

all: ko mkfs-iofs

ko:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

mkfs-iofs_SOURCES:
	mkfs-iofs.c iofs.h

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm mkfs-iofs
