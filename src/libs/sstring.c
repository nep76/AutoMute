/* sstring.h */

#include "sstring.h"

char *safe_strncpy( char *targ, const char *orig, size_t n )
{
	if( n <= 0 ) return NULL;
	
	n--;
	while( n-- && *orig != '\0' ) *targ++ = *orig++;
	
	*targ = '\0';
	
	return targ;
}

char *safe_strncat( char *targ, const char *orig, size_t n )
{
	size_t targ_len = strlen( targ );
	char *retp = targ;
	
	n -= targ_len;
	if( n <= 0 ) return NULL;
	
	targ += targ_len;
	
	safe_strncpy( targ, orig, n );
	
	return retp;
}

