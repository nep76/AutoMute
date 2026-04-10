/*
	fbmgr.c
*/

#include "fbmgr.h"
	
/*-----------------------------------------------
	ローカル関数
-----------------------------------------------*/

/*-----------------------------------------------
	ローカル変数
-----------------------------------------------*/
static FbmgrDisplayStat st_save_stat;
static FbmgrDisplayStat st_stat;
static size_t           st_buflen;
static void             *st_disp_buf;
static void             *st_draw_buf;

/*=============================================*/

void fbmgrInit( void )
{
	/* 現在のディスプレイモードと表示バッファを取得 */
	fbmgrGetDisplayStat( &st_save_stat );
	
	/* これを複製し作業用を作成 */
	st_stat   = st_save_stat;
	
	/* ディスプレイモードからフレームバッファの容量を計算 */
	st_buflen = fbmgrGetPixelLength( st_stat.pixelFormat ) * st_stat.bufferWidth * st_stat.height;
}

void *fbmgrSetBuffers( void *disp_buf, void *draw_buf )
{
	st_disp_buf = disp_buf;
	st_draw_buf = draw_buf;
	
	return st_draw_buf;
}

size_t fbmgrFrameBufSize( void )
{
	return st_buflen;
}

void fbmgrDestroy( void )
{
	/* フレームバッファのアドレスを初期化前の位置に戻す */
	sceDisplaySetFrameBuf( st_save_stat.frameBuffer, st_save_stat.bufferWidth, st_save_stat.pixelFormat, PSP_DISPLAY_SETBUF_IMMEDIATE );
}

void *fbmgrGetCurrentDispBuf( void )
{
	return st_disp_buf;
}

void *fbmgrGetCurrentDrawBuf( void )
{
	return st_draw_buf;
}

void *fbmgrSwapBuffers( void )
{
	void *swap  = st_disp_buf;
	st_disp_buf = st_draw_buf;
	st_draw_buf = swap;
	
	/* 新しい表示バッファを表示 */
	sceDisplaySetFrameBuf( st_disp_buf, st_stat.bufferWidth, st_stat.pixelFormat, PSP_DISPLAY_SETBUF_IMMEDIATE );
	
	return st_draw_buf;
}

void fbmgrBlitBuffers( void )
{
	memcpy( st_disp_buf, st_draw_buf, st_buflen );
}

int fbmgrGetDisplayStat( FbmgrDisplayStat *stat )
{
	int rval = 0;
	
	rval = sceDisplayGetMode( &(stat->mode), &(stat->width), &(stat->height) );
	if( rval ) return rval;
	
	rval = sceDisplayGetFrameBuf( (void *)&(stat->frameBuffer), &(stat->bufferWidth), (int *)&(stat->pixelFormat), PSP_DISPLAY_SETBUF_IMMEDIATE );
	if( rval ) return rval;
	
	return 0;
}

size_t fbmgrGetPixelLength( enum PspDisplayPixelFormats pxfmt )
{
	switch( pxfmt ){
		case PSP_DISPLAY_PIXEL_FORMAT_565:
		case PSP_DISPLAY_PIXEL_FORMAT_5551:
		case PSP_DISPLAY_PIXEL_FORMAT_4444:
			return 2;
		case PSP_DISPLAY_PIXEL_FORMAT_8888:
		default:
			return 4;
	}
}
