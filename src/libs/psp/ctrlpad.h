/*
	Controll pad utility
*/

#ifndef CTRLPAD_H
#define CTRLPAD_H

#include <pspkernel.h>
#include <pspctrl.h>
#include <psprtc.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include "utils/strutil.h"

/*-----------------------------------------------
	マクロ
-----------------------------------------------*/
#define CTRLPAD_INVALID_RIGHT_TRIANGLE_DEGREE 45
#define CTRLPAD_DEGREE_TO_RADIAN( d )         ( ( d ) *  (3.14 / 180 ) )

#define CTRLPAD_SQUARE( x )                ( ( x ) * ( x ) )
#define CTRLPAD_IN_DEADZONE( x, y, r )     ( CTRLPAD_SQUARE( x ) + CTRLPAD_SQUARE( y ) <= ( CTRLPAD_SQUARE( r ) << 1 ) )

#define CTRLPAD_IGNORE_ANALOG_DIRECTION    -1

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------
	型宣言
-----------------------------------------------*/

/*
	アナログパッドの方向を格納。
	ボタンコードとして使用し、enum PspCtrlButtonsが使用していない空きビットを使用。
*/
enum ctrlpad_analog_direction {
	CTRLPAD_CTRL_ANALOG_UP    = 0x00000002,
	CTRLPAD_CTRL_ANALOG_RIGHT = 0x00000004,
	CTRLPAD_CTRL_ANALOG_DOWN  = 0x00000400,
	CTRLPAD_CTRL_ANALOG_LEFT  = 0x00000800
};

struct ctrlpad_button {
	unsigned int button;
	char *label;
};

typedef struct {
	unsigned int lastButtons;
	uint64_t     tickLast;
	uint64_t     tickRepeatDelayLow;
	uint64_t     tickRepeatDelayHigh;
	int          countLowToHigh;
	int          countLow;
	unsigned int maskRepeatButtons;
} CtrlpadParams;

/*-----------------------------------------------
	関数
-----------------------------------------------*/
void ctrlpadInit( CtrlpadParams *params );
void ctrlpadPref( CtrlpadParams *params, uint64_t low, uint64_t high, int count );
void ctrlpadSetRepeatButtons( CtrlpadParams *params, unsigned int mask );
void ctrlpadReset( CtrlpadParams *params );
void ctrlpadUpdateData( CtrlpadParams *params );
unsigned int ctrlpadGetData( CtrlpadParams *params, SceCtrlData *pad_data, int analog_deadzone );

/*-----------------------------------------------
	ユーティリティ関数
-----------------------------------------------*/
char *ctrlpadUtilButtonsToString( unsigned int buttons, char *str, size_t max );
unsigned int ctrlpadUtilStringToButtons( char *str );
unsigned int ctrlpadUtilGetAnalogDirection( int x, int y, int deadzone );

#ifdef __cplusplus
}
#endif

#endif
