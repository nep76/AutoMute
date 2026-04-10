/* __inimgr_find_callback.c */

#include "inimgr_types.h"

struct inimgr_callback *__inimgr_find_callback( struct inimgr_params *params, const char *section )
{
	struct inimgr_callback *current_callback;
	
	for( current_callback = params->callbacks; current_callback; current_callback = current_callback->next ){
		if(
			( current_callback->cmplen < 0 && strcasecmp( current_callback->section, section ) == 0 ) ||
			( current_callback->cmplen == 0 ) ||
			( strncasecmp( current_callback->section, section, current_callback->cmplen ) == 0 )
		){
			break;
		}
	}
	
	return current_callback;
}
