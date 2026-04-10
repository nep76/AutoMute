/*
	pathexpand.h
	
	realpath()もどき。
	getcwdを使うのでUSE_KERNEL_LIBC下では動かない。
*/

#ifndef PATHEXPAND_H
#define PATHEXPAND_H

#include <unistd.h>
#include <stdbool.h>
#include <ctype.h>
#include "utils/strutil.h"
#include "psp/memsce.h"

#define PE_PATH_MAX 255

#ifdef __cplusplus
extern "C" {
#endif

/*
	相対パスを絶対パスに変換する。
	変換する際に現在の作業ディレクトリを使用するが、
	もしスレッドに作業ディレクトリが存在しない場合は失敗する。
*/
bool pathexpandFromBase( const char* basepath, const char *path, char *resolved_path, size_t len );
bool pathexpandFromCwd( const char *path, char *resolved_path, size_t len );

#ifdef __cplusplus
}
#endif

#endif
