/*
	Configration Manager
	
	設定項目ラベルについて。
		ファイルから読み込んだラベルについては空白文字の削除と全て大文字かによる
		ノーマライズを行うが、ConfmgrHandlerParamsのlabelメンバにはなにもしない。
		labelメンバの値を間違って書かないように注意。
*/

#ifndef __CONFMGR_H__
#define __CONFMGR_H__

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "utils/strutil.h"
#include "psp/fileh.h"

/*-----------------------------------------------
	定数
-----------------------------------------------*/
#define CONFMGR_LABEL_BUFFER 64
#define CONFMGR_LINE_BUFFER  256

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------
	型宣言
-----------------------------------------------*/
typedef void ( *ConfmgrStoreCallback )( char buf[], size_t max, void *arg );
typedef void ( *ConfmgrLoadCallback  )( char *name, char *value, void *arg );

typedef enum {
	CH_LONGINT,
	CH_CHAR,
	CH_CALLBACK_STORE,
	CH_CALLBACK_LOAD,
} ConfmgrHandleType;

typedef struct {
	char *label;
	ConfmgrHandleType type;
	void *handler;
	void *args;
} ConfmgrHandlerParams;

/*-----------------------------------------------
	関数プロトタイプ
-----------------------------------------------*/
int confmgrStore( ConfmgrHandlerParams *params, int count, const char *fullpath );
int confmgrLoad( ConfmgrHandlerParams *params, int count, const char *fullpath );

#ifdef __cplusplus
}
#endif

#endif
