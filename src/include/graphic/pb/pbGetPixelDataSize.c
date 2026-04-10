/* pbGetPixelDataSize.c */

#include "pb_types.h"

int pbGetPixelDataSize( int format )
{
	switch( format ){
		case PB_PXF_8888:
			return 4;
		default:
			return 2;
	}
}
