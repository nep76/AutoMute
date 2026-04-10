/* inimgrMakeSection.c */

#include "inimgr_types.h"

int inimgrMakeSection( char *buf, size_t len, char *section )
{
	int ret = snprintf( buf, len, "[%s]", section );
	
	len--;
	return ( ret > len ? len : ret );
}
