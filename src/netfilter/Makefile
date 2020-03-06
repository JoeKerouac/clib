CONFIG_MODULE_SIG=n
obj-m+=nf_kernel_custom_hook.o
all:
	make  -C /lib/modules/$(shell uname -r)/build/ M=${PWD} modules
clean:
	make  -C /lib/modules/$(shell uname -r)/build/ M=${PWD} clean
