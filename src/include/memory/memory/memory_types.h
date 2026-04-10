/* memory_types.h                 */
/* 内部で使用。外から参照しない。 */

#include "../memory.h"
#include <string.h>
#include <math.h>

/*=========================================================
	ローカルマクロ
=========================================================*/
#define MEMORY_POWER_OF_TWO( x ) ( ! ( ( x ) & ( ( x ) - 1 ) ) )
#define MEMORY_REAL_SIZE( size ) MEMORY_PAGE_SIZE * (unsigned int)ceil( (double)size / (double)MEMORY_PAGE_SIZE )

/*=========================================================
	ローカル型宣言
=========================================================*/
struct memory_header {
	SceUID  blockId;
	SceSize size;
};

