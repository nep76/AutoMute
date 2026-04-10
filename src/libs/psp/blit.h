/*
	PSPのVRAM上に図形を転送する。
*/

#ifndef SCR_WIDTH
#define SCR_WIDTH  480
#endif
#ifndef SCR_HEIGHT
#define SCR_HEIGHT 272
#endif
#ifndef BUF_WIDTH
#define BUF_WIDTH  512
#endif

#ifndef BLIT_H
#define BLIT_H

#include <pspkernel.h>
#include <pspdisplay.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "psp/graphics/fbmgr.h"

/* 透明色 */
#define BLIT_TRANSPARENT 0

/* アルファブレンド */
#define BLIT_ALPHA_BLEND( s, d, a, b ) ( ( ( ( s ) + 1 - ( d ) ) * ( a ) >> ( b ) ) + ( d ) )

/* 字間の幅 */
#define BLIT_LETTER_SPACE 1

/* この値を変えても文字が大きくなったり小さくなったりはしません */
#define BLIT_CHAR_WIDTH  ( 5 + BLIT_LETTER_SPACE )
#define BLIT_CHAR_HEIGHT 8

/* 関数にみえるマクロ */
#define blitOffsetChar( x ) ( ( x ) * BLIT_CHAR_WIDTH  )
#define blitOffsetLine( x ) ( ( x ) * BLIT_CHAR_HEIGHT )
#define blitMeasureChar( x ) ( ( x ) * BLIT_CHAR_WIDTH )
#define blitMeasureLine( x ) ( ( x ) * BLIT_CHAR_HEIGHT )
#define blitLineRel( x, y, e, f, c ) blitLine( x, y, ( x ) + ( e ), ( y ) + ( f ), c )
#define blitFillBox( x, y, w, h, c ) blitFillRect( x, y, ( x ) + ( w ), ( y ) + ( h ), c )
#define blitLineBox( x, y, w, h, c ) blitLineRect( x, y, ( x ) + ( w ), ( y ) + ( h ), c )

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t ( *BlitAlphaBlender )( void *, void * );

typedef enum {
	BO_AUTOFLUSH  = 0x00000001,
	BO_ALPHABLEND = 0x00000002
} BlitOption;

typedef enum {
	B8_PSPSDK,
	B8_BUTTON_SYMBOL,
} Blit8BitCharTable;

typedef struct {
	u8 *table;
	int charnum;
} BlitFontTable;

int  blitInit( void );
void blitSetOptions( unsigned int opt );
void blitGetFrameBufStat( FbmgrDisplayStat *stat );
void blitSetFrameBufAddrTop( void *addr );
int  blitGetFrameBufPixelLength( void );
uint32_t blitAlphaBlending8888( void *fg, void *bg );
uint32_t blitAlphaBlending4444( void *fg, void *bg );
uint32_t blitAlphaBlending5551( void *fg, void *bg );
uint32_t blitAlphaBlending565( void *fg, void *bg );

/* blitColorConvert565
	PSP_DISPLAY_PIXEL_FORAMT_8888形式の色を
	PSP_DISPLAY_PIXEL_FORAMT_565形式に変換する。
	
	@param u32 color
		変換したいPSP_DISPLAY_PIXEL_FORAMT_8888形式の色。
	
	@return u16
		変換された色。
*/
u16 blitColorConvert565( u32 color );

/* blitColorConvert5551
	PSP_DISPLAY_PIXEL_FORAMT_8888形式の色を
	PSP_DISPLAY_PIXEL_FORAMT_5551形式に変換する。
	
	@param u32 color
		変換したいPSP_DISPLAY_PIXEL_FORAMT_8888形式の色。
	
	@return u16
		変換された色。
*/
u16 blitColorConvert5551( u32 color );

/* blitColorConvert4444
	PSP_DISPLAY_PIXEL_FORAMT_8888形式の色を
	PSP_DISPLAY_PIXEL_FORAMT_4444形式に変換する。
	
	@param u32 color
		変換したいPSP_DISPLAY_PIXEL_FORAMT_8888形式の色。
	
	@return u16
		変換された色。
*/
u16 blitColorConvert4444( u32 color );

/* blitConvertColorFrom8888
	PSP_DISPLAY_PIXEL_FORAMT_8888形式の色を
	指定したピクセルフォーマットの形式に変換する。
	
	@param u32 color
		変換したいPSP_DISPLAY_PIXEL_FORAMT_8888形式の色。
	
	@return u32
		変換された色。
*/
u32 blitConvertColorFrom8888( enum PspDisplayPixelFormats pxfmt, u32 color );

/* blitGetPixelLength
	渡したピクセルフォーマットのデータのバイト数を返す。
	
	@param enum PspDisplayPixelFormats pxfmt
		長さを調べたいピクセルフォーマット。
	
	@return int
		バイト数。
*/
int blitGetPixelLength( enum PspDisplayPixelFormats pxfmt );

/* blitChar
	任意の位置に文字を描画する。
	
	@param unsigned int sx
		描画する位置。
		座標ではなく文字数なのに注意。
		実際の座標では、x = ( sx * BLIT_CHAR_WIDTH ) となる。
	
	@param unsigned int sy
		描画する行。
		座標ではなく行なのに注意。
		実際の座標では、y = ( sy * BLIT_CHAR_HEIGHT ) となる。
	
	@param const char chr
		描画する文字。
	
	@param u32 fgcolor
		文字の色。
	
	@param u32 bgcolor
		文字の背景の色。
	
	@return int
		描画した文字数。
*/
int blitChar( unsigned int sx, unsigned int sy, u32 fgcolor, u32 bgcolor, const char chr );

/* blitString
	任意の位置に文字列を描画する。
	
	@param unsigned int sx
		描画を開始するX座標。
	
	@param unsigned int sy
		描画を開始するY座標。
	
	@param u32 fgcolor
		文字の色。
	
	@param u32 bgcolor
		文字の背景の色。
	
	@param const char *msg
		描画する文字列。
	
	@return int
		描画した文字数。
*/
int blitString( unsigned int sx, unsigned int sy, u32 fgcolor, u32 bgcolor, const char *msg );

/* blitFillRect
	指定した範囲を塗りつぶす。
	
	@param unsigned int sx
		塗りつぶしを開始するX座標。
	
	@param unsigned int sy
		塗りつぶしを開始するY座標。
	
	@param unsigned int ex
		塗りつぶしを終了するX座標。
	
	@param unsigned int ey
		塗りつぶしを終了するY座標。
	
	@param u32 color
		塗りつぶす色。
*/
void blitFillRect( unsigned int sx, unsigned int sy, unsigned int ex, unsigned int ey, u32 color );

/* blitLine
	指定した座標へ線を描画する。
	
	@param unsigned int sx
		線の開始X座標。
	
	@param unsigned int sy
		線の開始Y座標。
	
	@param unsigned int ex
		線の終了X座標。
	
	@param unsigned int ey
		線の終了Y座標。
	
	@param u32 color
		線の色。
*/
void blitLine( unsigned int sx, unsigned int sy, unsigned int ex, unsigned int ey, u32 color );

/* blitLineRect
	指定した座標へ矩形を描画する。
	
	@param unsigned int sx
		矩形の開始X座標。
	
	@param unsigned int sy
		矩形の開始Y座標。
	
	@param unsigned int ex
		矩形の終了X座標。
	
	@param unsigned int ey
		矩形の終了Y座標。
	
	@param u32 color
		矩形の色。
*/
void blitLineRect( unsigned int sx, unsigned int sy, unsigned int ex, unsigned int ey, u32 color );

/* blit8BitCharTableSwitch
	8bit asciiコードに割り当てる図形テーブル。
	
	B8_PSPSDK: PSPSDK標準の8ビットコードテーブル。
	B8_BUTTON_SUMBOL: \x80-\x88までしかない、ボタンを表す図形。
*/
void blit8BitCharTableSwitch( Blit8BitCharTable table );

/* blitStringf
	任意の位置に書式文字列を描画する。
	
	@param unsigned int sx
		描画を開始するX座標。
	
	@param unsigned int sy
		描画を開始するY座標。
	
	@param u32 fgcolor
		文字の色。
	
	@param u32 bgcolor
		文字の背景の色。
	
	@param size_t len
		最大文字数。
	
	@param const char *format
		printf形式の書式。
	
	@param ...
		可変長引数。
	
	@return int
		描画した文字数。
*/
int blitStringf( unsigned int sx, unsigned int sy, u32 fgcolor, u32 bgcolor, const char *format, ... );

int blitStringvf( unsigned int sx, unsigned int sy, u32 fgcolor, u32 bgcolor, const char *format, va_list ap );

#ifdef __cplusplus
}
#endif

#endif
