/* dmemNew.c */

#include "dmem_types.h"

DmemUID dmemNew( size_t minblock, int type )
{
	struct dmem_root *root;
	
	if( type != PSP_SMEM_High ) type = PSP_SMEM_Low;
	
	root = memoryAllocEx( "DmemParams", MEMORY_USER, 0, sizeof( struct dmem_root ), type, NULL );
	if( ! root ) return 0;
	
	root->minHeapSize = minblock > 0 ? minblock : DMEM_DEFAULT_MINBLOCK;
	root->allocType   = type;
	root->heapList    = NULL;
	root->lastUse     = NULL;
	
	return (DmemUID)root;
}
