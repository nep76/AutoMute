/*==========================================================

	pspapi.h

	PSPSDKにプロトタイプがないAPI。

==========================================================*/
#ifndef PSPAPI_H
#define PSPAPI_H

#include <pspkernel.h>
#include <pspimpose_driver.h>

/*-----------------------------------------------
	POPSで使われるFW4.01のsceImpose_driver関数
-----------------------------------------------*/
int sceImposeGetParam401( SceImposeParam param );
int sceImposeSetParam401( SceImposeParam param, int val );

#endif
