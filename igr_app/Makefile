
EE_BIN = igr.elf

EE_OBJS_DIR = obj/
EE_SRC_DIR = src/
EE_ASM_DIR = asm/

EE_LIBS = -ldebug -lpatches -lpad

EE_OBJS = main.o sd2psxman_rpc.o
ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

# Add embedded IRX files
EE_IRX_FILES =\
	mcman.irx \
	sio2man.irx\
	sd2psxman.irx\
	padman.irx

EE_IRX_OBJS = $(addsuffix _irx.o, $(basename $(EE_IRX_FILES)))
EE_OBJS += $(EE_IRX_OBJS)

# Where to find the IRX files
vpath %.irx $(PS2SDK)/iop/irx/
vpath %.irx $(ROOT_DIR)/../sd2psxman/irx/

# Rule to generate them
%_irx.o: %.irx
	bin2c $< $*_irx.c $*_irx
	mips64r5900el-ps2-elf-gcc -c $*_irx.c -o $*_irx.o

all: $(EE_BIN)

clean:
	rm -f -r $(EE_OBJS) $(EE_BIN) *_irx.c

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
