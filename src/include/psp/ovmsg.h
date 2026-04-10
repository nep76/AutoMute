/*=========================================================
	
	ovmsg.h
	
	オーバーレイメッセージ。
	
=========================================================*/
#ifndef OVMSG_H
#define OVMSG_H

#include <pspkernel.h>
#include <stdarg.h>
#include <stdbool.h>

/*=========================================================
	マクロ
=========================================================*/
#define OVMSG_LENGTH 128

#ifdef __cplusplus
extern "C" {
#endif

/*=========================================================
	関数
=========================================================*/
int ovmsgInit( void );
int ovmsgThreadMain( SceSize arglen, void *argp );
void ovmsgPrintIntrStart( void );
bool ovmsgPrintf( const char *format, ... );
bool ovmsgVprintf( const char *format, va_list ap );
void ovmsgWaitForInit( void );
bool ovmsgIsRunning( void );
void ovmsgWaitForReady( void );
void ovmsgShutdownStart( void );

#ifdef __cplusplus
}
#endif

#endif
