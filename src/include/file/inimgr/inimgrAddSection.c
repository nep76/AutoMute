/* inimgrAddSection.c */

#include "inimgr_types.h"

int inimgrAddSection( IniUID uid, const char *section )
{
	struct inimgr_params  *params = (struct inimgr_params *)uid;
	struct inimgr_section *last_section;
	struct inimgr_section *new_section;
	size_t section_buf;
	
	if( ! section || strcasecmp( section, INIMGR_DEFAULT_SECTION_NAME ) == 0 ) return CG_ERROR_INVALID_ARGUMENT;
	
	section_buf = strlen( section ) + 1;
	if( section_buf > INIMGR_SECTION_BUFFER ) return CG_ERROR_OUT_OF_BUFFER;
	
	for( last_section = ((struct inimgr_params *)uid)->index; last_section->next; last_section = last_section->next );
	
	new_section = __inimgr_create_section( params, section );
	if( ! new_section ) return CG_ERROR_NOT_ENOUGH_MEMORY;
	
	new_section->name = (char *)( (uintptr_t)new_section + sizeof( struct inimgr_section ) );
	new_section->prev  = last_section;
	last_section->next = new_section;
	
	return CG_ERROR_OK;
}
