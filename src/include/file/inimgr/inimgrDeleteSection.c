/* inimgrDeleteSection.c */

#include "inimgr_types.h"

void inimgrDeleteSection( IniUID uid, const char *section )
{
	/* デフォルトセクションは削除させない */
	if( ! section || strcasecmp( section, INIMGR_DEFAULT_SECTION_NAME ) == 0 ) return;
	
	__inimgr_delete_section( (struct inimgr_params *)uid, __inimgr_find_section( (struct inimgr_params *)uid, section ) );
}
