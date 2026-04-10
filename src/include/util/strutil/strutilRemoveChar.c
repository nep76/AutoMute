/* strutilRemoveChar */

#include "../strutil.h"

int strutilRemoveChar( char *__restrict__ str, const char *__restrict__ search )
{
	int count      = 0;
	int offset     = 0;
	int ins_offset = 0;
	
	while( str[offset] != '\0' ){
		if( ! strchr( search, str[offset] )  ){
			str[ins_offset++] = str[offset];
			count++;
		}
		offset++;
	}
	
	str[ins_offset] = str[offset];
	
	return count;
}
