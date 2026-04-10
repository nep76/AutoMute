/* inimgrExistSection.c */

#include "inimgr_types.h"

bool inimgrExistSection( IniUID uid, const char *section )
{
	if( ! section || strcasecmp( section, INIMGR_DEFAULT_SECTION_NAME ) == 0 || __inimgr_find_section( (struct inimgr_params *)uid, section ) ) return true;
	return false;
}
