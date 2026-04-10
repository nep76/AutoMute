/* strutilCounterPbrk */

#include "../strutil.h"

char *strutilCounterPbrk( const char *__restrict__ src, const char *__restrict__ search )
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
