insmod ./iofs.ko 
mount -o loop,owner,group,users ../qemu-risc6/IO.img mnt
ls mnt
umount mnt
rmmod ./iofs.ko
dmesg | tail -n 30

