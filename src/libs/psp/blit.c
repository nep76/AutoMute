/*
	PSP VSH 24bpp text bliter (Original from "SDK for devhook 0.43")
*/

#include "font.c"
#include "blit.h"


BlitFontTable st_fonttable[] = {
	{ font7Ascii, sizeof( font7Ascii ) },
	{ font8Pspsdk, sizeof( font8Pspsdk ) }
};

static void blit_get_display_status( BlitDisplayStatus *stat )
{
	sceDisplayGetMode( &(stat->mode), &(stat->width), &(stat->height) );
	sceDisplayGetFrameBuf( (void *)&(stat->frameBuffer), &(stat->bufferWidth), (int *)&(stat->pixelFormat), PSP_DISPLAY_SETBUF_IMMEDIATE );
}

u16 blitColorConvert565( u32 color )
{
	unsigned int r, g, b;
	
	r = ( color >> 3  ) & MASK_5BIT;
	g = ( color >> 10 ) & MASK_6BIT;
	b = ( color >> 19 ) & MASK_5BIT;
	
	return ( r | ( g << 5 ) | ( b << 11 ) );
}

u16 blitColorConvert5551( u32 color )
{
	unsigned int a, r, g, b;
	
	a = ( color >> 24 ) ? ALPHA_BIT : 0;
	r = ( color >> 3  ) & MASK_5BIT;
	g = ( color >> 11 ) & MASK_5BIT;
	b = ( color >> 19 ) & MASK_5BIT;
	
	return ( a | r | ( g << 5 ) | ( b << 10 ) );
}

u16 blitColorConvert4444( u32 color )
{
	unsigned int a, r, g, b;
	
	a = ( color >> 28 ) & MASK_4BIT; 
	r = ( color >> 4  ) & MASK_4BIT;
	g = ( color >> 12 ) & MASK_4BIT;
	b = ( color >> 20 ) & MASK_4BIT;
	
	return ( ( a << 12 ) | r | ( g << 4 ) | ( b << 8 ) );
}

int blitGetPixelLength( enum PspDisplayPixelFormats pxfmt )
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

u32 blitConvertColorFrom8888( enum PspDisplayPixelFormats pxfmt, u32 color )
{
	switch( pxfmt ){
		case PSP_DISPLAY_PIXEL_FORMAT_565:
			return blitColorConvert565( color );
		case PSP_DISPLAY_PIXEL_FORMAT_5551:
			return blitColorConvert5551( color );
		case PSP_DISPLAY_PIXEL_FORMAT_4444:
			return blitColorConvert4444( color );
		case PSP_DISPLAY_PIXEL_FORMAT_8888:
		default:
			return color;
	}
}

int blitChar( unsigned int sx, unsigned int sy, const char chr, u32 fgcolor, u32 bgcolor )
{
	char tstr[2] = { chr, '\0' };
	return blitString( sx ,sy, fgcolor, bgcolor, tstr );
}

int blitString( unsigned int sx, unsigned int sy, u32 fgcolor, u32 bgcolor, const char *msg )
{
	int c, x, y, p;
	int base_offset, offset, pxlen;
	unsigned char font;
	BlitDisplayStatus dstat;
	
	blit_get_display_status( &dstat );
	
	if( ! dstat.bufferWidth || ! dstat.frameBuffer ) return -1;
	
	pxlen   = blitGetPixelLength( dstat.pixelFormat );
	fgcolor = blitConvertColorFrom8888( dstat.pixelFormat, fgcolor );
	bgcolor = blitConvertColorFrom8888( dstat.pixelFormat, bgcolor );
	
	base_offset = sx + ( sy * dstat.bufferWidth );
	
	for( c = 0, x = 0; msg[c]; c++, x++ ){
		if( msg[c] == '\n' || sx + ( x + 1 ) * BLIT_CHAR_WIDTH >= dstat.width ){
			sy += BLIT_CHAR_HEIGHT;
			base_offset = sx + ( sy * dstat.bufferWidth );
			if( msg[c] == '\n' ){
				x = -1;
				continue;
			} else{
				x = 0;
			}
		}
		
		for( y = 0; y < BLIT_CHAR_HEIGHT; y++ ){
			offset = base_offset + ( y * dstat.bufferWidth ) + ( x * BLIT_CHAR_WIDTH );
			if( (unsigned char)msg[c] < 0x80 ){
				/* 7bit ASCII */
				font = st_fonttable[0].table[ msg[c]*8 + y ];
			} else{
				/* 8bit ASCII */
				unsigned char t_c = (msg[c] & 0x7f)*8 + y;
				if( t_c < st_fonttable[1].charnum ){
					/* 文字が定義されていれば使用 */
					font = st_fonttable[1].table[t_c];
				} else{
					/* 文字がテーブルの最大文字数を超えていたら ? を表示 */
					font = st_fonttable[0].table[ '?'*8 + y ];
				}
			}
			for( p = 0; p < BLIT_CHAR_WIDTH; p++ ){
				if( font & 0x80 ){
					memcpy( (void *)((unsigned int)(dstat.frameBuffer) + ( offset * pxlen )), (void *)&fgcolor, pxlen );
				} else{
					memcpy( (void *)((unsigned int)(dstat.frameBuffer) + ( offset * pxlen )), (void *)&bgcolor, pxlen );
				}
				font <<= 1;
				offset++;
			}
		}
	}
	return c;
}

void blitFillRect( unsigned int sx, unsigned int sy, unsigned int ex, unsigned int ey, u32 color )
{
	unsigned int cx, cy;
	int offset, pxlen;
	BlitDisplayStatus dstat;
	
	blit_get_display_status( &dstat );
	
	pxlen = blitGetPixelLength( dstat.pixelFormat );
	color = blitConvertColorFrom8888( dstat.pixelFormat, color );
	
	if( sx > 512 ) sx = 512;
	if( ex > 512 ) ex = 512;
	if( sy > 272 ) sy = 272;
	if( ey > 272 ) ey = 272;
	
	for( cy = sy; cy <= ey; cy++ ){
		offset = ( sx + cy * dstat.bufferWidth ) * pxlen;
		for( cx = sx; cx <= ex; cx++ ){
			memcpy( (void *)((unsigned int)(dstat.frameBuffer) + offset), (void *)&color, pxlen );
			offset += pxlen;
		}
	}
}

/* bresenhamの線分描画アルゴリズム */
void blitLine( unsigned int sx, unsigned int sy, unsigned int ex, unsigned int ey, u32 color )
{
	unsigned int dx, dy;
	unsigned int e = 0, pxlen;
	BlitDisplayStatus dstat;
	
	blit_get_display_status( &dstat );
	
	pxlen = blitGetPixelLength( dstat.pixelFormat );
	color = blitConvertColorFrom8888( dstat.pixelFormat, color );
	
	if( sx > 512 ) sx = 512;
	if( ex > 512 ) ex = 512;
	if( sy > 272 ) sy = 272;
	if( ey > 272 ) ey = 272;
	
	if( sx > ex ){
		/* 値交換 */
		int tx = sx;
		sx = ex;
		ex = tx;
		dx = sx - ex;
	} else{
		dx = ex - sx;
	}
	if( sy > ey ){
		/* 値交換 */
		int ty = sy;
		sy = ey;
		ey = ty;
		dy = sy - ey;
	} else{
		dy = ey - sy;
	}
	
	if( dx > dy ){
		unsigned int x, y;
		for( x = sx, y = sy; x <= ex; x++ ){
			e += dy;
			if( e > dx ){
				e -= dx;
				y++;
			}
			memcpy( (void *)((unsigned int)(dstat.frameBuffer) + ( ( x + ( y * dstat.bufferWidth ) ) * pxlen )), (void *)&color, pxlen );
		}
	} else{
		unsigned int x, y;
		for( x = sx, y = sy; y <= ey; y++ ){
			e += dx;
			if( e > dy ){
				e -= dy;
				x++;
			}
			memcpy( (void *)((unsigned int)(dstat.frameBuffer) + ( ( x + ( y * dstat.bufferWidth ) ) * pxlen )), (void *)&color, pxlen );
		}
	}
}

void blitLineRect( unsigned int sx, unsigned int sy, unsigned int ex, unsigned int ey, u32 color )
{
	blitLine( sx, sy, ex, sy, color );
	blitLine( ex, sy, ex, ey, color );
	blitLine( ex, ey, sx, ey, color );
	blitLine( sx, ey, sx, sy, color );
}

void blit8BitCharTableSwitch( Blit8BitCharTable table )
{
	switch( table ){
		case B8_BUTTON_SYMBOL:
			st_fonttable[1].table   = font8ButtonSymbols;
			st_fonttable[1].charnum = sizeof( font8ButtonSymbols );
			break;
		case B8_PSPSDK:
		default:
			st_fonttable[1].table  = font8Pspsdk;
			st_fonttable[1].charnum = sizeof( font8Pspsdk );
			break;
	}
}

int blitStringvf( unsigned int sx, unsigned int sy, u32 fgcolor, u32 bgcolor, const char *format, va_list ap )
{
	char str[255];
	
	vsnprintf( str, 255, format, ap );
	return blitString( sx, sy, fgcolor, bgcolor, str );
}

int blitStringf( unsigned int sx, unsigned int sy, u32 fgcolor, u32 bgcolor, const char *format, ... )
{
	int c;
	va_list ap;
	
	va_start( ap, format );
	c = blitStringvf( sx, sy, fgcolor, bgcolor, format, ap );
	va_end( ap );
	
	return c;
}
