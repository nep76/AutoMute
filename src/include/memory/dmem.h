/*=========================================================
	
	dmem.h
	
	動的メモリ管理。
	ヒープを数珠上に繋いで、物理メモリが許す限りのメモリを動的に確保する。
	自動化している都合上オーバーヘッドが大きいので、
	サイズが分かっている場合は、heap.hやmemory.hで直接確保する方がよい。
	
	伸び縮みするリンクリストなどで有効。
	
	*メモリが断片化した際の対策がまだない*
	
=========================================================*/
#ifndef DMEM_H
#define DMEM_H

#include "memory/heap.h"

/*=========================================================
	マクロ
=========================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*=========================================================
	型宣言
=========================================================*/
typedef uintptr_t DmemUID;

/*=========================================================
	関数
=========================================================*/
DmemUID dmemNew( size_t minblock, int type );
void *dmemAlloc( DmemUID uid, size_t size );
void *dmemCalloc( DmemUID uid, size_t nelem, size_t size );
int dmemFree( DmemUID uid, void *ptr );
void dmemDestroy( DmemUID uid );

#ifdef __cplusplus
}
#endif

#endif
