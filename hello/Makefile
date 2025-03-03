#!/usr/bin/make -f

MODULE_NAME := hello
MODULE_VERSION := 0.0.1

KVER := $(shell uname -r)
LIB_MODULES_PATH := /lib/modules
KERNEL_MODLIB := $(LIB_MODULES_PATH)/$(KVER)
MODDIR := $(KERNEL_MODLIB)/extra
INCLUDEDIR := /usr/local/include
KDIR := $(KERNEL_MODLIB)/build
PWD := $(shell pwd)

obj-m += hello.o

src ?= $(PWD)
inc ?= $(src)/../include
ccflags-y := -I$(inc)

all: module

module: hello.ko

hello.ko: hello.c
	$(MAKE) -C "$(KDIR)" M="$(src)" modules

hello.mod.c: hello.ko
hello.mod.o: hello.mod.c

clean:
	rm -rf $(obj-m) hello.ko hello.mod.c hello.mod.o modules.order Module.symvers .*.cmd hello.mod

install: module
	install -d "$(MODDIR)"
	install -m 644 hello.ko "$(MODDIR)"
	install -d "$(INCLUDEDIR)/linux"
	install -m 644 $(inc)/linux/hello.h "$(INCLUDEDIR)/linux/"
	depmod "$(KVER)"

uninstall:
	rm -f "$(MODDIR)/hello.ko"
	rmdir --ignore-fail-on-non-empty "$(MODDIR)"
	rm "$(INCLUDEDIR)/linux/hello.h"
	rmdir --ignore-fail-on-non-empty "$(INCLUDEDIR)/linux"
	depmod "$(KVER)"

load: module
	insmod hello.ko

unload:
	! grep -q "^hello " /proc/modules || rmmod hello

test: module unload
	insmod hello.ko
	echo -n "test1" > /dev/hello
	[ "$$(cat /dev/hello)" = "test1" ]
	echo -n "test2" > /dev/hello
	[ "$$(cat /dev/hello)" = "test2" ]
	echo -n "test3" >> /dev/hello
	[ "$$(cat /dev/hello)" = "test2test3" ]
	rmmod hello

.PHONY: module clean install uninstall load unload install-dkms uninstall-dkms test
