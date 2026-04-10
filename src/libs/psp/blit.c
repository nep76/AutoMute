/*
	PSP text blitter (Original from "SDK for devhook 0.43")
*/

#include "font.c"
#include "blit.h"

/*-----------------------------------------------
	ローカル関数
-----------------------------------------------*/
static BlitAlphaBlender blit_get_blender_by_pixelformat( enum PspDisplayPixelFormats pxfmt );
static void *blit_autoflush( unsigned int opt, void *addr );

/*-----------------------------------------------
	ローカル変数
-----------------------------------------------*/
static FbmgrDisplayStat st_dstat;
static int st_pxlen;
static unsigned int st_options;
static BlitFontTable st_fonttable[] = {
	{ font7Ascii, sizeof( font7Ascii ) },
	{ font8Pspsdk, sizeof( font8Pspsdk ) }
};
/*=============================================*/

static BlitAlphaBlender blit_get_blender_by_pixelformat( enum PspDisplayPixelFormats pxfmt )
{
	if( ! ( st_options & BO_ALPHABLEND ) ) return NULL;
	
	switch( pxfmt ){
		case PSP_DISPLAY_PIXEL_FORMAT_8888:
			return blitAlphaBlending8888;
		case PSP_DISPLAY_PIXEL_FORMAT_4444:
			return blitAlphaBlending4444;
		case PSP_DISPLAY_PIXEL_FORMAT_5551:
			return blitAlphaBlending5551;
		case PSP_DISPLAY_PIXEL_FORMAT_565:
			return blitAlphaBlending565;
	}
	return NULL;
}

static void *blit_autoflush( unsigned int opt, void *addr )
{
	if( opt & BO_AUTOFLUSH ){
		return (void *)( (unsigned int)addr | FBMGR_MEM_NOCACHE );
	} else if( ((unsigned int)addr) & FBMGR_MEM_NOCACHE ){
		return (void *)( (unsigned int)addr ^ FBMGR_MEM_NOCACHE );
	}
	return addr;
}

int blitInit( void )
{
	if(
		fbmgrGetDisplayStat( &st_dstat ) ||
		! st_dstat.bufferWidth || ! st_dstat.frameBuffer /* スリープ実行時に0が戻ってくるっぽい? */
	) return -1;
	
	st_pxlen = fbmgrGetPixelLength( st_dstat.pixelFormat );
	
	return 0;
}

void blitSetOptions( unsigned int opt )
{
	st_options = opt;
	
	/* オートフラッシュフラグの処理 */
	st_dstat.frameBuffer = blit_autoflush( st_options, st_dstat.frameBuffer );
}

void blitGetFrameBufStat( FbmgrDisplayStat *stat )
{
	*stat = st_dstat;
}

int blitGetFrameBufPixelLength( void )
{
	return st_pxlen;
}

void blitSetFrameBufAddrTop( void *addr )
{
	st_dstat.frameBuffer = blit_autoflush( st_options, addr );
}

uint32_t blitAlphaBlending8888( void *fg, void *bg )
{
	uint32_t src   = *((uint32_t *)fg);
	uint32_t dest  = *((uint32_t *)bg);
	uint8_t  alpha = ( src >> 24 ) & 0xFF;
	
	if( ! alpha ){
		return dest;
	} else if( alpha == 0xFF ){
		return src;
	} else{
		uint32_t color = alpha << 24;
		color |= BLIT_ALPHA_BLEND( ( src       ) & 0xFF, ( dest       ) & 0xFF, alpha, 8 );
		color |= BLIT_ALPHA_BLEND( ( src >>  8 ) & 0xFF, ( dest >>  8 ) & 0xFF, alpha, 8 ) << 8;
		color |= BLIT_ALPHA_BLEND( ( src >> 16 ) & 0xFF, ( dest >> 16 ) & 0xFF, alpha, 8 ) << 16;
		return color;
	}
}

uint32_t blitAlphaBlending4444( void *fg, void *bg )
{
	uint16_t src   = blitColorConvert4444( *((uint32_t *)fg) );
	uint16_t dest  = *((uint16_t *)bg);
	uint8_t  alpha = ( src >> 12 ) & 0xF;
	
	return src;
	
	if( ! alpha ){
		return dest;
	} else if( alpha == 0xF ){
		return src;
	} else{
		uint16_t color = alpha << 12;
		color |= BLIT_ALPHA_BLEND( ( src      ) & 0xF, ( dest      ) & 0xF, alpha, 4 );
		color |= BLIT_ALPHA_BLEND( ( src >> 4 ) & 0xF, ( dest >> 4 ) & 0xF, alpha, 4 ) << 4;
		color |= BLIT_ALPHA_BLEND( ( src >> 8 ) & 0xF, ( dest >> 4 ) & 0xF, alpha, 4 ) << 8;
		return color;
	}
}

uint32_t blitAlphaBlending5551( void *fg, void *bg )
{
	uint8_t  alpha = ( *((uint32_t *)fg) >> 27 ) & 0x1F;
	uint16_t src   = blitColorConvert5551( *((uint32_t *)fg) );
	uint16_t dest  = *((uint16_t *)bg);
	
	if( ! alpha ){
		return dest;
	} else if( alpha == 0x1F ){
		return src;
	} else{
		uint16_t color = ( alpha ? 1 : 0 ) << 15;
		color |= BLIT_ALPHA_BLEND( ( src       ) & 0x1F, ( dest       ) & 0x1F, alpha, 5 );
		color |= BLIT_ALPHA_BLEND( ( src >>  5 ) & 0x1F, ( dest >>  5 ) & 0x1F, alpha, 5 ) << 5;
		color |= BLIT_ALPHA_BLEND( ( src >> 10 ) & 0x1F, ( dest >> 10 ) & 0x1F, alpha, 5 ) << 10;
		return color;
	}
}

uint32_t blitAlphaBlending565( void *fg, void *bg )
{
	uint8_t  alpha = ( *((uint32_t *)fg) >> 26 ) & 0x3F;
	uint16_t src   = blitColorConvert565( *((uint32_t *)fg) );
	uint16_t dest  = *((uint16_t *)bg);
	
	if( ! alpha ){
		return dest;
	} else if( alpha == 0x3F ){
		return src;
	} else{
		uint16_t color = 0;
		color |= BLIT_ALPHA_BLEND( ( src       ) & 0x1F, ( dest       ) & 0x1F, ( alpha >> 1 ) & 0x1F, 5 );
		color |= BLIT_ALPHA_BLEND( ( src >>  5 ) & 0x3F, ( dest >>  5 ) & 0x3F, alpha                , 6 ) << 5;
		color |= BLIT_ALPHA_BLEND( ( src >> 11 ) & 0x1F, ( dest >> 11 ) & 0x1F, ( alpha >> 1 ) & 0x1F, 5 ) << 11;
		return color;
	}
}

uint16_t blitColorConvert565( uint32_t color )
{
	unsigned int r, g, b;
	
	r = ( color >> 3  ) & 0x1F;
	g = ( color >> 10 ) & 0x3F;
	b = ( color >> 19 ) & 0x1F;
	
	return ( r | ( g << 5 ) | ( b << 11 ) );
}

uint16_t blitColorConvert5551( uint32_t color )
{
	unsigned int r, g, b, a;
	
	r = ( color >> 3  ) & 0x1F;
	g = ( color >> 11 ) & 0x1F;
	b = ( color >> 19 ) & 0x1F;
	a = ( color >> 24 ) ? 1 : 0;
	
	return ( r | ( g << 5 ) | ( b << 10 ) | ( a << 15 ) );
}

uint16_t blitColorConvert4444( uint32_t color )
{
	unsigned int r, g, b, a;
	
	r = ( color >> 4  ) & 0xF;
	g = ( color >> 12 ) & 0xF;
	b = ( color >> 20 ) & 0xF;
	a = ( color >> 28 ) & 0xF;
	
	return ( r | ( g << 4 ) | ( b << 8 ) | ( a << 12 ) );
}

int blitChar( unsigned int sx, unsigned int sy, uint32_t fgcolor, uint32_t bgcolor, const char chr )
{
	char tstr[2] = { chr, '\0' };
	return blitString( sx ,sy, fgcolor, bgcolor, tstr );
}

int blitString( unsigned int sx, unsigned int sy, uint32_t fgcolor, uint32_t bgcolor, const char *msg )
{
	void *addr;
	unsigned int c, x, y, p;
	unsigned int base_offset, offset;
	unsigned char glyph;
	uint32_t blend_color;
	BlitAlphaBlender blender = blit_get_blender_by_pixelformat( st_dstat.pixelFormat );
	
	base_offset = ( sx + ( sy * st_dstat.bufferWidth ) );
	
	for( c = 0, x = 0; msg[c]; c++, x++ ){
		if( msg[c] == '\n' || sx + ( x + 1 ) * BLIT_CHAR_WIDTH >= st_dstat.width ){
			sy += BLIT_CHAR_HEIGHT;
			base_offset = sx + ( sy * st_dstat.bufferWidth );
			if( msg[c] == '\n' ){
				x = -1;
				continue;
			} else{
				x = 0;
			}
		}
		
		for( y = 0; y < BLIT_CHAR_HEIGHT; y++ ){
			offset = ( base_offset + ( y * st_dstat.bufferWidth ) + ( x * BLIT_CHAR_WIDTH ) ) * st_pxlen;
			if( (unsigned char)msg[c] < 0x80 ){
				/* 7bit ASCII */
				glyph = st_fonttable[0].table[ msg[c]*8 + y ];
			} else{
				/* 8bit ASCII */
				unsigned char t_c = (msg[c] & 0x7f)*8 + y;
				if( t_c < st_fonttable[1].charnum ){
					/* 文字が定義されていれば使用 */
					glyph = st_fonttable[1].table[t_c];
				} else{
					/* 文字がテーブルの最大文字数を超えていたら ? を表示 */
					glyph = st_fonttable[0].table[ '?'*8 + y ];
				}
			}
			for( p = 0; p < BLIT_CHAR_WIDTH; p++, offset += st_pxlen, glyph <<= 1 ){
				addr = (void *)((unsigned int)(st_dstat.frameBuffer) + offset);
				if( glyph & 0x80 ){
					blend_color = fgcolor;
				} else{
					blend_color = bgcolor;
				}
				
				if( blend_color != BLIT_TRANSPARENT ){
					if( blender ) blend_color = ( blender )( &blend_color, addr );
					memcpy( addr, (void *)&blend_color, st_pxlen );
				}
			}
		}
	}
	return c;
}

void blitFillRect( unsigned int sx, unsigned int sy, unsigned int ex, unsigned int ey, uint32_t color )
{
	unsigned int cx, cy;
	unsigned int offset;
	void *addr;
	uint32_t blend_color;
	BlitAlphaBlender blender = blit_get_blender_by_pixelformat( st_dstat.pixelFormat );
	
	if( color == BLIT_TRANSPARENT ) return;
	
	if( sx > 512 ) sx = 512;
	if( ex > 512 ) ex = 512;
	if( sy > 272 ) sy = 272;
	if( ey > 272 ) ey = 272;
	
	for( cy = sy; cy <= ey; cy++ ){
		offset = ( sx + cy * st_dstat.bufferWidth ) * st_pxlen;
		for( cx = sx; cx < ex; cx++, offset += st_pxlen ){
			addr = (void *)((unsigned int)(st_dstat.frameBuffer) + offset);
			blend_color = blender ? ( blender )( &color, addr ) : color;
			memcpy( addr, (void *)&blend_color, st_pxlen );
		}
	}
}

/* bresenhamの線分描画アルゴリズム */
void blitLine( unsigned int sx, unsigned int sy, unsigned int ex, unsigned int ey, uint32_t color )
{
	unsigned int dx, dy, e = 0;
	void *addr;
	uint32_t blend_color;
	BlitAlphaBlender blender = blit_get_blender_by_pixelformat( st_dstat.pixelFormat );
	
	if( color == BLIT_TRANSPARENT ) return;
	
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
			addr = (void *)((unsigned int)(st_dstat.frameBuffer) + ( ( x + ( y * st_dstat.bufferWidth ) ) * st_pxlen ));
			blend_color = blender ? ( blender )( &color, addr ) : color;
			memcpy( addr, (void *)&blend_color, st_pxlen );
		}
	} else{
		unsigned int x, y;
		for( x = sx, y = sy; y <= ey; y++ ){
			e += dx;
			if( e > dy ){
				e -= dy;
				x++;
			}
			addr = (void *)((unsigned int)(st_dstat.frameBuffer) + ( ( x + ( y * st_dstat.bufferWidth ) ) * st_pxlen ));
			blend_color = blender ? ( blender )( &color, addr ) : color;
			memcpy( addr, (void *)&blend_color, st_pxlen );
		}
	}
}

void blitLineRect( unsigned int sx, unsigned int sy, unsigned int ex, unsigned int ey, uint32_t color )
{
	if( color == BLIT_TRANSPARENT ) return;
	
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

int blitStringvf( unsigned int sx, unsigned int sy, uint32_t fgcolor, uint32_t bgcolor, const char *format, va_list ap )
{
	char str[255];
	
	vsnprintf( str, 255, format, ap );
	return blitString( sx, sy, fgcolor, bgcolor, str );
}

int blitStringf( unsigned int sx, unsigned int sy, uint32_t fgcolor, uint32_t bgcolor, const char *format, ... )
{
	unsigned int c;
	va_list ap;
	
	va_start( ap, format );
	c = blitStringvf( sx, sy, fgcolor, bgcolor, format, ap );
	va_end( ap );
	
	return c;
}
