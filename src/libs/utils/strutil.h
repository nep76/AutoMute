/*char *strutilToLowerFirst( char *str )

	便利な文字列処理用関数
*/

#ifndef __STRUTIL_H__
#define __STRUTIL_H__

#include <ctype.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------
	関数プロトタイプ
-----------------------------------------------*/

/*
	strutilSafeCopy
	
	標準ライブラリのstrncpyに似ているが、必ず末尾にNULLを付加する。
	
	@param: char *dest
		文字列のコピー先。
		このポインタに元々書き込まれていた文字列は、srcで上書きされ失われる。
	
	@param: const char *src
		文字列のコピー元。
		この文字列がdestへコピーされる。
	
	@param: size_t max
		コピーする最大文字数。
		NULL文字を含む。
		1では常にNULLのみを、0以下ではなにもしない。
	
	@return: char*
		destを返す
*/
char *strutilSafeCopy( char *dest, const char *src, size_t max );

/*
	strutilSafeCat
	
	標準ライブラリのstrncatに似ているが、必ず末尾にNULLを付加する。
	また、サイズはsrcの文字数ではなく、結合後の最大文字数。
	つまり、destは必ずmax以下になる。
	dest + srcの文字列がmaxよりも大きい場合は、srcは全てコピーされない。
	
	@param: char *dest
		元文字列。この後ろにsrcが結合される。
	
	@param: const char *src
		結合する文字列。destの後にこの文字列が結合される。
	
	@param: size_t max
		結合後の最大文字数。
		destは必ずこの長さ以下になる。
	
	@return: char*
		destを返す。
*/
char *strutilSafeCat( char *dest, const char *src, size_t max );

/*
	strutilRemoveChar 
	
	文字列から、指定した文字を削除し、さらに詰める。
	
	@param: char *str
		処理対象の文字列。
	
	@param: const char *target
		取り除く文字を指定。
		文字列で指定するが、取り除きたい文字を全て書き出す。
*/
void strutilRemoveChar( char *str, const char *target );

/*
	strutilToUpper
	
	文字列に含まれるアルファベットをすべて大文字に変換する。
	
	@param: char *str
		変換対象文字列。
	
	@return: char*
		strを返す。
*/
char *strutilToUpper( char *str );

/*
	strutilToLower
	
	文字列に含まれるアルファベットをすべて小文字に変換する。
	
	@param: char *str
		変換対象文字列。
	
	@return: char*
		strを返す。
*/
char *strutilToLower( char *str );

/*
	strutilToUpperFirst
	
	文字列の先頭を大文字に変換する。
	
	@param: char *str
		変換対象文字列。
	
	@return: char*
		strを返す。
*/
char *strutilToUpperFirst( char *str );

/*
	strutilToLowerFirst
	
	文字列の先頭を小文字に変換する。
	
	@param: char *str
		変換対象文字列。
	
	@return: char*
		strを返す。
*/
char *strutilToLowerFirst( char *str );

#ifdef __cplusplus
}
#endif

#endif
