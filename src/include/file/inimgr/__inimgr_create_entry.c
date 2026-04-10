/* __inimgr_create_entry.c */

#include "inimgr_types.h"

struct inimgr_entry *__inimgr_create_entry( struct inimgr_params *params, const char *key, const char *value )
{
	int keylen;
	struct inimgr_entry *new_entry = (struct inimgr_entry *)dmemAlloc( params->dmem, sizeof( struct inimgr_entry ) );
	if( ! new_entry ) return NULL;
	
	new_entry->key = (char *)dmemAlloc( params->dmem, INIMGR_ENTRY_BUFFER );
	if( ! new_entry->key ) return NULL;
	keylen = strlen( key );
	
	new_entry->value  = new_entry->key + keylen + 1;
	new_entry->vspace = INIMGR_ENTRY_BUFFER - ( keylen + 1 );
	new_entry->prev   = NULL;
	new_entry->next   = NULL;
	
	strcpy( new_entry->key, key );
	strutilCopy( new_entry->value, value, new_entry->vspace );
	
	return new_entry;
}
