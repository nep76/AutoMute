/* pb_types.h                     */
/* 内部で使用。外から参照しない。 */

#include "../pb.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "cgerrs.h"

#define PB_SWAP( a, b ) { \
	int t = *(int *)a; \
	*(int *)a = *(int *)b; \
	*(int *)b = t; \
}

#define PB_BLEND_FUNC( src_ch, dst_ch, alpha, maxbits ) ( ( ( ( src_ch ) + 1 - ( dst_ch ) ) * ( alpha ) >> ( maxbits ) ) + ( dst_ch ) )

#define PB_COLOR_EXTRACT_8888( c, r, g, b, a ) \
	r = ( c ) & 0xFF; \
	g = ( ( c ) >>  8 ) & 0xFF; \
	b = ( ( c ) >> 16 ) & 0xFF; \
	a = ( ( c ) >> 24 ) & 0xFF;

#define PB_COLOR_EXTRACT_4444( c, r, g, b, a ) \
	r = ( c ) & 0xF; \
	g = ( ( c ) >>  4 ) & 0xF; \
	b = ( ( c ) >>  8 ) & 0xF; \
	a = ( ( c ) >> 12 ) & 0xF;

#define PB_COLOR_EXTRACT_5551( c, r, g, b, a ) \
	r = ( c ) & 0x1F; \
	g = ( ( c ) >> 5 ) & 0x1F; \
	b = ( ( c ) >> 10 ) & 0x1F; \
	a = ( ( c ) >> 15 ) & 0x1 ? 1 : 0;

#define PB_COLOR_EXTRACT_5650( c, r, g, b ) \
	r = ( c ) & 0x1F; \
	g = ( ( c ) >>  5 ) & 0x3F; \
	b = ( ( c ) >> 11 ) & 0x1F;

#define PB_COLOR_JOIN_8888( r, g, b, a ) ( (unsigned int)( r ) | (unsigned int)( g ) << 8 | (unsigned int)( b ) << 16 | (unsigned int)( a ) << 24 )
#define PB_COLOR_JOIN_4444( r, g, b, a ) ( (unsigned int)( r ) | (unsigned int)( g ) << 4 | (unsigned int)( b ) <<  8 | (unsigned int)( a ) << 12 )
#define PB_COLOR_JOIN_5551( r, g, b, a ) ( (unsigned int)( r ) | (unsigned int)( g ) << 5 | (unsigned int)( b ) << 10 | (unsigned int)( a ) << 15 )
#define PB_COLOR_JOIN_5650( r, g, b )    ( (unsigned int)( r ) | (unsigned int)( g ) << 5 | (unsigned int)( b ) << 11 )

#define PB_PUT_PIXEL( a, c ) { \
	unsigned int pc = c; \
	if( __pb_internal_params.draw->colorConv ) pc = ( __pb_internal_params.draw->colorConv )( pc, (void *)a ); \
	memcpy( (void *)a, (void *)&pc, __pb_internal_params.draw->pixelSize ); \
}

typedef unsigned int ( *color_convert  )( unsigned int, void* );

enum pb_blend_target {
	PB_BLEND_SRC,
	PB_BLEND_DST
};

struct pb_frame_buffer {
	int  format;
	void *addr;
	int  width;
	
	unsigned char pixelSize;
	unsigned int  lineSize;
	
	color_convert   colorConv;
};

struct pb_params {
	struct pb_frame_buffer frame0, frame1;
	struct pb_frame_buffer *display, *draw;
	unsigned int   options;
	unsigned int   blendFactor;
	unsigned int   linebreakWidth;
};

extern struct pb_params __pb_internal_params;

void *__pb_buf_addr( struct pb_frame_buffer *fb, int x, int y );
