/*
	ダブルバッファリングを使用してグラフィックスの転送を行う。
*/

#ifndef GB_H
#define GB_H

#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspge.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include "cgerrs.h"

#ifndef SCR_WIDTH
#define SCR_WIDTH 480
#endif

#ifndef SCR_HEIGHT
#define SCR_HEIGHT 272
#endif

#ifndef BUF_WIDTH
#define BUF_WIDTH 512
#endif

/*-----------------------------------------------
	定数
-----------------------------------------------*/
#define GB_NOCACHE     0x40000000
#define GB_TRANSPARENT 0

#define GB_CHAR_WIDTH     8
#define GB_CHAR_HEIGHT    8
#define GB_LETTER_SPACING -2
#define GB_LINE_HEIGHT    0

/*-----------------------------------------------
	マクロ
-----------------------------------------------*/
#define GB_ALPHABLENDING( s, d, a, b ) ( ( ( ( s ) + 1 - ( d ) ) * ( a ) >> ( b ) ) + ( d ) )

#ifdef __cplusplus
extern "C" {
#endif
/*-----------------------------------------------
	型宣言
-----------------------------------------------*/
typedef uint32_t ( *GbColorConverter )( uint32_t, void* );

typedef enum {
	GB_ALPHABLEND     = 0x00000001,
	GB_AUTO_LINEBREAK = 0x00000002,
	GB_AUTOFLUSH      = 0x00000004
} GbOptions;

typedef enum {
	GB_FONT_ASCII = 0,
	GB_FONT_CP432,
	GB_FONT_PSPSYM,
} GbFontId;

typedef struct {
	int mode;
	int width;
	int height;
	int bufferWidth;
	enum PspDisplayPixelFormats pixelFormat;
	void *displayBuffer;
	void *drawBuffer;
	unsigned int opt;
	struct {
		unsigned int pixelBytes;
		unsigned int lineBytes;
		unsigned int frameBytes;
	} data;
} GbParams;

struct gb_font {
	const char *glyphset;
	unsigned int count;
};

/*-----------------------------------------------
	関数
-----------------------------------------------*/

/*
	gbGetBytesPerPixel
	
	ピクセルフォーマットを渡すと、そのピクセルフォーマットにおける
	1ピクセルの容量をバイト数で返す。
	
	@param: enum PspDisplayPixelFormats pxformat
		ピクセルフォーマット。
	
	@return: unsigned int
		1ピクセルの容量。
*/
unsigned int gbGetBytesPerPixel( enum PspDisplayPixelFormats pxformat );

/*
	gbAlphaBlendingXXXX
	
	標準的なアルファブレンドを行う。
	ブレンド方法を切り替えることはできない。
	
	関数名末尾のXXXXは、変換対象アドレスのピクセルフォーマット。
	
	@param: uint32_t fg
		ブレンドする色。
		PSP_DISPLAY_PIXEL_FORMAT_8888形式の色。
	
	@param: void *bgaddr
		ブレンドされる色。
		ピクセルフォーマットは関数名による。
	
	@return: uint32_t
		ブレンドされた色。
		常にuint32_tで返される。
*/
uint32_t gbAlphaBlending8888( uint32_t fg, void *bgaddr );
uint32_t gbAlphaBlending4444( uint32_t fg, void *bgaddr );
uint32_t gbAlphaBlending5551( uint32_t fg, void *bgaddr );
uint32_t gbAlphaBlending565( uint32_t fg, void *bgaddr );

/*
	gbColorConvertXXXX
	
	色をピクセルフォーマットに合わせる。
	
	関数末尾のXXXXは、変換後に望むピクセルフォーマット。
	
	@param: uint32_t color
		変換対象の色。
		PSP_DISPLAY_PIXEL_FORMAT_8888形式の色。
	
	@param: void *always_null
		この引数は無視される。
	
	@return: uint32_t
		変換後の色。
		常にuint32_tで返される。
*/
uint32_t gbColorConvert4444( uint32_t color, void *always_null );
uint32_t gbColorConvert5551( uint32_t color, void *always_null );
uint32_t gbColorConvert565( uint32_t color, void *always_null );

/*
	gbGetColorConverter
	
	指定された引数に応じて適切なgbAlphaBlendingXXXX、
	またはgbColorConverterXXXXの関数ポインタを返す。
	
	gbライブラリの関数は、内部でこの関数を自動的に呼び出すため、
	他のライブラリ関数に渡すために、あらかじめ色を変換したいなどの理由がなければ、
	この関数を使用する必要はない。
	
	@return: GbColorConverter
		色変換関数のポインタ。
*/
GbColorConverter gbGetColorConverter( bool alphablend, enum PspDisplayPixelFormats pxformat );

/*
	gbUse8BitFont
	
	8ビットフォントとして使用するグリフセットを指定する。
	
	@param: GbFontId
		フォントID。
			GB_FONT_CP432 : コードページ432
			GB_FONT_PSPSYM: PSPのボタン類のシンボル
*/
void gbUse8BitFont( GbFontId font );

/*
	gbInit
	
	gbライブラリを初期化し、ディスプレイを新たに設定する。
	ディスプレイが既に設定されていて、それを使用したい場合は代わりにgbBind()を呼ぶ。
	
	@param: int mode
		ディスプレイモード(?)。
		通常0。
	
	@param: int width
		画面サイズ(?)
		通常SCR_WIDTH。
	
	@param: int height
		画面の高さ(?)
		通常SCR_HEIGHT。
	
	@param: int bufwidth
		バッファの幅(?)。
		通常BUF_WIDTH。
	
	@param: enum PspDisplayPixelFormats pxformat
		設定するピクセルフォーマット。
*/
void gbInit( int mode, int width, int height, int bufwidth, enum PspDisplayPixelFormats pxformat );

/*
	gbBind
	
	gbライブラリの初期化に既に設定されているディスプレイ情報を使用する。
	ディスプレイの設定から行う場合はgbInit()を呼ぶ。
	
	@return: int
		正常終了時に0を返す。
		0未満でエラー。
*/
int gbBind( void );

/*
	gbPrepare
	
	現在のGbParamの値で描画準備を行う。
	gbInit()/gbBind()後に最低一回は呼ぶ。
*/
void gbPrepare( void );

/*
	gbSync
	
	現在のGbParamの値でPSPのディスプレイとフレームバッファを設定する。
*/
void gbSync( void );

/*
	gbSleep
	
	この状態になると画面に図形を転送する関数はなにもしなくなる。
	gbSleep()中にgbSleep()を呼んでも何も起こらない。
*/
void gbSleep( void );

/*
	gbWakeup
	
	gbSleep()を解除する。
	gbWakeup()後にgbWakeup()を呼んでも何も起こらない。
*/
void gbWakeup( void );

/*
	gbSetOpt
	
	オプション値をセットする。
	既に何か値がセットされていても無視して指定されたオプションのみを再セットする。
	
	@param: unsigned int opt
		GbOptionsの論理和。
*/		
void gbSetOpt( unsigned int opt );

/*
	gbAddOpt
	
	オプションを追加設定する。
	既に設定されているオプションに新たなオプションを追加する。
	
	@param: unsigned int opt
		GbOptionsの論理和。
*/
void gbAddOpt( unsigned int opt );

/*
	gbRmvOpt
	
	オプションを取り除く。
	既に設定されているオプションから指定されたオプションを取り除く。
	
	@param: unsigned int opt
		GbOptionsの論理和。
*/
void gbRmvOpt( unsigned int opt );

/*
	gbSetDisplayBuf
	
	表示バッファを設定する。
	この設定は必須で、これを設定せずに図形描画系の関数を使用した場合は未定義。
	
	設定するアドレスは16バイト境界にアライメントされている必要がある。
	
	@param: const void *addr
		表示バッファとして使用するメモリの先頭アドレス。
*/
void gbSetDisplayBuf( void *addr );

/*
	gbGetCurrentDisplayBuf
	
	現在の表示バッファの先頭アドレスを返す。
	gbSwapBuffers()で二つのバッファが切り替わるが、その際この返り値も切り替わる。
	
	@return: void *
		現在の表示バッファの先頭アドレス。
*/
void *gbGetCurrentDisplayBuf( void );

/*
	gbSetDrawBuf
	
	描画バッファを設定する。
	設定するアドレスは16バイト境界にアライメントされている必要がある。
	
	@param: const void *addr
		描画バッファとして使用するメモリの先頭アドレス。
*/
void gbSetDrawBuf( void *addr );

/*
	gbGetCurrentDrawBuf
	
	現在の描画バッファの先頭アドレスを返す。
	gbSwapBuffers()で二つのバッファが切り替わるが、その際この返り値も切り替わる。
	
	gbSetDrawBuf()で描画バッファを設定していないかNULLを設定している場合は、
	この関数はgbGetCurrentDisplayBuf()の返り値を返す。
	
	@return: void *
		現在の描画バッファの先頭アドレス。
*/
void *gbGetCurrentDrawBuf( void );

/*
	gbSwapBuffers
	
	表示バッファと描画バッファを切り替える。
	イメージ的には、sceGuSwapBuffers()とやっていることは同じ。
	
	内部でsceDisplaySetFrameBuf()を呼んでいるため、
	表示バッファ、NULLでない描画バッファのいずれかが、16バイト境界にアライメントされていない場合は失敗する。
	失敗した場合、エラー等を返すように作っていないので注意。
	
	返り値にgbGetCurrentDrawBuf()の返り値を返す。
	
	@return: void *
		次の描画バッファの先頭アドレス。
*/
void *gbSwapBuffers( void );

/*
	gbGetMode
	
	現在のPSPのディスプレイモード。
	詳細不明。
	通常0が返る模様。
	
	@return: int
		0
*/
int gbGetMode( void );

/*
	gbGetWidth
	
	現在のPSPの画面の横幅。
	通常SCR_WIDTH。
	
	@return: int
		横のピクセル数。
*/
int gbGetWidth( void );

/*
	gbGetHeight
	
	現在のPSPの画面の縦幅。
	通常SCR_HEIGHT。
	
	@return: int
		縦のピクセル数。
*/
int gbGetHeight( void );

/*
	gbGetBufferWidth
	
	現在のPSPのスクリーンの内部的な横幅。
	通常BUF_WIDTH。
	
	@return: int
		内部的な横幅。
*/
int gbGetBufferWidth( void );

/*
	gbGetPixelFormat
	
	現在のピクセルフォーマット。
	
	@return: enum PspDisplayPixelFormats
		ピクセルフォーマット。
*/
enum PspDisplayPixelFormats gbGetPixelFormat( void );

/*
	gbGetDataPixelSize
	
	1ピクセルの容量を取得する。
	
	@return: unsigned int
		1ピクセルの容量。
*/
unsigned int gbGetDataPixelSize( void );

/*
	gbGetDataLineSize
	
	フレームバッファの1行の容量を取得する。
	
	@return: unsigned int
		バッファ1行の容量。
*/
unsigned int gbGetDataLineSize( void );

/*
	gbGetDataFrameSize
	
	フレームバッファ1枚の容量を取得する。
	
	@return: unsigned int
		フレームバッファ1枚の容量。
*/
unsigned int gbGetDataFrameSize( void );

/*
	gbGetOpt
	
	現在設定されているオプション。
	
	@return: unsigned int
		GbOptionsの論理和。
*/
unsigned int gbGetOpt( void );

/*
	gbPrint
	
	描画バッファに文字列を書き込む。
	
	@param: int x
		書き込む位置のX座標。
	
	@param: int y
		書き込む位置のY座標。
	
	@param: uint32_t fgcolor
		文字の色。
	
	@param: uint32_t bgcolor
		文字の背景の色。
	
	@param: const char *str
		書き込む文字列。
	
	@return: int
		書き込んだ文字数。
*/
int gbPrint( int x, int y, uint32_t fgcolor, uint32_t bgcolor, const char *str );

/*
	guPutChar
	
	描画バッファに文字を書き込む。
	
	@param: int x
		書き込む位置のX座標。
	
	@param: int y
		書き込む位置のY座標。
	
	@param: uint32_t fgcolor
		文字の色。
	
	@param: uint32_t bgcolor
		文字の背景の色。
	
	@param: const char chr
		書き込む文字。
	
	@return: int
		書き込んだ文字数。
*/
int gbPutChar( int x, int y, uint32_t fgcolor, uint32_t bgcolor, const char chr );

/*
	gbPrintf
	
	描画バッファにフォーマット文字列を書き込む。
	
	@param: int x
		書き込む位置のX座標。
	
	@param: int y
		書き込む位置のY座標。
	
	@param: uint32_t fgcolor
		文字の色。
	
	@param: uint32_t bgcolor
		文字の背景の色。
	
	@param: const char *format
		printf互換のフォーマット文字列。
	
	@param: ...
	
	@return: int
		書き込んだ文字数。
*/
int gbPrintf( int x, int y, unsigned int fgcolor, unsigned int bgcolor, const char *format, ... );

/*
	gbPoint
	
	描画バッファに点を描き込む。
	
	@param: int x
		点を打つX座標。
	
	@param: int y
		点を打つY座標。
	
	@param: uint32_t color
		点の色。
*/
void gbPoint( int x, int y, uint32_t color );

/*
	gbLine
	
	描画バッファに線を引く。
	
	@param: int sx
		線の始点X座標。
	
	@param: int sy
		線の始点Y座標。
	
	@param: int ex
		線の終点X座標。
	
	@param:	int ey
		線の終点Y座標。
	
	@param: uint32_t color
		線の色。
*/
void gbLine( int sx, int sy, int ex, int ey, uint32_t color );

/*
	gbLineRect
	
	描画バッファに矩形を描き込む。
	
	@param: int sx
		矩形の左上の角のX座標。
	
	@param: int sy
		矩形の左上の角のY座標。
	
	@param: int ex
		矩形の右下の角のX座標。
	
	@param: int ey
		矩形の右下の角のY座標。
	
	@param: uint32_t color
		矩形の色。
*/		
void gbLineRect( int sx, int sy, int ex, int ey, uint32_t color );

/*
	gbFillRect
	
	描画バッファに塗りつぶし矩形を描き込む。
	
	@param: int sx
		矩形の左上の角のX座標。
	
	@param: int sy
		矩形の左上の角のY座標。
	
	@param: int ex
		矩形の右下の角のX座標。
	
	@param: int ey
		矩形の右下の角のY座標。
	
	@param: uint32_t color
		塗りつぶす色。
*/
void gbFillRect( int sx, int sy, int ex, int ey, uint32_t color );

/*
	gbOffsetChar
	
	文字幅を計算する。
	
	@param: x
		文字数。
	
	@return:
		文字幅。
*/
#define gbOffsetChar( x ) ( ( x ) * ( GB_CHAR_WIDTH + GB_LETTER_SPACING ) )

/*
	gbOffsetLine
	
	行の高さを計算する。
	
	@param: y
		行数。
	
	@return:
		行の高さ。
*/
#define gbOffsetLine( y ) ( ( y ) * ( GB_CHAR_HEIGHT + GB_LINE_HEIGHT ) )

/*
	gbLineRel
	
	描画バッファに線を引く。
	gbLineと同じだが、引数に相対座標を指定する点が異なる。
	
	@param: x
		始点X座標。
	
	@param: y
		始点Y座標。
	
	@param: e
		始点からの相対X座標。
	
	@param: f
		始点からの相対Y座標。
	
	@param: c
		線の色。
*/
#define gbLineRel( x, y, e, f, c )     gbLine( x, y, ( x ) + ( e ), ( y ) + ( f ), c )

/*
	gbLineRectRel
	
	描画バッファに矩形を描き込む。
	gbLineRectと同じだが、引数に相対座標を指定する点が異なる。
		
	@param: x
		矩形の左上の角のX座標。
	
	@param: y
		矩形の左上の角のY座標。
	
	@param: w
		矩形の左上の角のX座標からの相対的な長さ。
		
	@param: h
		矩形の左上の角のY座標からの相対的な長さ。
	
	@param: c
		矩形の色。
*/
#define gbLineRectRel( x, y, w, h, c ) gbLineRect( x, y, ( x ) + ( w ), ( y ) + ( h ), c )

/*
	gbFillRectRel
	
	描画バッファに塗りつぶし矩形を描き込む。
	gbFillRectと同じだが、引数に相対座標を指定する点が異なる。
		
	@param: x
		矩形の左上の角のX座標。
	
	@param: y
		矩形の左上の角のY座標。
	
	@param: w
		矩形の左上の角のX座標からの相対的な長さ。
		
	@param: h
		矩形の左上の角のY座標からの相対的な長さ。
	
	@param: c
		塗りつぶす色。
*/
#define gbFillRectRel( x, y, w, h, c ) gbFillRect( x, y, ( x ) + ( w ), ( y ) + ( h ), c )

/*
	gbLineCircle
	
	描画バッファに円弧を描き込む。
		
	@param: int x
		円の中心のX座標。
	
	@param: int y
		円の中心のY座標。
	
	@param: unsigned int radius
		半径。
	
	@param: c
		円の色。
*/
void gbLineCircle( int x, int y, unsigned int radius, uint32_t color );

#ifdef __cplusplus
}
#endif

#endif
