/* pbApply.c */

#include "pb_types.h"

static unsigned int pb_color_8888_5650( unsigned int color, void *null )
{
	register unsigned char r, g, b;
	r = ( color >> 3  ) & 0x1F;
	g = ( color >> 10 ) & 0x3F;
	b = ( color >> 19 ) & 0x1F;
	return PB_COLOR_JOIN_5650( r, g, b );
}

static unsigned int pb_color_8888_5551( unsigned int color, void *null )
{
	register unsigned char r, g, b, a;
	r = ( color >> 3  ) & 0x1F;
	g = ( color >> 11 ) & 0x1F;
	b = ( color >> 19 ) & 0x1F;
	a = ( color >> 24 ) ? 1 : 0;
	return PB_COLOR_JOIN_5551( r, g, b, a );
}

static unsigned int pb_color_8888_4444( unsigned int color, void *null )
{
	register unsigned char r, g, b, a;
	r = ( color >> 4  ) & 0xF;
	g = ( color >> 12 ) & 0xF;
	b = ( color >> 20 ) & 0xF;
	a = ( color >> 28 ) & 0xF;
	return PB_COLOR_JOIN_4444( r, g, b, a );
}

static unsigned int pb_color_blend_8888( unsigned int src, void *dst_addr )
{
	uint8_t      alpha = ( src >> 24 ) & 0xFF;
	unsigned int dst   = *((uint32_t *)dst_addr);
	
	if( ! alpha ){
		return dst;
	} else if( alpha == 0xFF ){
		return src;
	} else{
		uint32_t color = alpha << 24;
		color |= PB_BLEND_FUNC( ( src       ) & 0xFF, ( dst       ) & 0xFF, alpha, 8 );
		color |= PB_BLEND_FUNC( ( src >>  8 ) & 0xFF, ( dst >>  8 ) & 0xFF, alpha, 8 ) << 8;
		color |= PB_BLEND_FUNC( ( src >> 16 ) & 0xFF, ( dst >> 16 ) & 0xFF, alpha, 8 ) << 16;
		return color;
	}
}

static unsigned int pb_color_blend_4444( unsigned int src, void *dst_addr )
{
	uint8_t  alpha = ( src >> 12 ) & 0xF;
	uint16_t dst   = *((uint32_t *)dst_addr);
	
	src = pb_color_8888_4444( src, NULL );
	
	if( ! alpha ){
		return dst;
	} else if( alpha == 0xF ){
		return src;
	} else{
		uint16_t color = alpha << 12;
		color |= PB_BLEND_FUNC( ( src      ) & 0xF, ( dst      ) & 0xF, alpha, 4 );
		color |= PB_BLEND_FUNC( ( src >> 4 ) & 0xF, ( dst >> 4 ) & 0xF, alpha, 4 ) << 4;
		color |= PB_BLEND_FUNC( ( src >> 8 ) & 0xF, ( dst >> 4 ) & 0xF, alpha, 4 ) << 8;
		return color;
	}
}

static unsigned int pb_color_blend_5551( unsigned int src, void *dst_addr )
{
	uint8_t  alpha = ( src >> 27 ) & 0x1F;
	uint16_t dst   = *((uint32_t *)dst_addr);
	
	src = pb_color_8888_5551( src, NULL );
	
	if( ! alpha ){
		return dst;
	} else if( alpha == 0x1F ){
		return src;
	} else{
		uint16_t color = ( alpha ? 1 : 0 ) << 15;
		color |= PB_BLEND_FUNC( ( src       ) & 0x1F, ( dst       ) & 0x1F, alpha, 5 );
		color |= PB_BLEND_FUNC( ( src >>  5 ) & 0x1F, ( dst >>  5 ) & 0x1F, alpha, 5 ) << 5;
		color |= PB_BLEND_FUNC( ( src >> 10 ) & 0x1F, ( dst >> 10 ) & 0x1F, alpha, 5 ) << 10;
		return color;
	}
}

static unsigned int pb_color_blend_5650( unsigned int src, void *dst_addr )
{
	uint8_t  alpha = ( src >> 26 ) & 0x3F;
	uint16_t dst   = *((uint32_t *)dst_addr);
	
	src = pb_color_8888_5650( src, NULL );
	
	if( ! alpha ){
		return dst;
	} else if( alpha == 0x3F ){
		return src;
	} else{
		uint16_t color = 0;
		color |= PB_BLEND_FUNC( ( src       ) & 0x1F, ( dst       ) & 0x1F, ( alpha >> 1 ) & 0x1F, 5 );
		color |= PB_BLEND_FUNC( ( src >>  5 ) & 0x3F, ( dst >>  5 ) & 0x3F, alpha                , 6 ) << 5;
		color |= PB_BLEND_FUNC( ( src >> 11 ) & 0x1F, ( dst >> 11 ) & 0x1F, ( alpha >> 1 ) & 0x1F, 5 ) << 11;
		return color;
	}
}

static color_convert pb_get_color_converter( int format )
{
	switch( format ){
		case PB_PXF_4444:
			return pb_color_8888_4444;
		case PB_PXF_5551:
			return pb_color_8888_5551;
		case PB_PXF_5650:
			return pb_color_8888_5650;
		default:
			return NULL;
	}
}

static color_convert pb_get_color_blender( int format )
{
	switch( format ){
		case PB_PXF_8888:
			return pb_color_blend_8888;
		case PB_PXF_4444:
			return pb_color_blend_4444;
		case PB_PXF_5551:
			return pb_color_blend_5551;
		case PB_PXF_5650:
			return pb_color_blend_5650;
		default:
			return NULL;
	}
}

static void pb_set_framebuf_conf( struct pb_frame_buffer *fb, unsigned int opt )
{
	fb->pixelSize = pbGetPixelDataSize( fb->format );
	fb->lineSize = fb->width * fb->pixelSize;
	if( opt & PB_NO_CACHE ){
		fb->addr = (void *)( (uintptr_t)fb->addr | PB_DISABLE_CACHE );
	} else{
		fb->addr = (void *)( (uintptr_t)fb->addr & ~PB_DISABLE_CACHE );
	}
	if( opt & PB_BLEND ){
		fb->colorConv = pb_get_color_blender( fb->format );
	} else{
		fb->colorConv = pb_get_color_converter(fb->format );
	}
}

int pbApply( void )
{
	if( ! __pb_internal_params.display->addr ){
		return CG_ERROR_INVALID_ARGUMENT;
	} else{
		pb_set_framebuf_conf( __pb_internal_params.display, __pb_internal_params.options );
	}
	
	if( __pb_internal_params.options & PB_DOUBLE_BUFFER ){
		pb_set_framebuf_conf( __pb_internal_params.draw, __pb_internal_params.options );
	} else{
		*(__pb_internal_params.draw) = *(__pb_internal_params.display);
	}
	
	return 0;
}
