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

char *strutilCounterPbrk( const char *src, const char *search )
{
	bool find = false;
	
	if( search[0] == '\0' ) return NULL;
	
	for( ; *src != '\0'; src++ ){
		for( ; *search != '\0'; search++ ){
			if( *src == *search ){
				find = true;
				break;
			}
		}
		if( find ){
			find = false;
		} else{
			break;
		}
	}
	
	if( *src == '\0' ){
		return NULL;
	} else{
		return (char *)src;
	}
}

char *strutilCounterReversePbrk( const char *src, const char *search )
{
	int src_index, search_index, search_len;
	bool find = false;
	
	if( search[0] == '\0' ) return NULL;
	
	search_len = strlen( search );
	for( src_index = strlen( src ); src_index >= 0; src_index-- ){
		for( search_index = search_len; search_index >= 0; search_index-- ){
			if( src[src_index] == search[search_index] ){
				find = true;
				break;
			}
		}
		if( find ){
			find = false;
		} else{
			break;
		}
	}
	
	if( src[src_index] == '\0' ){
		return NULL;
	} else{
		return (char *)(&src[src_index]);
	}
}

void strutilRemoveChar( char *str, const char *search )
{
	int offset     = 0;
	int ins_offset = 0;
	
	while( str[offset] != '\0' ){
		if( ! strchr( search, str[offset] )  ){
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
