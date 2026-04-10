/* inimgrCbTell.c */

#include "inimgr_types.h"

int inimgrCbTell( InimgrCallbackParams *params )
{
	return params->cbinfo->offset - fiomgrTell( params->ini );
}
