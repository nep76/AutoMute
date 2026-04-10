/* strutilCounterReversePbrk */

#include "../strutil.h"

char *strutilCounterReversePbrk( const char *__restrict__ src, const char *__restrict__ search )
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
