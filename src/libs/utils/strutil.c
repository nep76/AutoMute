/*
	strutil.c
*/

#include "strutil.h"

char *strutilSafeCopy( char *dest, const char *src, size_t max )
{
	if( max <= 0 ) return NULL;
	
	max--;
	while( max-- > 0 && *src != '\0' ) *dest++ = *src++;
	
	*dest = '\0';
	
	return dest;
}

char *strutilSafeCat( char *dest, const char *src, size_t max )
{
	size_t dest_len = strlen( dest );
	char *retp = dest;
	
	max -= dest_len;
	if( max <= 0 ) return NULL;
	
	dest += dest_len;
	
	strutilSafeCopy( dest, src, max );
	
	return retp;
}

void strutilRemoveChar( char *str, const char *target )
{
	int offset     = 0;
	int ins_offset = 0;
	
	while( str[offset] != '\0' ){
		if( ! strchr( target, str[offset] )  ){
			str[ins_offset++] = str[offset];
		}
		offset++;
	}
	
	str[ins_offset] = str[offset];
}

char *strutilToUpper( char *str )
{
	int i;
	
	for( i = 0; str[i]; i++ ) str[i] = toupper( str[i] );
	
	return str;
}

char *strutilToLower( char *str )
{
	int i;
	
	for( i = 0; str[i]; i++ ) str[i] = tolower( str[i] );
	
	return str;
}

char *strutilToUpperFirst( char *str )
{
	str[0] = toupper( str[0] );
	return str;
}

char *strutilToLowerFirst( char *str )
{
	str[0] = tolower( str[0] );
	return str;
}
