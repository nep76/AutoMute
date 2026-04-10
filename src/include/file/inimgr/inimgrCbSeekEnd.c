/* inimgrCbSeekEnd.c */

#include "inimgr_types.h"

int inimgrCbSeekEnd( InimgrCallbackParams *params, SceOff offset )
{
	SceOff endpos = params->cbinfo->offset + params->cbinfo->length;
	
	if( offset < 0 ){
		if( abs( offset ) <= params->cbinfo->length ){
			return fiomgrSeek( params->ini, endpos + offset, FH_SEEK_SET );
		} else{
			return fiomgrSeek( params->ini, params->cbinfo->offset, FH_SEEK_SET );
		}
	} else{
		return fiomgrSeek( params->ini, endpos, FH_SEEK_SET );
	}
}
