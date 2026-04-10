/* __inimgr_find_section.c */

#include "inimgr_types.h"

struct inimgr_section *__inimgr_find_section( struct inimgr_params *params, const char *section )
{
	struct inimgr_section *current_section;
	
	if( params->last && strcasecmp( params->last->name, section ) == 0 ) return params->last;
	
	for( current_section = params->index; current_section; current_section = current_section->next ){
		if( strcasecmp( current_section->name, section ) == 0 ){
			params->last = current_section;
			break;
		}
	}
	return current_section;
}
