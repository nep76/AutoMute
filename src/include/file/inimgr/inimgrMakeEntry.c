/* inimgrMakeEntry.c */

#include "inimgr_types.h"

int inimgrMakeEntry( char *buf, size_t len, char *key, char *value )
{
	int ret = snprintf( buf, len, "%s = %s", key, value );
	
	len--;
	return ( ret > len ? len : ret );
}
