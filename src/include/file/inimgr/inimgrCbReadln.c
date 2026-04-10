/* inimgrCbReadln.c */

#include "inimgr_types.h"

int inimgrCbReadln( InimgrCallbackParams *params, char *buf, size_t buflen )
{
	if( fiomgrTell( params->ini ) >= ( params->cbinfo->offset + params->cbinfo->length ) ) return 0;
	return fiomgrReadln( params->ini, buf, buflen );
}
