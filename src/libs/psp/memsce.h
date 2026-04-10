/*
	memsce.h
	
	sceKernelAllocPartitionMemory()
	sceKernelFreePartitionMemory()
	を使ってメモリを割り当てる。
	
	これによって、sceKernelTotalFreeMemSize()やsceKernelMaxFreeMemSize()が
	使用量に応じた値を返す。
	
	EBOOT.PBPを作成する場合、PSP_HEAP_SIZE_KB()にてPSPSDKのライブラリが
	malloc()用に確保するメモリを減らしておかないと、確保できる容量が減る。
*/

#ifndef MEMSCE_H
#define MEMSCE_H

#include <pspkernel.h>
#include <pspkerneltypes.h>
#include <string.h>

#define MEMSCE_POWER_OF_TWO( x ) ( ! ( x & ( x - 1 ) ) )
#define MEMSCE_ALIGNOFFSET(x, align) (((x)+((align)-1))&~((align)-1))

#define MEMSCE_PART_KERNEL_HIGH        1
#define MEMSCE_PART_USER               2
#define MEMSCE_PART_KERNEL_HIGH_MIRROR 3
#define MEMSCE_PART_KERNEL_LOW         4
#define MEMSCE_PART_VOLATILE           5
#define MEMSCE_PART_USER_MIRROR        6

#ifdef __cplusplus
extern "C" {
#endif

struct heap_header {
	SceUID	block_id;
	unsigned int size;
};

/* MemSceMalloc
 *
 *	標準モジュールのmallocもどき。
 *
 *	@param: SceSize size
 *		割り当てるメモリサイズ。
 *
 *	@returns:
 *		割り当てたメモリの先頭アドレス。
 */
void *memsceMalloc( SceSize size );

/* MemSceCalloc
 *
 *	標準モジュールのcallocもどき。
 *	MemSceMalloc()後に割り当てたメモリをすべてゼロでクリアする。
 *
 *	@param: SceSize blk_num
 *		割り当てるブロックの数。
 *
 *	@param: SceSize size
 *		一つのブロックのサイズ。
 *
 *	@returns:
 *		割り当てたメモリの先頭アドレス。
 */
void *memsceCalloc( SceSize blk_num, SceSize size );

/* MemSceRealloc
 *
 *	標準モジュールのreallocもどき。
 *	MemSceMalloc()で新しいメモリ領域を確保した後、内容をコピーして
 *	古いメモリ領域を開放する。
 *
 *	@param: void *src_mem_prt
 *		拡張したいメモリのポインタ。
 *
 *	@param: SceSize resize
 *		割り当てるメモリサイズ。
 *
 *	@returns:
 *		割り当てたメモリの先頭アドレス。
 *		元のメモリアドレスとは変わる。
 */
void *memsceRealloc( void *src_heap_start, SceSize resize );

/* MemSceMemalign
 *
 *	少なくともsizeバイト確保したアライメントされたメモリを得る。
 *	アライメント境界は1以上で、2のべき乗でなければならない。
 *	これは自動的に調べられ、2のべき乗でない場合は0を返す。
 *	(ちゃんと動いていないみたい)
 *
 *	@param: SceSize boundary
 *		アライメント境界を指定する。
 *
 *	@param: SceSize size
 *		少なくとも確保するサイズ。
 *
 *	@returns:
 *		割り当てたメモリの先頭アドレス。
 */
void *memsceMemalign( SceSize align, SceSize size );

/* MemSceFree
 *
 *	標準モジュールのfreeもどき。
 *	割り当てたメモリを解放する。
 *	解放するアドレスとしてNULLポインタを渡された場合はなにもせずに0で戻る。
 *
 *	@param: void *first_mem_ptr
 *		解放するメモリのアドレス。
 */
int memsceFree( void *heap_start );

/* MemSceMallocEx
 *
 *	実際にメモリを割り当てている関数。
 *	MemSceMalloc()/MemSceCalloc()/MemSceRealloc()/MemSceMemalignは最終的にこの関数を呼ぶ。
 *	引数を元に、sceKernelAllocPartitionMemory()を呼んで実際にメモリを確保する。
 *	このとき、すこし余分に確保して、先頭にメモリの解放や、再割り当てに必要な少量のデータを書き込む。
 *	このため、実際に割り当てられるメモリは指定したメモリサイズより数バイト多くなる。
 *
 *  sceKernelAllocPartitionMemory()は256バイト未満のメモリ領域を確保できない。
 *  どんなに小さなサイズを確保するように指示しても、256バイトは必ず確保する。
 *
 *	また、MemSceMalloc()/MemSceCalloc()/MemSceRealloc()/MemSceMemalignは確保するメモリパーティションとして、
 *	常にMEMSCE_PART_USERを、確保するブロックタイプとして常にPSP_SMEM_Lowを指定する。
 *	それ以外のパラメータでメモリを確保する場合もこの関数を使う。
 *	(正しく動作するかは別として。)
 *
 *	@param: SceSize boundary
 *		アライメント境界を指定する。
 *		この関数から直接指定する場合は2のべき乗かどうか調べないので注意。
 *		変な値を入れるとバスエラーが出るかもしれない。
 *
 *	@param: SceUID partitionid
 *		確保するメモリパーティション。
 *		(詳細 http://wiki.ps2dev.org/psp:memory_mapより)
 *			MEMSCE_PART_KERNEL_HIGH        1
 *			MEMSCE_PART_USER               2
 *			MEMSCE_PART_KERNEL_HIGH_MIRROR 3
 *			MEMSCE_PART_KERNEL_LOW         4
 *			MEMSCE_PART_VOLATILE           5
 *			MEMSCE_PART_USER_MIRROR        6
 *
 *	@param: const char *name
 *		新しいメモリブロックの名前。
 *		(詳細不明)
 *
 *	@param: int type
 *		メモリブロックタイプ。
 *		どこからメモリを割り当てるかの設定。
 *			PSP_SMEM_Low
 *              メモリアドレスの小さいほうから、連続した空き容量を確保できるアドレスを探す。
 *			PSP_SMEM_High
 *              メモリアドレスの大きいほうから、連続した空き容量を確保できるアドレスを探す。
 *			PSP_SMEM_Addr
 *              指定したメモリアドレスから、連続した空き容量を確保する。
 *              この領域がすでに使用されている場合はおそらくエラーが返る。
 *
 *	@param void *adddr
 *		typeがPSP_SMEM_Addrのときに使用。
 *      連続した空き容量を確保する先頭アドレスを指定。
 *
 *	@returns:
 *		割り当てたメモリのアドレス。
 */
void *memsceMallocEx( SceSize align, SceUID partitionid, const char *name, int type, SceSize size, void *addr );

#ifdef __cplusplus
}
#endif

#endif
