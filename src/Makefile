TARGET = automute
PSP_FW_VERSION = 371

# PRXを作成する
BUILD_PRX = 1

# エクスポート定義ファイル
PRX_EXPORTS = exports.exp

# 機能が制限されているが軽量なカーネルlibc(sysclib)。
# でも機能制限されすぎなので使わない。
# 無指定だとnewlibを使おうとしてくれる模様。
#USE_KERNEL_LIBC = 1

# カーネルモードPRXを作る場合は推奨されているようだ。
# これを指定するといくつかのライブラリは
# 優先的にカーネルモードの方(psp***_driver)をリンクしようとするみたい。
USE_KERNEL_LIBS = 1

CLASSG_LIBS = libs

INCDIR = $(CLASSG_LIBS)
CFLAGS = -g -Os -G0 -Wall -fno-strict-aliasing -fno-builtin-printf
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR =
LDFLAGS =

# newlibを使う場合、libpspuserをリンクしないとsceKernelFreeMemSizeのリンクに失敗する。
# libpspuserより先にlibpspkernelをリンクしないと、システムコントロール系のシンボルに
# ユーザモードの方をリンクしてしまい、カーネルモードスレッドを生成できない模様？
LIBS = -lpsphprm_driver -lpspkernel -lc -lpspuser

OBJS = exports.o \
       main.o \
       notice.o \
       $(CLASSG_LIBS)/utils/strutil.o \
       $(CLASSG_LIBS)/psp/memsce.o \
       $(CLASSG_LIBS)/psp/blit.o \
       $(CLASSG_LIBS)/psp/pathexpand.o \
       $(CLASSG_LIBS)/psp/fileh.o \
       $(CLASSG_LIBS)/utils/confmgr.o \
       $(CLASSG_LIBS)/psp/confmgr/cmpspbtn.o \

PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build_prx.mak
