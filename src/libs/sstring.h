/*
	sstring.h
*/

#ifndef __SSTRING_H__
#define __SSTRING_H__

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
	origの内容をn - 1バイトだけtargにコピーし、末尾にNULLを書き込む。
	origの内容がnに満たない場合も常に文字列終端にNULLが付加される。
*/
char *safe_strncpy ( char *targ, const char *orig, size_t n );

/*
	n - 1からtargの長さを減算して、
	残ったバイト数分のorigの内容をtargの後ろに結合、末尾にNULLを書き込む。
*/
char *safe_strncat ( char *targ, const char *orig, size_t n );

#ifdef __cplusplus
}
#endif

#endif
