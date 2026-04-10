/*
	pspbtn.c
*/

#include "cmpspbtn.h"

/*-----------------------------------------------
	ローカル関数プロトタイプ
-----------------------------------------------*/
static unsigned int cmpspbtn_get_code_by_label( char *btn_name );

/*-----------------------------------------------
	ローカル変数
-----------------------------------------------*/
static unsigned int st_buttons_mask;
static struct confmgr_pspbtn st_avail_buttons[] = {
	{ PSP_CTRL_SELECT,   "SELECT"   },
	{ PSP_CTRL_START,    "START"    },
	{ PSP_CTRL_UP,       "UP"       },
	{ PSP_CTRL_RIGHT,    "RIGHT"    },
	{ PSP_CTRL_DOWN,     "DOWN"     },
	{ PSP_CTRL_LEFT,     "LEFT"     },
	{ PSP_CTRL_LTRIGGER, "LTRIGGER" },
	{ PSP_CTRL_RTRIGGER, "RTRIGGER" },
	{ PSP_CTRL_TRIANGLE, "TRIANGLE" },
	{ PSP_CTRL_CIRCLE,   "CIRCLE"   },
	{ PSP_CTRL_CROSS,    "CROSS"    },
	{ PSP_CTRL_SQUARE,   "SQUARE"   },
	{ PSP_CTRL_HOME,     "HOME"     },
	{ PSP_CTRL_HOLD,     "HOLD"     },
	{ PSP_CTRL_NOTE,     "NOTE"     },
	{ PSP_CTRL_SCREEN,   "SCREEN"   },
	{ PSP_CTRL_VOLUP,    "VOLUP"    },
	{ PSP_CTRL_VOLDOWN,  "VOLDOWN"  },
	{ PSP_CTRL_WLAN_UP,  "WLANUP"   },
	{ PSP_CTRL_REMOTE,   "REMOTE"   },
	{ PSP_CTRL_DISC,     "DISC"     },
	{ PSP_CTRL_MS,       "MS"       }
};

/*=============================================*/

void cmpspbtnSetMask( unsigned int mask )
{
	st_buttons_mask = mask;
}

void cmpspbtnLoad( char *name, char *value, void *arg )
{
	unsigned int *buttons = (unsigned int *)arg;
	char *btn_name, *next;
	
	*buttons = 0;
	btn_name = value;
	
	do{
		next = strchr( btn_name, '+' );
		if( next ) *next = '\0';
		
		strutilRemoveChar( btn_name, "\x20\t" );
		strutilToUpper( btn_name );
		*buttons |= cmpspbtn_get_code_by_label( btn_name );
	} while( next && ( btn_name = ++next ) );
}

void cmpspbtnStore( char buf[], size_t max, void *arg )
{
	unsigned int buttons = *((unsigned int *)arg);
	int i;
	
	for( i = 0; i < sizeof( st_avail_buttons ) / sizeof( st_avail_buttons[0] ); i++ ){
		if( st_buttons_mask & st_avail_buttons[i].button ) continue;
		
		if( buttons & st_avail_buttons[i].button ){
			if( buf[0] != '\0' ){
				strutilSafeCat( buf, " + ", max );
			}
			strutilSafeCat( buf, st_avail_buttons[i].label, max );
		}
	}
}

static unsigned int cmpspbtn_get_code_by_label( char *btn_name )
{
	unsigned int btn_code = 0;
	
	int i;
	for( i = 0; i < sizeof( st_avail_buttons ) / sizeof( st_avail_buttons[0] ); i++ ){
		if( st_buttons_mask & st_avail_buttons[i].button ) continue;
		
		if( strcmp( btn_name, st_avail_buttons[i].label ) == 0 ){
			btn_code = st_avail_buttons[i].button;
			break;
		}
	}
	return btn_code;
}
