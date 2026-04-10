/*
	ctrlpad.c
*/

#include "ctrlpad.h"

/*-----------------------------------------------
	ローカル関数プロトタイプ
-----------------------------------------------*/

/*-----------------------------------------------
	ローカル変数
-----------------------------------------------*/
struct ctrlpad_button st_button_names[] = {
	{ PSP_CTRL_SELECT,           "SELECT"      },
	{ PSP_CTRL_START,            "START"       },
	{ PSP_CTRL_UP,               "UP"          },
	{ PSP_CTRL_RIGHT,            "RIGHT"       },
	{ PSP_CTRL_DOWN,             "DOWN"        },
	{ PSP_CTRL_LEFT,             "LEFT"        },
	{ PSP_CTRL_LTRIGGER,         "LTRIGGER"    },
	{ PSP_CTRL_RTRIGGER,         "RTRIGGER"    },
	{ PSP_CTRL_TRIANGLE,         "TRIANGLE"    },
	{ PSP_CTRL_CIRCLE,           "CIRCLE"      },
	{ PSP_CTRL_CROSS,            "CROSS"       },
	{ PSP_CTRL_SQUARE,           "SQUARE"      },
	{ PSP_CTRL_HOME,             "HOME"        },
	{ PSP_CTRL_HOLD,             "HOLD"        },
	{ PSP_CTRL_NOTE,             "NOTE"        },
	{ PSP_CTRL_SCREEN,           "SCREEN"      },
	{ PSP_CTRL_VOLUP,            "VOLUP"       },
	{ PSP_CTRL_VOLDOWN,          "VOLDOWN"     },
	{ PSP_CTRL_WLAN_UP,          "WLANUP"      },
	{ PSP_CTRL_REMOTE,           "REMOTE"      },
	{ PSP_CTRL_DISC,             "DISC"        },
	{ PSP_CTRL_MS,               "MS"          },
	{ CTRLPAD_CTRL_ANALOG_UP,    "ANALOGUP"    },
	{ CTRLPAD_CTRL_ANALOG_RIGHT, "ANALOGRIGHT" },
	{ CTRLPAD_CTRL_ANALOG_DOWN,  "ANALOGDOWN"  },
	{ CTRLPAD_CTRL_ANALOG_LEFT,  "ANALOGLEFT"  }
};
/*=============================================*/

char *ctrlpadUtilButtonsToString( unsigned int buttons, char *str, size_t max )
{
	int i;
	
	*str = '\0';
	
	for( i = 0; i < sizeof( st_button_names) / sizeof( st_button_names[0] ); i++ ){
		if( buttons & st_button_names[i].button ){
			if( *str != '\0' ){
				strutilSafeCat( str, " + ", max );
			}
			strutilSafeCat( str, st_button_names[i].label, max );
		}
	}
	
	return str;
}

unsigned int ctrlpadUtilStringToButtons( char *str )
{
	int i;
	unsigned int buttons = 0;
	char *btn_name, *next;
	
	btn_name = str;
	
	do{
		next = strchr( btn_name, '+' );
		if( next ) *next = '\0';
		
		strutilRemoveChar( btn_name, "\x20\t" );
		strutilToUpper( btn_name );
		
		for( i = 0; i < sizeof( st_button_names ) / sizeof( st_button_names[0] ); i++ ){
			if( strcmp( btn_name, st_button_names[i].label ) == 0 ){
				buttons |= st_button_names[i].button;
				break;
			}
		}
	
	} while( next && ( btn_name = ++next ) );
	
	return buttons;
}

unsigned int ctrlpadUtilGetAnalogDirection( int x, int y, int deadzone )
{
	unsigned int direction = 0;
	
	x -= 128;
	y -= 128;
	
	if( deadzone ){
		if( CTRLPAD_IN_DEADZONE( abs( x ), abs( y ), deadzone ) ) return 0;
	} else{
		if( x == 0 && y == 0 ) return 0;
	}
	
	if( abs( y ) > (int)(sin( CTRLPAD_DEGREE_TO_RADIAN( CTRLPAD_INVALID_RIGHT_TRIANGLE_DEGREE ) ) * abs( x )) ){
		if( y > 0 ){
			direction |= CTRLPAD_CTRL_ANALOG_DOWN;
		} else if( y < 0 ){
			direction |= CTRLPAD_CTRL_ANALOG_UP;
		}
	}
	
	if( abs( x ) > (int)(sin( CTRLPAD_DEGREE_TO_RADIAN( CTRLPAD_INVALID_RIGHT_TRIANGLE_DEGREE ) ) * abs( y )) ){
		if( x > 0 ){
			direction |= CTRLPAD_CTRL_ANALOG_RIGHT;
		} else if( x < 0 ){
			direction |= CTRLPAD_CTRL_ANALOG_LEFT;
		}
	}
	
	return direction;
}

void ctrlpadInit( CtrlpadParams *params )
{
	if( ! params ) return;
	
	params->tickRepeatDelayLow  = 250000;
	params->tickRepeatDelayHigh = 100000;
	params->countLowToHigh      = 1;
	params->maskRepeatButtons   = 0;
	
	sceCtrlSetSamplingMode( PSP_CTRL_MODE_ANALOG );
	
	ctrlpadUpdateData( params );
}

void ctrlpadPref( CtrlpadParams *params, uint64_t low, uint64_t high, int count )
{
	if( ! params ) return;
	
	params->tickRepeatDelayLow  = low;
	params->tickRepeatDelayHigh = high;
	params->countLowToHigh      = count;
}

void ctrlpadReset( CtrlpadParams *params )
{
	if( ! params ) return;
	
	params->tickLast    = 0;
	params->countLow    = 0;
	params->lastButtons = 0;
}

void ctrlpadSetRepeatButtons( CtrlpadParams *params, unsigned int mask )
{
	if( ! params ) return;
	
	params->maskRepeatButtons = mask;
}

void ctrlpadUpdateData( CtrlpadParams *params )
{
	SceCtrlData pad;
	if( ! params ) return;
	
	sceRtcGetCurrentTick( &(params->tickLast) );
	sceCtrlPeekBufferPositive( &pad, 1 );
	
	params->countLow    = 0;
	params->lastButtons = pad.Buttons;
}

unsigned int ctrlpadGetData( CtrlpadParams *params, SceCtrlData *pad_data, int analog_deadzone )
{
	uint64_t current_tick;
	uint64_t *delay_tick;
	unsigned int buttons;
	
	sceRtcGetCurrentTick( &current_tick );
	sceCtrlPeekBufferPositive( pad_data, 1 );
	
	buttons = pad_data->Buttons;
	
	if( analog_deadzone >= 0 ){
		buttons |= ctrlpadUtilGetAnalogDirection( pad_data->Lx, pad_data->Ly, analog_deadzone );
	}
	
	if( params->countLow >= params->countLowToHigh ){
		delay_tick = &(params->tickRepeatDelayHigh);
	} else{
		delay_tick = &(params->tickRepeatDelayLow);
	}
	
	if( ! buttons ){
		ctrlpadReset( params );
	} else if( buttons != params->lastButtons ){
		params->tickLast    = current_tick;
		params->lastButtons = buttons;
		params->countLow    = 0;
	} else if( ! *delay_tick ){
		return 0;
	} else if( current_tick - params->tickLast > *delay_tick ){
		buttons             &= params->maskRepeatButtons;
		params->tickLast    = current_tick;
		params->lastButtons = buttons;
		if( params->countLow <= params->countLowToHigh ) params->countLow++;
	} else{
		return 0;
	}
	
	return buttons;
}
