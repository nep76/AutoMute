/* __inimgr_delete_entry.c */

#include "inimgr_types.h"

void __inimgr_delete_entry( struct inimgr_params *params, struct inimgr_section *section, struct inimgr_entry *entry )
{
	if( ! entry ) return;
	
	/* entryをインデックスから切り離す */
	if( entry->prev ) entry->prev->next = entry->next;
	if( entry->next ) entry->next->prev = entry->prev;
	if( section->entry == entry ) section->entry = entry->next;
	
	dmemFree( params->dmem, entry->key );
	dmemFree( params->dmem, entry );
}
