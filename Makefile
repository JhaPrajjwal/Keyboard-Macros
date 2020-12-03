obj-m += kmodule.o

all:
	make -C /lib/modules/`uname -r`/build M=$(PWD) modules
	gcc write_proc.c -o write_proc
	gcc read_macros.c -o read_macros
clean:
	make -C /lib/modules/`uname -r`/build M=$(PWD) clean
	rm write_proc read_macros
