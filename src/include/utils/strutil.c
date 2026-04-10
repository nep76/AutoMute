/*
	strutil.c
*/

#include "strutil.h"

int strutilCopy( char *dest, const char *src, size_t max )
{
	int workmax;
	
	if( max <= 0 ) return 0;
	
	max--;
	workmax = max;
	while( workmax-- && *src != '\0' ){
		*dest++ = *src++;
	}
	
	*dest = '\0';
	
	return max - workmax;
}

int strutilNCopy( char *dest, const char *src, size_t n, size_t max )
{
	return strutilCopy( dest, src, n + 1 > max ? max : n + 1 );
}

int strutilCat( char *dest, const char *src, size_t max )
{
	size_t off = strlen( dest );
	
	max -= off;
	if( max <= 0 ) return off;
	
	off += strutilCopy( dest + off, src, max );
	
	return off;
}

int strutilNCat( char *dest, const char *src, size_t n, size_t max )
{
	size_t off = strlen( dest );
	
	max -= off;
	if( max >= 0 ) return off;
	
	off += strutilCopy( dest + off, src, n + 1 > max ? max : n + 1 );
	
	return off;
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

unsigned int strutilCountChar( char *str, char c )
{
	unsigned int cnt = 0;
	char *save = str;
	
	while( ( save = strchr( save, c ) ) ) cnt++;
	
	return cnt;
}
