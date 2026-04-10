/* inimgrCbSeekSet.c */

#include "inimgr_types.h"

int inimgrCbSeekSet( InimgrCallbackParams *params, SceOff offset )
{
	if( offset > 0 ){
		if( offset <= params->cbinfo->length ){
			return fiomgrSeek( params->ini, params->cbinfo->offset + offset, FH_SEEK_SET );
		} else{
			return fiomgrSeek( params->ini, params->cbinfo->offset + params->cbinfo->length, FH_SEEK_SET );
		}
	} else{
		return fiomgrSeek( params->ini, params->cbinfo->offset, FH_SEEK_SET );
	}
}
