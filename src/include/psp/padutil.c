/*=========================================================

	padutil.c

	パッドデータ処理。

=========================================================*/
#include "padutil.h"

/*=========================================================
	ローカルマクロ
=========================================================*/
#define PADUTIL_INVALID_RIGHT_TRIANGLE_DEGREE 45
#define PADUTIL_DEGREE_TO_RADIAN( d )         ( ( d ) *  ( 3.14 / 180 ) )
#define PADUTIL_SQUARE( x )                   ( ( x ) * ( x ) )
#define PADUTIL_IN_DEADZONE( x, y, r )        ( PADUTIL_SQUARE( x ) + PADUTIL_SQUARE( y ) <= ( PADUTIL_SQUARE( r ) ) )

/*=========================================================
	ローカル変数
=========================================================*/
static PadutilButtonName *st_symbols;
static unsigned short    st_symbols_refcount;
static PadutilButtonName *st_names;
static unsigned short    st_names_refcount;

/*=========================================================
	関数
=========================================================*/
PadutilButtonName *padutilCreateButtonSymbols( void )
{
	if( st_symbols ){
		st_symbols_refcount++;
		return st_symbols;
	}
	
	st_symbols = (PadutilButtonName *)memoryAllocEx( "PadutilButtonSymbols", MEMORY_USER, 0, sizeof( PadutilButtonName ) * 33, PSP_SMEM_High, NULL );
	if( ! st_symbols ) return NULL;
	
	st_symbols[0].button = padutilSetPad( PSP_CTRL_SELECT );
	st_symbols[0].name   = "SELECT";
	st_symbols[1].button = padutilSetPad( PSP_CTRL_START );
	st_symbols[1].name   = "START";
	
	st_symbols[2].button = padutilSetPad( PSP_CTRL_UP );
	st_symbols[2].name   = "\x80";
	st_symbols[3].button = padutilSetPad( PSP_CTRL_RIGHT );
	st_symbols[3].name   = "\x81";
	st_symbols[4].button = padutilSetPad( PSP_CTRL_DOWN );
	st_symbols[4].name   = "\x82";
	st_symbols[5].button = padutilSetPad( PSP_CTRL_LEFT );
	st_symbols[5].name   = "\x83";
	
	st_symbols[6].button = padutilSetPad( PADUTIL_CTRL_ANALOG_UP );
	st_symbols[6].name   = "A\x80";
	st_symbols[7].button = padutilSetPad( PADUTIL_CTRL_ANALOG_RIGHT );
	st_symbols[7].name   = "A\x81";
	st_symbols[8].button = padutilSetPad( PADUTIL_CTRL_ANALOG_DOWN );
	st_symbols[8].name   = "A\x82";
	st_symbols[9].button = padutilSetPad( PADUTIL_CTRL_ANALOG_LEFT );
	st_symbols[9].name   = "A\x83";
	
	st_symbols[10].button = padutilSetPad( PSP_CTRL_LTRIGGER );
	st_symbols[10].name   = "L";
	st_symbols[11].button = padutilSetPad( PSP_CTRL_RTRIGGER );
	st_symbols[11].name   = "R";
	
	st_symbols[12].button = padutilSetPad( PSP_CTRL_TRIANGLE );
	st_symbols[12].name   = "\x84";
	st_symbols[13].button = padutilSetPad( PSP_CTRL_CIRCLE );
	st_symbols[13].name   = "\x85";
	st_symbols[14].button = padutilSetPad( PSP_CTRL_CROSS );
	st_symbols[14].name   = "\x86";
	st_symbols[15].button = padutilSetPad( PSP_CTRL_SQUARE );
	st_symbols[15].name   = "\x87";
	
	st_symbols[16].button = padutilSetPad( PSP_CTRL_HOME );
	st_symbols[16].name   = "HOME";
	st_symbols[17].button = padutilSetPad( PSP_CTRL_HOLD );
	st_symbols[17].name   = "HOLD";
	st_symbols[18].button = padutilSetPad( PSP_CTRL_NOTE );
	st_symbols[18].name   = "\x88";
	st_symbols[19].button = padutilSetPad( PSP_CTRL_SCREEN );
	st_symbols[19].name   = "SCREEN";
	
	st_symbols[20].button = padutilSetPad( PSP_CTRL_VOLUP );
	st_symbols[20].name   = "VOL+";
	st_symbols[21].button = padutilSetPad( PSP_CTRL_VOLDOWN );
	st_symbols[21].name   = "VOL-";
	
	st_symbols[22].button = padutilSetPad( PSP_CTRL_WLAN_UP );
	st_symbols[22].name   = "WLAN";
	st_symbols[23].button = padutilSetPad( PSP_CTRL_REMOTE );
	st_symbols[23].name   = "REMOTE";
	st_symbols[24].button = padutilSetPad( PSP_CTRL_DISC );
	st_symbols[24].name   = "DISC";
	st_symbols[25].button = padutilSetPad( PSP_CTRL_MS );
	st_symbols[25].name   = "MS";
	
	st_symbols[26].button = padutilSetHprm( PSP_HPRM_PLAYPAUSE );
	st_symbols[26].name   = "RPLAY";
	st_symbols[27].button = padutilSetHprm( PSP_HPRM_FORWARD );
	st_symbols[27].name   = "RNEXT";
	st_symbols[28].button = padutilSetHprm( PSP_HPRM_BACK );
	st_symbols[28].name   = "RPREV";
	st_symbols[29].button = padutilSetHprm( PSP_HPRM_VOL_UP );
	st_symbols[29].name   = "RVOL+";
	st_symbols[30].button = padutilSetHprm( PSP_HPRM_VOL_DOWN );
	st_symbols[30].name   = "RVOL-";
	st_symbols[31].button = padutilSetHprm( PSP_HPRM_HOLD );
	st_symbols[31].name   = "RHOLD";
	
	st_symbols[32].button = 0;
	st_symbols[32].name   = NULL;
	
	st_symbols_refcount++;
	return st_symbols;
}

PadutilButtonName *padutilCreateButtonNames( void )
{
	if( st_names ){
		st_names_refcount++;
		return st_names;
	}
	
	st_names = (PadutilButtonName *)memoryAllocEx( "PadutilButtonNames", MEMORY_USER, 0, sizeof( PadutilButtonName ) * 33, PSP_SMEM_High, NULL );
	if( ! st_names ) return NULL;
	
	st_names[0].button = padutilSetPad( PSP_CTRL_SELECT );
	st_names[0].name   = "SELECT";
	st_names[1].button = padutilSetPad( PSP_CTRL_START );
	st_names[1].name   = "START";
	
	st_names[2].button = padutilSetPad( PSP_CTRL_UP );
	st_names[2].name   = "Up";
	st_names[3].button = padutilSetPad( PSP_CTRL_RIGHT );
	st_names[3].name   = "Right";
	st_names[4].button = padutilSetPad( PSP_CTRL_DOWN );
	st_names[4].name   = "Down";
	st_names[5].button = padutilSetPad( PSP_CTRL_LEFT );
	st_names[5].name   = "Left";
	
	st_names[6].button = padutilSetPad( PADUTIL_CTRL_ANALOG_UP );
	st_names[6].name   = "AnalogUp";
	st_names[7].button = padutilSetPad( PADUTIL_CTRL_ANALOG_RIGHT );
	st_names[7].name   = "AnalogRight";
	st_names[8].button = padutilSetPad( PADUTIL_CTRL_ANALOG_DOWN );
	st_names[8].name   = "AnalogDown";
	st_names[9].button = padutilSetPad( PADUTIL_CTRL_ANALOG_LEFT );
	st_names[9].name   = "AnalogLeft";
	
	st_names[10].button = padutilSetPad( PSP_CTRL_LTRIGGER );
	st_names[10].name   = "LTrigger";
	st_names[11].button = padutilSetPad( PSP_CTRL_RTRIGGER );
	st_names[11].name   = "RTrigger";
	
	st_names[12].button = padutilSetPad( PSP_CTRL_TRIANGLE );
	st_names[12].name   = "Triangle";
	st_names[13].button = padutilSetPad( PSP_CTRL_CIRCLE );
	st_names[13].name   = "Circle";
	st_names[14].button = padutilSetPad( PSP_CTRL_CROSS );
	st_names[14].name   = "Cross";
	st_names[15].button = padutilSetPad( PSP_CTRL_SQUARE );
	st_names[15].name   = "Square";
	
	st_names[16].button = padutilSetPad( PSP_CTRL_HOME );
	st_names[16].name   = "HOME";
	st_names[17].button = padutilSetPad( PSP_CTRL_HOLD );
	st_names[17].name   = "HOLD";
	st_names[18].button = padutilSetPad( PSP_CTRL_NOTE );
	st_names[18].name   = "NOTE";
	st_names[19].button = padutilSetPad( PSP_CTRL_SCREEN );
	st_names[19].name   = "SCREEN";
	
	st_names[20].button = padutilSetPad( PSP_CTRL_VOLUP );
	st_names[20].name   = "VolUp";
	st_names[21].button = padutilSetPad( PSP_CTRL_VOLDOWN );
	st_names[21].name   = "VolDown";
	
	st_names[22].button = padutilSetPad( PSP_CTRL_WLAN_UP );
	st_names[22].name   = "WLAN";
	st_names[23].button = padutilSetPad( PSP_CTRL_REMOTE );
	st_names[23].name   = "REMOTE";
	st_names[24].button = padutilSetPad( PSP_CTRL_DISC );
	st_names[24].name   = "DISC";
	st_names[25].button = padutilSetPad( PSP_CTRL_MS );
	st_names[25].name   = "MS";
	
	st_names[26].button = padutilSetHprm( PSP_HPRM_PLAYPAUSE );
	st_names[26].name   = "RPlayPause";
	st_names[27].button = padutilSetHprm( PSP_HPRM_FORWARD );
	st_names[27].name   = "RForward";
	st_names[28].button = padutilSetHprm( PSP_HPRM_BACK );
	st_names[28].name   = "RBack";
	st_names[29].button = padutilSetHprm( PSP_HPRM_VOL_UP );
	st_names[29].name   = "RVolUp";
	st_names[30].button = padutilSetHprm( PSP_HPRM_VOL_DOWN );
	st_names[30].name   = "RVolDown";
	st_names[31].button = padutilSetHprm( PSP_HPRM_HOLD );
	st_names[31].name   = "RHOLD";
	
	st_names[32].button = 0;
	st_names[32].name   = NULL;
	
	st_names_refcount++;
	return st_names;
}

void padutilDestroyButtonList( PadutilButtonName *name )
{
	if( name == st_symbols && st_symbols_refcount ){
		st_symbols_refcount--;
		if( ! st_symbols_refcount ){
			memoryFree( st_symbols );
			st_symbols = NULL;
		}
	} else if( name == st_names && st_names_refcount ){
		st_names_refcount--;
		if( ! st_names_refcount ){
			memoryFree( st_names );
			st_names = NULL;
		}
	}
}

char *padutilGetButtonNamesByCode( PadutilButtonName *availbtn, PadutilButtons buttons, const char *delim, unsigned int opt, char *buf, size_t len )
{
	int i;
	char delimtoken[16];
	
	if( ! buf || ! len || ! availbtn ) return NULL;
	if( ! buttons  ){
		*buf = '\0';
		return buf;
	}
	
	if( delim ){
		strutilCopy( delimtoken, delim, sizeof( delimtoken ) );
		
		if( opt & PADUTIL_OPT_IGNORE_SP ){
			strutilRemoveChar( delimtoken, "\x20\t" );
		}
		if( opt & PADUTIL_OPT_TOKEN_SP ){
			int len = strlen( delimtoken );
			memmove( delimtoken + 1, delimtoken, len + 1 );
			delimtoken[0]       = '\x20';
			delimtoken[len + 1] = '\x20';
			delimtoken[len + 2] = '\0';
		}
	}
	
	*buf = '\0';
	
	for( i = 0; availbtn[i].button || availbtn[i].name; i++ ){
		if( buttons & availbtn[i].button ){
			if( *buf != '\0' && *delimtoken ){
				strutilCat( buf, delimtoken, len );
			}
			if( availbtn[i].name ) strutilCat( buf, availbtn[i].name, len );
		}
	}
	
	return buf;
}

PadutilButtons padutilGetButtonCodeByNames( PadutilButtonName *availbtn, char *names, const char *delim, unsigned int opt )
{
	int i, nameslen = 0, delimlen = 0;
	PadutilButtons buttons = 0;
	char *btn_name, *next = NULL;
	
	void *comparer;
	char delimtoken[16];
	
	if( ! names || names[0] == '\0' || ! availbtn ) return 0;
	if( delim ) delimlen = strlen( delim );
	
	nameslen = strlen( names );
	
	strutilCopy( delimtoken, delim, sizeof( delimtoken ) );
	
	if( opt & PADUTIL_OPT_IGNORE_SP ){
		strutilRemoveChar( names,      "\x20\t" );
		strutilRemoveChar( delimtoken, "\x20\t" );
	}
	if( opt & PADUTIL_OPT_CASE_SENS ){
		comparer = strcmp;
	} else{
		comparer = strcasecmp;
	}
	
	btn_name = names;
	
	do{
		if( delimlen ){
			next = strstr( btn_name, delimtoken );
			if( next ){
				*next = '\0';
				next += delimlen;
			}
		}
		
		for( i = 0; availbtn[i].button || availbtn[i].name; i++ ){
			if( availbtn[i].name && ( ((int (*)(const char*, const char*))comparer)( btn_name, availbtn[i].name ) == 0 ) ){
				buttons |= availbtn[i].button;
				break;
			}
		}
		btn_name = next;
	} while( btn_name );
	
	return buttons;
}

unsigned int padutilGetAnalogStickDirection( PadutilCoord x, PadutilCoord y, PadutilCoord deadzone )
{
	unsigned int direction = 0;
	unsigned char nx, ny;
	nx = abs( x - PADUTIL_CENTER_X );
	ny = abs( y - PADUTIL_CENTER_Y );
	
	if( ( nx == 0 && ny == 0 ) || ( deadzone && PADUTIL_IN_DEADZONE( nx, ny, deadzone ) ) ) return 0;
	
	if( ny > (int)(sin( PADUTIL_DEGREE_TO_RADIAN( PADUTIL_INVALID_RIGHT_TRIANGLE_DEGREE ) ) * nx) ){
		if( y > PADUTIL_CENTER_Y ){
			direction |= PADUTIL_CTRL_ANALOG_DOWN;
		} else{
			direction |= PADUTIL_CTRL_ANALOG_UP;
		}
	}
	
	if( nx > (int)(sin( PADUTIL_DEGREE_TO_RADIAN( PADUTIL_INVALID_RIGHT_TRIANGLE_DEGREE ) ) * ny) ){
		if( x > PADUTIL_CENTER_X ){
			direction |= PADUTIL_CTRL_ANALOG_RIGHT;
		} else{
			direction |= PADUTIL_CTRL_ANALOG_LEFT;
		}
	}
	
	return direction;
}

void padutilSetAnalogStickDirection( unsigned int analog_direction, PadutilCoord *x, PadutilCoord *y )
{
	if( analog_direction & PADUTIL_CTRL_ANALOG_UP    ) *y = 0;
	if( analog_direction & PADUTIL_CTRL_ANALOG_RIGHT ) *x = 255;
	if( analog_direction & PADUTIL_CTRL_ANALOG_DOWN  ) *y = 255;
	if( analog_direction & PADUTIL_CTRL_ANALOG_LEFT  ) *x = 0;
}

PadutilRemap *padutilCreateRemapArray( size_t count )
{
	int i;
	PadutilRemap *remap = (PadutilRemap *)memoryAllocEx( "PadutilRemapArray", MEMORY_USER, 0, sizeof( PadutilRemap ) * count, PSP_SMEM_High, NULL );
	if( ! remap ) return NULL;
	
	remap[0].realButtons  = 0;
	remap[0].remapButtons = 0;
	remap[0].prev         = NULL;
	remap[0].next         = NULL;
	
	for( i = 1; i < count; i++ ){
		remap[i].realButtons  = 0;
		remap[i].remapButtons = 0;
		remap[i].prev         = &(remap[i - 1]);
		remap[i].next         = NULL;
		remap[i - 1].next     = &(remap[i]);
	}
	
	return remap;
}

void padutilDestroyRemapArray( PadutilRemap *remap )
{
	memoryFree( remap );
}

void padutilAdjustAnalogStick( PadutilAnalogStick *analogstick, SceCtrlData *pad )
{
	if( PADUTIL_IN_DEADZONE( abs( pad->Lx - analogstick->originX ), abs( pad->Ly - analogstick->originY ), analogstick->deadzone ) ){
		pad->Lx = PADUTIL_CENTER_X;
		pad->Ly = PADUTIL_CENTER_Y;
	} else if( analogstick->sensitivity != 1.0f ){
		int x = analogstick->originX + ( ( pad->Lx - analogstick->originX ) * analogstick->sensitivity );
		int y = analogstick->originY + ( ( pad->Ly - analogstick->originY ) * analogstick->sensitivity );
		
		if( x > PADUTIL_MAX_COORD ){
			x = PADUTIL_MAX_COORD;
		} else if( x < 0 ){
			x = 0;
		}
		
		if( y > PADUTIL_MAX_COORD ){
			y = PADUTIL_MAX_COORD;
		} else if( y < 0 ){
			y = 0;
		}
		
		pad->Lx = x;
		pad->Ly = y;
	}
	
	if(
		( analogstick->originX < PADUTIL_CENTER_X && pad->Lx > analogstick->originX && pad->Lx < PADUTIL_CENTER_X ) ||
		( analogstick->originX > PADUTIL_CENTER_X && pad->Lx < analogstick->originX && pad->Lx > PADUTIL_CENTER_X )
	){
		pad->Lx = PADUTIL_CENTER_X;
	}
	
	if(
		( analogstick->originY < PADUTIL_CENTER_Y && pad->Ly > analogstick->originY && pad->Ly < PADUTIL_CENTER_Y ) ||
		( analogstick->originY > PADUTIL_CENTER_Y && pad->Ly < analogstick->originY && pad->Ly > PADUTIL_CENTER_Y )
	){
		pad->Ly = PADUTIL_CENTER_Y;
	}
}

void padutilRemap( PadutilRemap *remap, PadutilButtons src, SceCtrlData *pad, u32 *hprmkey, bool redefine )
{
	PadutilButtons rmv_buttons = 0;
	PadutilButtons new_buttons = 0;
	unsigned char new_x      = pad->Lx;
	unsigned char new_y      = pad->Ly;
	unsigned int analog_dir  = padutilGetAnalogStickDirection( pad->Lx, pad->Ly, 0 );
	
	for( ; remap; remap = remap->next ){
		if( redefine ) rmv_buttons |= remap->remapButtons;
		if( ( ( src | padutilGetPad( analog_dir ) ) & remap->realButtons ) == remap->realButtons ){
			if( padutilGetPad( remap->realButtons ) & ( PADUTIL_CTRL_ANALOG_UP | PADUTIL_CTRL_ANALOG_RIGHT | PADUTIL_CTRL_ANALOG_DOWN | PADUTIL_CTRL_ANALOG_LEFT ) ){
				new_x = PADUTIL_CENTER_X;
				new_y = PADUTIL_CENTER_Y;
			}
			
			padutilSetAnalogStickDirection( padutilGetPad( remap->realButtons ), &new_x, &new_y );
			
			rmv_buttons |= remap->realButtons;
			new_buttons |= remap->remapButtons;
		}
	}
	
	pad->Buttons &= ~( padutilGetPad( rmv_buttons ) );
	pad->Buttons |= padutilGetPad( new_buttons );
	pad->Lx      = new_x;
	pad->Ly      = new_y;
	
	*hprmkey &= ~( padutilGetHprm( rmv_buttons ) );
	*hprmkey |= padutilGetHprm( new_buttons );
}
