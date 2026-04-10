/* inimgrDeleteEntry.c */

#include "inimgr_types.h"

void inimgrDeleteEntry( IniUID uid, const char *section, const char *key )
{
	struct inimgr_section *sectionptr;
	
	if( ! section || ! key ) return;
	
	sectionptr = __inimgr_find_section( (struct inimgr_params *)uid, section );
	__inimgr_delete_entry( (struct inimgr_params *)uid, sectionptr, __inimgr_find_entry( sectionptr, key ) );
}
