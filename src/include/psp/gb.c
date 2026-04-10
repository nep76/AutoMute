/*
	gb.c
	
	*フォント関連をなんとかする。
*/

#include "gb.h"

/*-----------------------------------------------
	フォント読み込み
-----------------------------------------------*/
#include "fonts/cg_ascii.c"
#include "fonts/pspsdk_cp432.c"
#include "fonts/pspsym.c"

/*-----------------------------------------------
	スタティック関数
-----------------------------------------------*/
static void *gb_calc_pos( int x, int y );
static void gb_swap( int *a, int *b );
static void gb_set_data( void );
static inline void gb_put_pixel( void *addr, uint32_t color, size_t len );

/*-----------------------------------------------
	スタティック変数
-----------------------------------------------*/
static GbParams st_gb;
static struct gb_font st_font[] = {
	{ font_cg_ascii,     sizeof( font_cg_ascii ) },
	{ font_pspsdk_cp432, sizeof( font_pspsdk_cp432 ) },
	{ font_pspsym,       sizeof( font_pspsym ) },
};
static GbFontId st_font_8bit = GB_FONT_CP432;
static GbColorConverter st_cconv;
static bool             st_draw = true;

/*=============================================*/

static void *gb_calc_pos( int x, int y )
{
	return (void *)( (unsigned int)( gbGetCurrentDrawBuf() ) + ( x + y * st_gb.bufferWidth ) * st_gb.data.pixelBytes );
}

static void gb_swap( int *a, int *b )
{
	int c = *a;
	*a = *b;
	*b = c;
}

static void gb_set_data( void )
{
	st_gb.data.pixelBytes = gbGetBytesPerPixel( st_gb.pixelFormat );
	st_gb.data.lineBytes  = st_gb.data.pixelBytes * st_gb.bufferWidth;
	st_gb.data.frameBytes = st_gb.data.lineBytes * st_gb.height;
}

static inline void gb_put_pixel( void *addr, uint32_t color, size_t len )
{
	unsigned int drawbuf = (unsigned int)gbGetCurrentDrawBuf();
	uint32_t     pixel;
	
	if( (unsigned int)addr >= drawbuf && (unsigned int)addr <= drawbuf + gbGetDataFrameSize() ){
		pixel = st_cconv ? ( st_cconv )( color, addr ) : color;
		memcpy( addr, (void *)&pixel, len );
	}
}

unsigned int gbGetBytesPerPixel( enum PspDisplayPixelFormats pxformat )
{
	switch( pxformat ){
		case PSP_DISPLAY_PIXEL_FORMAT_8888:
			return 4;
		default:
			return 2;
	}
}

uint32_t gbAlphaBlending8888( uint32_t fg, void *bgaddr )
{
	uint8_t alpha = ( fg >> 24 ) & 0xFF;
	uint32_t bg = *((uint32_t *)bgaddr);
	
	if( ! alpha ){
		return bg;
	} else if( alpha == 0xFF ){
		return fg;
	} else{
		uint32_t color = alpha << 24;
		color |= GB_ALPHABLENDING( ( fg       ) & 0xFF, ( bg       ) & 0xFF, alpha, 8 );
		color |= GB_ALPHABLENDING( ( fg >>  8 ) & 0xFF, ( bg >>  8 ) & 0xFF, alpha, 8 ) << 8;
		color |= GB_ALPHABLENDING( ( fg >> 16 ) & 0xFF, ( bg >> 16 ) & 0xFF, alpha, 8 ) << 16;
		return color;
	}
}

uint32_t gbAlphaBlending4444( uint32_t fg, void *bgaddr )
{
	uint8_t alpha = ( fg >> 12 ) & 0xF;
	uint16_t bg = *((uint32_t *)bgaddr);
	
	fg = gbColorConvert4444( fg, NULL );
	
	if( ! alpha ){
		return bg;
	} else if( alpha == 0xF ){
		return fg;
	} else{
		uint16_t color = alpha << 12;
		color |= GB_ALPHABLENDING( ( fg      ) & 0xF, ( bg      ) & 0xF, alpha, 4 );
		color |= GB_ALPHABLENDING( ( fg >> 4 ) & 0xF, ( bg >> 4 ) & 0xF, alpha, 4 ) << 4;
		color |= GB_ALPHABLENDING( ( fg >> 8 ) & 0xF, ( bg >> 4 ) & 0xF, alpha, 4 ) << 8;
		return color;
	}
}

uint32_t gbAlphaBlending5551( uint32_t fg, void *bgaddr )
{
	uint8_t  alpha = ( fg >> 27 ) & 0x1F;
	uint16_t bg = *((uint32_t *)bgaddr);
	
	fg = gbColorConvert5551( fg, NULL );
	
	if( ! alpha ){
		return bg;
	} else if( alpha == 0x1F ){
		return fg;
	} else{
		uint16_t color = ( alpha ? 1 : 0 ) << 15;
		color |= GB_ALPHABLENDING( ( fg       ) & 0x1F, ( bg       ) & 0x1F, alpha, 5 );
		color |= GB_ALPHABLENDING( ( fg >>  5 ) & 0x1F, ( bg >>  5 ) & 0x1F, alpha, 5 ) << 5;
		color |= GB_ALPHABLENDING( ( fg >> 10 ) & 0x1F, ( bg >> 10 ) & 0x1F, alpha, 5 ) << 10;
		return color;
	}
}

uint32_t gbAlphaBlending565( uint32_t fg, void *bgaddr )
{
	uint8_t  alpha = ( fg >> 26 ) & 0x3F;
	uint16_t bg = *((uint32_t *)bgaddr);
	
	fg = gbColorConvert565( fg, NULL );
	
	if( ! alpha ){
		return bg;
	} else if( alpha == 0x3F ){
		return fg;
	} else{
		uint16_t color = 0;
		color |= GB_ALPHABLENDING( ( fg       ) & 0x1F, ( bg       ) & 0x1F, ( alpha >> 1 ) & 0x1F, 5 );
		color |= GB_ALPHABLENDING( ( fg >>  5 ) & 0x3F, ( bg >>  5 ) & 0x3F, alpha                , 6 ) << 5;
		color |= GB_ALPHABLENDING( ( fg >> 11 ) & 0x1F, ( bg >> 11 ) & 0x1F, ( alpha >> 1 ) & 0x1F, 5 ) << 11;
		return color;
	}
}

uint32_t gbColorConvert565( uint32_t color, void *always_null )
{
	unsigned int r, g, b;
	
	r = ( color >> 3  ) & 0x1F;
	g = ( color >> 10 ) & 0x3F;
	b = ( color >> 19 ) & 0x1F;
	
	return ( r | ( g << 5 ) | ( b << 11 ) );
}

uint32_t gbColorConvert5551( uint32_t color, void *always_null )
{
	unsigned int r, g, b, a;
	
	r = ( color >> 3  ) & 0x1F;
	g = ( color >> 11 ) & 0x1F;
	b = ( color >> 19 ) & 0x1F;
	a = ( color >> 24 ) ? 1 : 0;
	
	return ( r | ( g << 5 ) | ( b << 10 ) | ( a << 15 ) );
}

uint32_t gbColorConvert4444( uint32_t color, void *always_null )
{
	unsigned int r, g, b, a;
	
	r = ( color >> 4  ) & 0xF;
	g = ( color >> 12 ) & 0xF;
	b = ( color >> 20 ) & 0xF;
	a = ( color >> 28 ) & 0xF;
	
	return ( r | ( g << 4 ) | ( b << 8 ) | ( a << 12 ) );
}

GbColorConverter gbGetColorConverter( bool alphablend, enum PspDisplayPixelFormats pxformat )
{
	if( alphablend ){
		switch( pxformat ){
			case PSP_DISPLAY_PIXEL_FORMAT_8888:
				return gbAlphaBlending8888;
			case PSP_DISPLAY_PIXEL_FORMAT_4444:
				return gbAlphaBlending4444;
			case PSP_DISPLAY_PIXEL_FORMAT_5551:
				return gbAlphaBlending5551;
			case PSP_DISPLAY_PIXEL_FORMAT_565:
				return gbAlphaBlending565;
		}
	} else{
		switch( pxformat ){
			case PSP_DISPLAY_PIXEL_FORMAT_8888:
				return NULL;
			case PSP_DISPLAY_PIXEL_FORMAT_4444:
				return gbColorConvert4444;
			case PSP_DISPLAY_PIXEL_FORMAT_5551:
				return gbColorConvert5551;
			case PSP_DISPLAY_PIXEL_FORMAT_565:
				return gbColorConvert565;
		}
	}
	return NULL;
}

void gbUse8BitFont( GbFontId font )
{
	if( font == GB_FONT_ASCII ){
		st_font_8bit = GB_FONT_CP432;
	} else{
		st_font_8bit = font;
	}
}

void gbInit( int mode, int width, int height, int bufwidth, enum PspDisplayPixelFormats pxformat )
{
	memset( &st_gb, 0, sizeof( GbParams ) );
	
	st_gb.mode        = mode;
	st_gb.width       = width;
	st_gb.height      = height;
	st_gb.bufferWidth = bufwidth;
	st_gb.pixelFormat = pxformat;
	
	gb_set_data();
}

int gbBind( void )
{
	int rc;
	
	memset( &st_gb, 0, sizeof( GbParams ) );
	
	rc = sceDisplayGetMode( &(st_gb.mode), &(st_gb.width), &(st_gb.height) );
	if( rc < 0 ) return rc;
	
	rc = sceDisplayGetFrameBuf( &(st_gb.displayBuffer), &(st_gb.bufferWidth), (int *)&(st_gb.pixelFormat), PSP_DISPLAY_SETBUF_IMMEDIATE );
	if( rc < 0 ) return rc;
	
	if( ! ( st_gb.bufferWidth ) || ! ( st_gb.displayBuffer ) ) return CG_ERROR_DISPLAY_NOT_INITIALIZED;
	
	gb_set_data();
	
	return 0;
}

void gbSync( void )
{
	sceDisplaySetMode( st_gb.mode, st_gb.width, st_gb.height );
	sceDisplaySetFrameBuf( st_gb.displayBuffer, st_gb.bufferWidth, (int)(st_gb.pixelFormat), PSP_DISPLAY_SETBUF_IMMEDIATE );
}

void gbSleep( void )
{
	st_draw = false;
}

void gbWakeup( void )
{
	st_draw = true;
}

void gbPrepare( void )
{
	if( st_gb.opt & GB_AUTOFLUSH ){
		if( st_gb.displayBuffer ) st_gb.displayBuffer = (void *)( (unsigned int)(st_gb.displayBuffer) | GB_NOCACHE );
		if( st_gb.drawBuffer )    st_gb.drawBuffer    = (void *)( (unsigned int)(st_gb.drawBuffer) | GB_NOCACHE );
	}
	
	st_cconv = gbGetColorConverter( st_gb.opt & GB_ALPHABLEND ? true : false, st_gb.pixelFormat );
}

void gbSetOpt( unsigned int opt )
{
	st_gb.opt = opt;
}

void gbAddOpt( unsigned int opt )
{
	st_gb.opt |= opt;
}

void gbRmvOpt( unsigned int opt )
{
	st_gb.opt &= ~opt;
}

void gbSetDisplayBuf( void *addr )
{
	st_gb.displayBuffer = addr;
}

void *gbGetCurrentDisplayBuf( void )
{
	return st_gb.displayBuffer;
}

void gbSetDrawBuf( void *addr )
{
	st_gb.drawBuffer = addr;
}

void *gbGetCurrentDrawBuf( void )
{
	return st_gb.drawBuffer ? st_gb.drawBuffer : st_gb.displayBuffer;
}

void *gbSwapBuffers( void )
{
	if( st_gb.drawBuffer ){
		void *swap = st_gb.displayBuffer;
		st_gb.displayBuffer = st_gb.drawBuffer;
		st_gb.drawBuffer = swap;
		
		sceDisplaySetFrameBuf( st_gb.displayBuffer, st_gb.bufferWidth, st_gb.pixelFormat, PSP_DISPLAY_SETBUF_IMMEDIATE );
	}
	
	return gbGetCurrentDrawBuf();
}

int gbGetMode( void )
{
	return st_gb.mode;
}

int gbGetWidth( void )
{
	return st_gb.width;
}

int gbGetHeight( void )
{
	return st_gb.height;
}

int gbGetBufferWidth( void )
{
	return st_gb.bufferWidth;
}

enum PspDisplayPixelFormats gbGetPixelFormat( void )
{
	return st_gb.pixelFormat;
}

unsigned int gbGetOpt( void )
{
	return st_gb.opt;
}

unsigned int gbGetDataPixelSize( void )
{
	return st_gb.data.pixelBytes;
}

unsigned int gbGetDataLineSize( void )
{
	return st_gb.data.lineBytes;
}

unsigned int gbGetDataFrameSize( void )
{
	return st_gb.data.frameBytes;
}

int gbPrint( int x, int y, uint32_t fgcolor, uint32_t bgcolor, const char *str )
{
	unsigned int addr, drawstart;
	unsigned int i, char_off, char_x_off, char_y_off;
	const char   *glyph;
	char         glyphline;
	unsigned int char_width;
	unsigned int line_height;
	uint32_t     color;
	
	char_width  = gbOffsetChar( 1 );
	line_height = gbOffsetLine( 1 );
	
	if( ! st_draw || x + char_width > st_gb.width ) return 0;
	
	drawstart = (unsigned int)gb_calc_pos( x, y );
	char_off  = 0;
	
	for( i = 0; str[i]; i++ ){
		/* 改行 */
		if( str[i] == '\n' || ( ( st_gb.opt & GB_AUTO_LINEBREAK ) && ( x + char_off + char_width >= st_gb.width ) ) ){
			char_off  = 0;
			drawstart += st_gb.data.lineBytes * line_height;
			if( str[i] == '\n' ) continue;
		}
		
		/* グリフ取得 */
		if( (unsigned int)str[i] < 0x80 ){
			glyph = &(st_font[GB_FONT_ASCII].glyphset[str[i] * line_height]);
		} else{
			unsigned int gi = ( str[i] & 0x7F ) * line_height;
			if( gi < st_font[st_font_8bit].count ){
				glyph = &(st_font[st_font_8bit].glyphset[gi]);
			} else{
				glyph = &(st_font[GB_FONT_ASCII].glyphset['?' * line_height]);
			}
		}
		
		/* 描画 */
		for( char_y_off = 0; char_y_off < line_height; char_y_off++ ){
			addr = drawstart + ( char_y_off * st_gb.data.lineBytes ) + ( char_off * st_gb.data.pixelBytes );
			
			glyphline = glyph[char_y_off];
			
			for( char_x_off = 0; char_x_off < char_width; char_x_off++, glyphline <<= 1, addr += st_gb.data.pixelBytes ){
				color = glyphline & 0x80 ? fgcolor : bgcolor;
				if( color != GB_TRANSPARENT ) gb_put_pixel( (void *)addr, color, st_gb.data.pixelBytes );
			}
		}
		char_off += char_width;
	}
	return i;
}

int gbPutChar( int x, int y, uint32_t fgcolor, uint32_t bgcolor, const char chr )
{
	char putchr[] = { chr, '\0' };
	return gbPrint( x, y, fgcolor, bgcolor, putchr );
}

int gbPrintf( int x, int y, uint32_t fgcolor, uint32_t bgcolor, const char *format, ... )
{
	char str[255];
	va_list ap;
	
	va_start( ap, format );
	vsnprintf( str, sizeof( str ), format, ap );
	va_end( ap );
	
	return gbPrint( x, y, fgcolor, bgcolor, str );
}

void gbPoint( int x, int y, uint32_t color )
{
	unsigned int drawpos;
	
	if( ! st_draw || color == GB_TRANSPARENT ) return;
	
	drawpos   = (unsigned int)gb_calc_pos( x, y );
	gb_put_pixel( (void *)drawpos, color, st_gb.data.pixelBytes );
}

/* ブレゼンハムの線分描画アルゴリズム */
void gbLine( int sx, int sy, int ex, int ey, uint32_t color )
{
	unsigned int drawpos;
	int          e, dx, dy;
	
	if( ! st_draw || color == GB_TRANSPARENT ) return;
	
	if( sx > ex ) gb_swap( &sx, &ex );
	if( sy > ey ) gb_swap( &sy, &ey );
	
	e  = 0;
	dx = ex - sx;
	dy = ey - sy;
	
	drawpos = (unsigned int)gb_calc_pos( sx, sy );
	
	if( dx > dy ){
		int x;
		for( x = sx; x <= ex; x++, drawpos += st_gb.data.pixelBytes ){
			e += dy;
			if( e > dx ){
				e -= dx;
				drawpos += st_gb.data.lineBytes;
			}
			gb_put_pixel( (void *)drawpos, color, st_gb.data.pixelBytes );
		}
	} else{
		int y;
		for( y = sy; y <= ey; y++, drawpos += st_gb.data.lineBytes ){
			e += dx;
			if( e > dy ){
				e -= dy;
				drawpos += st_gb.data.pixelBytes;
			}
			gb_put_pixel( (void *)drawpos, color, st_gb.data.pixelBytes );
		}
	}
}

void gbLineRect( int sx, int sy, int ex, int ey, uint32_t color )
{
	if( ! st_draw || color == GB_TRANSPARENT ) return;
	
	gbLine( sx, sy, ex, sy, color );
	gbLine( ex, sy, ex, ey, color );
	gbLine( ex, ey, sx, ey, color );
	gbLine( sx, ey, sx, sy, color );
}
 
void gbFillRect( int sx, int sy, int ex, int ey, uint32_t color )
{
	unsigned int drawstart, offset;
	int          w, h, x;
	
	if( ! st_draw || color == GB_TRANSPARENT ) return;
	
	if( sx > ex ) gb_swap( &sx, &ex );
	if( sy > ey ) gb_swap( &sy, &ey );
	
	w = ex - sx;
	h = ey - sy;
	
	drawstart = (unsigned int)gb_calc_pos( sx, sy );
	
	for( ; h; h--, drawstart += st_gb.data.lineBytes ){
		offset = 0;
		for( x = 0; x < w; x++, offset += st_gb.data.pixelBytes ){
			gb_put_pixel( (void *)( drawstart + offset ), color, st_gb.data.pixelBytes );
		}
	}
}

/* ミッチェナーの円弧描画アルゴリズム */
void gbLineCircle( int x, int y, unsigned int radius, uint32_t color )
{
	int cx = 0, cy = radius;
	int d = 3 - 2 * radius;
	
	if( ! st_draw || color == GB_TRANSPARENT || ! radius ) return;
	
	/* 開始点 */
	gbPoint( x, y + radius, color ); /* ( 0, R) */
	gbPoint( x, y - radius, color ); /* ( 0,-R) */
	gbPoint( x + radius, y, color ); /* ( R, 0) */
	gbPoint( x - radius, y, color ); /* (-R, 0) */
	
	for( cx = 0; cx <= cy; cx++ ){
		if( d >= 0 ){
			d += 10 + 4 * cx - 4 * cy--;
		} else{
			d += 6 + 4 * cx;
		}
		
		gbPoint( x + cy, y + cx, color ); /*   0-45  度 */
		gbPoint( x + cx, y + cy, color ); /*  45-90  度 */
		gbPoint( x - cx, y + cy, color ); /*  90-135 度 */
		gbPoint( x - cy, y + cx, color ); /* 135-180 度 */
		gbPoint( x - cy, y - cx, color ); /* 180-225 度 */
		gbPoint( x - cx, y - cy, color ); /* 225-270 度 */
		gbPoint( x + cx, y - cy, color ); /* 270-315 度 */
		gbPoint( x + cy, y - cx, color ); /* 315-360 度 */
	}
}
