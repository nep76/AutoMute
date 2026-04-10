/*=========================================================

	sysclib.h
	
	SysclibForKernelに存在するのに
	
		USE_KERNEL_LIBC = 1
	
	を指定すると見つからないプロトタイプの補完。
	力業でgccの警告対策。
	
=========================================================*/
#ifndef PSP_SYSCLIB_H
#define PSP_SYSCLIB_H

char *strtok_r( char *str, const char *delim, char **save_ptr );

#endif
