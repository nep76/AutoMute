/* __inimgr_get_value.c */

#include "inimgr_types.h"

char *__inimgr_get_value( struct inimgr_params *params, const char *section, const char *key )
{
	struct inimgr_section *target_section;
	
	if( ! params  ) return NULL;
	if( ! section ) section = INIMGR_DEFAULT_SECTION_NAME;
	
	target_section = __inimgr_find_section( params, section );
	
	if( target_section ){
		struct inimgr_entry *target_entry = __inimgr_find_entry( target_section, key );
		if( target_entry ){
			return target_entry->value;
		}
	}
	
	return NULL;
}
