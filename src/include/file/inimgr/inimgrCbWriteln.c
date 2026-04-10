/* inimgrCbWriteln.c */

#include "inimgr_types.h"

int inimgrCbWriteln( InimgrCallbackParams *params, char *buf, size_t buflen )
{
	return fiomgrWriteln( params->ini, buf, buflen );
}
