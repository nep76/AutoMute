#======================================
# AutoMuteをビルドするためのMakefile.
# 
#   使用可能なmakeターゲット:
#
#   all(default) - AutoMute本体を作成する
#   clean        - AutoMute本体とオブジェクトファイルを削除
#   automute    - allの別名
#
#   libs         - AutoMuteをビルドするのに必要なライブラリを作成する
#   libsclean    - ライブラリ本体とオブジェクトファイルを削除
#
#   everything   - libs + automute
#   allclean     - clean + libsclean
#
#--------------------------------------

TARGET = automute

# PRXを作成する
BUILD_PRX = 1

# エクスポート関数定義ファイル
PRX_EXPORTS = exports.exp

# 機能が制限されているが軽量なカーネルlibc(sysclib)。
USE_KERNEL_LIBC = 1

# カーネルモードPRXを作る場合は推奨されているようだ。
# これを指定するといくつかのライブラリは
# 優先的にカーネルモードのライブラリ(psp***_driver)をリンクしようとするみたい。
USE_KERNEL_LIBS = 1

BASEPATH = .

INCDIR  = $(BASEPATH)/include
LIBDIR  = $(BASEPATH)/lib

CFLAGS      = -Os -G0 -Wall -fno-strict-aliasing
ASFLAGS     = $(CFLAGS)
LDFLAGS     = 
LIBS        = -lpsppower_driver -lpsphprm_driver -lm
CLASSG_LIBS = -lovmsg -linimgr -lfiomgr -lpadutil -lpb -ldmem -lheap -lmemory -lstrutil -lhook

OBJS = \
	exports.o \
	sceImpose_driver.o \
	sceImpose_driver401.o \
	ammain.o 

LIBS := $(CLASSG_LIBS) $(LIBS)

ifeq ($(USE_KERNEL_LIBC),1)
    CFLAGS += -DPSP_USE_KERNEL_LIBC
	OBJS += sceSystemMemoryManager.o
	LIBS += -lsysclib
else
	LIBS += -lpspkernel -lc -lpspuser
endif

PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build_prx.mak

everything: libs $(TARGET)

allclean: libsclean clean

$(TARGET): all

libs:
	@$(MAKE) -C $(BASEPATH)/include

libsclean:
	@$(MAKE) -C $(BASEPATH)/include clean
