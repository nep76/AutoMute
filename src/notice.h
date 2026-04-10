/*
	notice.h
*/

#include "global.h"
#include <pspdisplay.h>
#include "psp/blit.h"
#include "utils/strutil.h"

/*-----------------------------------------------
	定数マクロ
-----------------------------------------------*/
#define NOTICE_MESSAGE_BUFFER      255

/*-----------------------------------------------
	構造体/列挙型
-----------------------------------------------*/
typedef enum {
	NS_INIT = 0,
	NS_ERROR,
	NS_READY,
	NS_WORK
} NoticeStat;

/*-----------------------------------------------
	関数プロトタイプ
-----------------------------------------------*/
int noticeThread( SceSize arglen, void *argp );
void noticeThreadExit( void );
NoticeStat noticeGetStat( void );
void noticePrint( char *str );
void noticePrintAbort( void );
