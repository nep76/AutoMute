/* inimgrParseEntry.c */

#include "inimgr_types.h"

bool inimgrParseEntry( char *entry, char **key, char **value )
{
	char *delim = strchr( entry, '=' );
	if( ! delim ) return false;
	
	*delim = '\0';
	*key   = entry;
	*value = delim + 1;
	
	delim = strutilCounterReversePbrk( *key, "\t\x20" );
	if( delim ){
		*(delim + 1) = '\0';
	}
	
	delim = strutilCounterPbrk( *value, "\t\x20" );
	if( delim ){
		*value = delim;
	}
	
	return true;
}
