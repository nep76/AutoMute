/* inimgrCbSeekCur.c */

#include "inimgr_types.h"

int inimgrCbSeekCur( InimgrCallbackParams *params, SceOff offset )
{
	SceOff pos = fiomgrTell( params->ini ) - params->cbinfo->offset;
	
	if( pos < 0 ){
		pos = 0;
	} else if( pos > params->cbinfo->length ){
		pos = params->cbinfo->length;
	}
	
	pos += offset;
	
	if( pos < 0 ){
		pos = 0;
	} else if( pos > params->cbinfo->length ){
		pos = params->cbinfo->length;
	}
	
	return fiomgrSeek( params->ini, params->cbinfo->offset + pos, FH_SEEK_SET );
}
