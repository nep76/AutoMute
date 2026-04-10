/*=========================================================

	xprintf.c

	pspsyslibc用printf系実装。

=========================================================*/
#include <pspkernel.h>
#include <pspsysclib.h>
#include <stdio.h>
#include <stdarg.h>

#define XPRINTF_PRNT_SIG_START_OF_STRING 0x200
#define XPRINTF_PRNT_SIG_END_OF_STRING   0x201

/*=========================================================
	型宣言
=========================================================*/
struct xprintf_ctx {
	char   *buf;
	const size_t *len;
	size_t cpylen;
};

/*=========================================================
	ローカル関数
=========================================================*/
static void sprnt_cb( void *ctx, int ch )
{
	if( ch == XPRINTF_PRNT_SIG_START_OF_STRING || ch == XPRINTF_PRNT_SIG_END_OF_STRING ) return;
	(((struct xprintf_ctx *)ctx)->buf)[((struct xprintf_ctx *)ctx)->cpylen++] = ch;
}

static void snprnt_cb( void *ctx, int ch )
{
	if( ch == XPRINTF_PRNT_SIG_START_OF_STRING || ch == XPRINTF_PRNT_SIG_END_OF_STRING ) return;
		
	if( (((struct xprintf_ctx *)ctx)->cpylen) < *(((struct xprintf_ctx *)ctx)->len) )
		(((struct xprintf_ctx *)ctx)->buf)[((struct xprintf_ctx *)ctx)->cpylen] = ch;
	
	(((struct xprintf_ctx *)ctx)->cpylen)++;
}

/*=========================================================
	関数
=========================================================*/
int vsprintf( char *buf, const char *fmt, va_list ap )
{
	struct xprintf_ctx ctx = { buf, NULL, 0 };
	
	prnt( sprnt_cb, (void *)&ctx, fmt, ap );
	ctx.buf[ctx.cpylen] = '\0';
	
	return ctx.cpylen;
}

int sprintf( char *buf, const char *fmt, ... )
{
	va_list ap;
	int ret;
	
	va_start( ap, fmt );
	ret = vsprintf( buf, fmt, ap );
	va_end( ap );
	
	return ret;
}

int vsnprintf( char *buf, size_t n, const char *fmt, va_list ap )
{
	struct xprintf_ctx ctx = { buf, (const size_t *)&n, 0 };
	
	prnt( snprnt_cb, (void *)&ctx, fmt, ap );
	
	if( ctx.cpylen >= *(ctx.len) ){
		ctx.buf[*(ctx.len) - 1] = '\0';
	} else{
		ctx.buf[ctx.cpylen] = '\0';
	}
	
	return ctx.cpylen;
}

int snprintf( char *buf, size_t n, const char *fmt, ... )
{
	va_list ap;
	int ret;
	
	va_start( ap, fmt );
	ret = vsnprintf( buf, n, fmt, ap );
	va_end( ap );
	
	return ret;
}
