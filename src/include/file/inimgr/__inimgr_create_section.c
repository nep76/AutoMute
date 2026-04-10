/* __inimgr_create_section.c */

#include "inimgr_types.h"

struct inimgr_section *__inimgr_create_section( struct inimgr_params *params, const char *section )
{
	struct inimgr_section *new_section;
	size_t section_buf = strlen( section ) + 1;
	
	if( section_buf > INIMGR_SECTION_BUFFER ) return NULL;
	
	new_section = (struct inimgr_section *)dmemAlloc( params->dmem, sizeof( struct inimgr_section ) + section_buf );
	if( ! new_section ) return NULL;
	
	new_section->name = (char *)( (uintptr_t)new_section + sizeof( struct inimgr_section ) );
	strcpy( new_section->name, section );
	new_section->entry = NULL;
	new_section->prev  = NULL;
	new_section->next  = NULL;
	
	return new_section;
}
