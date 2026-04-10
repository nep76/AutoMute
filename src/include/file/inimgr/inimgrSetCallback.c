/* inimgrSetCallback.c */

#include "inimgr_types.h"

int inimgrSetCallback( IniUID uid, const char *section, InimgrCallback cb, void *arg )
{
	unsigned short section_len;
	struct inimgr_params   *params = (struct inimgr_params *)uid;
	struct inimgr_callback *new_cb;
	
	if( ! section || *section == '\0' ) section = INIMGR_DEFAULT_SECTION_NAME;
	section_len = strlen( section );
	
	if( section_len + 1 > INIMGR_SECTION_BUFFER ) return CG_ERROR_OUT_OF_BUFFER;
	
	new_cb = (struct inimgr_callback *)dmemAlloc( params->dmem, sizeof( struct inimgr_callback ) + section_len + 1 );
	if( ! new_cb ) return CG_ERROR_NOT_ENOUGH_MEMORY;
	
	new_cb->section = (char *)( (uintptr_t)new_cb + sizeof( struct inimgr_callback ) );
	
	strcpy( new_cb->section, section );
	
	if( new_cb->section[section_len - 1] == '*' ){
		new_cb->cmplen = section_len - 1;
	} else{
		new_cb->cmplen = -1;
	}
	new_cb->cb       = cb;
	new_cb->arg      = arg;
	new_cb->infoList = NULL;
	
	if( params->callbacks ){
		struct inimgr_callback *last_cb;
		for( last_cb = params->callbacks; last_cb->next; last_cb = last_cb->next );
		last_cb->next = new_cb;
		new_cb->next  = NULL;
	} else{
		new_cb->next = NULL;
		params->callbacks = new_cb;
	}
	
	return CG_ERROR_OK;
}
