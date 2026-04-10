/* __inimgr_set_value.c */

#include "inimgr_types.h"

int __inimgr_set_value( struct inimgr_params *params, const char *section, const char *key, const char *value )
{
	struct inimgr_section *target_section;
	struct inimgr_entry   *target_entry;
	
	if( ! params ) return CG_ERROR_INVALID_ARGUMENT;
	if( ! section ) section = INIMGR_DEFAULT_SECTION_NAME;
	
	target_section = __inimgr_find_section( params, section );
	if( ! target_section ){
		int ret = inimgrAddSection( (IniUID)params, section );
		if( ret < 0 ) return ret;
		target_section = __inimgr_find_section( params, section );
	}
	
	target_entry = __inimgr_find_entry( target_section, key );
	if( ! target_entry ){
		struct inimgr_entry *last_entry;
		
		target_entry = __inimgr_create_entry( params, key, value );
		if( ! target_entry ) return CG_ERROR_NOT_ENOUGH_MEMORY;
		
		if( ! target_section->entry ){
			target_section->entry = target_entry;
		} else{
			for( last_entry = target_section->entry; last_entry->next; last_entry = last_entry->next );
			last_entry->next = target_entry;
			target_entry->prev = last_entry;
		}
	} else{
		strutilCopy( target_entry->value, value, target_entry->vspace );
	}
	
	return CG_ERROR_OK;
}
