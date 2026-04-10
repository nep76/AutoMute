/* pbSync.c */

#include "pb_types.h"

int pbSync( int bufsync )
{
	return sceDisplaySetFrameBuf( __pb_internal_params.display->addr, __pb_internal_params.display->width, __pb_internal_params.display->format, bufsync );
}
