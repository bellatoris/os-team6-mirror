INCLUDE=-I/home/dooige/Documents/Operating_System/os-team6/include
GCCROOT=/home/dooige/Downloads/opt/tizen-toolchain-4.9~git-i686_armv7l-tizen-linux-gnueabi-20151113/bin
CFLAGS=$(INCLUDE)
CC=$(GCCROOT)/armv7l-tizen-linux-gnueabi-gcc

all: rotd

rotd: rotd.c
	@echo [Arm-cc] $<...
	@$(CC) $(CFLAGS) $< -o $@

clean:
	@rm rotd
