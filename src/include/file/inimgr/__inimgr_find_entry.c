/* __inimgr_find_entry.c */

#include "inimgr_types.h"

struct inimgr_entry *__inimgr_find_entry( struct inimgr_section *section, const char *key )
{
	struct inimgr_entry *current_entry;
	
	if( ! section ) return NULL;
	
	for( current_entry = section->entry; current_entry; current_entry = current_entry->next ){
		if( strcasecmp( current_entry->key, key ) == 0 ) break;
	}
	return current_entry;
}
