/* __inimgr_delete_section.c */

#include "inimgr_types.h"

void __inimgr_delete_section( struct inimgr_params *params, struct inimgr_section *section )
{
	struct inimgr_entry *current_entry, *next_entry;
	
	if( ! section ) return;
	
	/* sectionをインデックスから切り離す */
	if( section->prev ) section->prev->next = section->next;
	if( section->next ) section->next->prev = section->prev;
	if( params->index == section ) params->index = section->next;
	if( params->last  == section ) params->last  = NULL;
	
	for( current_entry = section->entry; current_entry; current_entry = next_entry ){
		next_entry = current_entry->next;
		__inimgr_delete_entry( params, section, current_entry );
	}
	
	dmemFree( params->dmem, section );
}
