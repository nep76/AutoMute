# PSPLINK用にデバッグメッセージをprintf出力する:
#     make WITH_PSPLINK_DEBUG=1
#

TARGET = automute
PSP_FW_VERSION = 380

# PRXを作成する
BUILD_PRX = 1

# エクスポート関数定義ファイル
PRX_EXPORTS = exports.exp

# 機能が制限されているが軽量なカーネルlibc(sysclib)。
# printf系関数もないが、prntという関数を提供してくれるため、
# 自分で再実装するのは容易。
USE_KERNEL_LIBC = 1

# カーネルモードPRXを作る場合は推奨されているようだ。
# これを指定するといくつかのライブラリは
# 優先的にカーネルモードのライブラリ(psp***_driver)をリンクしようとするみたい。
USE_KERNEL_LIBS = 1

DIR_CLASSG = include

INCDIR   = $(DIR_CLASSG)
CFLAGS   = -O2 -G 0 -Wall -fno-strict-aliasing
ASFLAGS  = $(CFLAGS)
LIBDIR   = 
LDFLAGS  = 
LIBS     = -lpsppower_driver -lpsphprm_driver -lm

OBJS = exports.o \
       sceSystemMemoryManager.o \
       sceImpose.o \
       ammain.o \
       $(DIR_CLASSG)/psp/padutil.o \
       $(DIR_CLASSG)/psp/memory.o \
       $(DIR_CLASSG)/psp/heap.o \
       $(DIR_CLASSG)/psp/dmem.o \
       $(DIR_CLASSG)/psp/ovmsg.o \
       $(DIR_CLASSG)/psp/fiomgr.o \
       $(DIR_CLASSG)/psp/gb.o \
       $(DIR_CLASSG)/psp/sysclib/sysclib.o \
       $(DIR_CLASSG)/psp/utils/inimgr.o \
       $(DIR_CLASSG)/utils/strutil.o \

ifdef WITH_PSPLINK_DEBUG
	CFLAGS += -fno-builtin-printf -g -DMEMORY_DEBUG
endif

ifeq ($(USE_KERNEL_LIBC),1)
    CFLAGS += -DPSP_USE_KERNEL_LIBC
	OBJS += sceSystemMemoryManager.o \
            $(DIR_CLASSG)/psp/sysclib/sysclib.o
else
	LIBS += -lpspkernel -lc -lpspuser
endif

PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build_prx.mak
