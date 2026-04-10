/* pbLine.c */

#include "pb_types.h"

void pbLine( int sx, int sy, int ex, int ey, uint32_t color )
{
	uintptr_t draw_addr;
	int       e, dx, dy;
	
	if( color == PB_TRANSPARENT || __pb_internal_params.options & PB_NO_DRAW ) return;
	
	if( sx > ex ) PB_SWAP( &sx, &ex );
	if( sy > ey ) PB_SWAP( &sy, &ey );
	
	draw_addr = (uintptr_t)__pb_buf_addr( __pb_internal_params.draw, sx, sy );
	
	if( sx == ex ){
		for( ; sy <= ey; sy++, draw_addr += __pb_internal_params.draw->lineSize ) PB_PUT_PIXEL( draw_addr, color );
	} else if( sy == ey ){
		for( ; sx <= ex; sx++, draw_addr += __pb_internal_params.draw->pixelSize ) PB_PUT_PIXEL( draw_addr, color );
	} else{
		/* ƒuƒŒƒ[ƒ“ƒnƒ€‚Ìü•ª•`‰æƒAƒ‹ƒSƒŠƒYƒ€ */
		e  = 0;
		dx = ex - sx;
		dy = ey - sy;
		
		if( dx > dy ){
			int x;
			for( x = sx; x <= ex; x++, draw_addr += __pb_internal_params.draw->pixelSize ){
				e += dy;
				if( e > dx ){
					e -= dx;
					draw_addr += __pb_internal_params.draw->lineSize;
				}
				PB_PUT_PIXEL( draw_addr, color );
			}
		} else{
			int y;
			for( y = sy; y <= ey; y++, draw_addr += __pb_internal_params.draw->lineSize ){
				e += dx;
				if( e > dy ){
					e -= dy;
					draw_addr += __pb_internal_params.draw->pixelSize;
				}
				PB_PUT_PIXEL( draw_addr, color );
			}
		}
	}
}
