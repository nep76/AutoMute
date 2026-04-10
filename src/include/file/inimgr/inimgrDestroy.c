/* inimgrDestroy.c */

#include "inimgr_types.h"

void inimgrDestroy( IniUID uid )
{
	struct inimgr_params *params = (struct inimgr_params *)uid;
	
	if( ! params ) return;
	
	dmemDestroy( params->dmem );
}

