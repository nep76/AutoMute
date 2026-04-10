/*
	inimgr.c
*/

#include "inimgr.h"

/*-----------------------------------------------
	ローカル関数
-----------------------------------------------*/
static enum inimgr_line_type inimgr_parse_line( const char *line, char **start, char **end );
static bool inimgr_split_keyval( char *line, char **key, char **value );
static char *inimgr_get_value( struct inimgr_params *params, const char *section, const char *key );
static int inimgr_set_value( struct inimgr_params *params, const char *section, const char *key, const char *value );
static struct inimgr_section *inimgr_search_section( struct inimgr_params *params, const char *section );
static struct inimgr_entry *inimgr_search_entry( struct inimgr_section *section, const char *key );
static struct inimgr_callback *inimgr_search_callback( struct inimgr_params *params, const char *section );
static struct inimgr_section *inimgr_create_section( const char *section );
static struct inimgr_entry *inimgr_create_entry( const char *key, const char *value );
static void inimgr_delete_section( struct inimgr_params *params, struct inimgr_section *section );
static void inimgr_delete_entry( struct inimgr_section *section, struct inimgr_entry *entry );

/*-----------------------------------------------
	ローカル変数
-----------------------------------------------*/


/*=============================================*/

IniUID inimgrNew( void )
{
	struct inimgr_params *params = (struct inimgr_params *)memsceMalloc( sizeof( struct inimgr_params ) );
	if( ! params ) return CG_ERROR_NOT_ENOUGH_MEMORY;
	
	params->callbacks = NULL;
	
	/* エントリの有無にかかわらず、デフォルトセクションは作成 */
	params->index = (struct inimgr_section *)memsceMalloc( sizeof( struct inimgr_section ) );
	if( ! params->index ){
		memsceFree( params );
		return CG_ERROR_NOT_ENOUGH_MEMORY;
	}
	
	strcpy( params->index->name, INIMGR_DEFAULT_SECTION_NAME );
	params->index->entry = NULL;
	params->index->prev  = NULL;
	params->index->next  = NULL;
	params->last  = params->index;
	
	return (IniUID)params;
}

int inimgrSetCallback( IniUID uid, const char *section, InimgrLoadCallback lcb, InimgrSaveCallback scb )
{
	struct inimgr_params   *params = (struct inimgr_params *)uid;
	struct inimgr_callback *new_cb = (struct inimgr_callback *)memsceMalloc( sizeof( struct inimgr_callback ) );
	if( ! new_cb ) return CG_ERROR_NOT_ENOUGH_MEMORY;
	
	strutilSafeCopy( new_cb->section, section, INIMGR_SECTION_BUFFER );
	new_cb->lcb = lcb;
	new_cb->scb = scb;
	
	if( params->callbacks ){
		struct inimgr_callback *last_cb;
		for( last_cb = params->callbacks; last_cb->next; last_cb = last_cb->next );
		last_cb->next = new_cb;
		new_cb->next  = NULL;
	} else{
		new_cb->next = NULL;
		params->callbacks = new_cb;
	}
	
	return 0;
}

int inimgrLoad( IniUID uid, const char *inipath )
{
	FilehUID fuid;
	char     buf[INIMGR_ENTRY_BUFFER];
	char     *line_start, *line_end;
	struct   inimgr_params    *params = (struct inimgr_params *)uid;
	struct   inimgr_section   *current_section = NULL;
	struct   inimgr_entry     *current_entry   = NULL;
	struct   inimgr_callback  *callback;
	
	fuid = filehOpen( inipath, PSP_O_RDONLY, 0777 );
	
	if( ! fuid ){
		return CG_ERROR_NOT_ENOUGH_MEMORY;
	} else if( filehGetLastError( fuid ) < 0 ){
		unsigned int ferror = filehGetLastError( fuid );
		filehDestroy( fuid );
		
		return ferror;
	}
	
	current_section = params->index;
	
	/* データ読み込み */
	while( filehReadln( fuid, buf, sizeof( buf ) ) ){
		struct inimgr_section  *new_section;
		struct inimgr_entry    *new_entry;
		char *key, *value;
		bool skip_section = false;
		
		switch( inimgr_parse_line( buf, &line_start, &line_end ) ){
			case INIMGR_LINE_NULL:
				break;
			case INIMGR_LINE_SECTION:
				*(line_end + 1) = '\0';
				
				if( skip_section ) skip_section = false;
				
				new_section = inimgr_create_section( line_start );
				if( ! new_section ) goto EXCEPTION_NOT_ENOUGH_MEMORY;
				
				new_section->prev = current_section;
				
				current_entry = NULL;
				current_section->next = new_section;
				current_section = new_section;
				
				callback = inimgr_search_callback( params, line_start );
				if( callback && callback->lcb ){
					callback->offset = filehTell( fuid );
					skip_section = true;
				}
				break;
			case INIMGR_LINE_ENTRY:
				if( skip_section ) continue;
				
				*(line_end + 1) = '\0';
				
				if( ! current_section || ! inimgr_split_keyval( line_start, &key, &value ) ) break;
				
				new_entry = inimgr_create_entry( key, value );
				if( ! new_entry ) goto EXCEPTION_NOT_ENOUGH_MEMORY;
				
				new_entry->prev   = current_entry;
				
				if( current_entry ){
					current_entry->next = new_entry;
				} else{
					current_section->entry = new_entry;
				}
				current_entry = new_entry;
				break;
		}
	}
	
	/* スペシャルセクションの処理 */
	for( callback = params->callbacks; callback; callback = callback->next ){
		filehSeek( fuid, callback->offset, FW_SEEK_SET );
		buf[0] = '\0';
		( callback->lcb )( (IniUID)params, fuid, buf, sizeof( buf ) );
	}
	
	filehClose( fuid );
	
	return 0;
	
	/* メモリ不足の際に飛ばすgotoラベル */
	EXCEPTION_NOT_ENOUGH_MEMORY:
		filehClose( fuid );
		return CG_ERROR_NOT_ENOUGH_MEMORY;
}

int inimgrSave( IniUID uid, const char *inipath )
{
	FilehUID fuid;
	char     buf[INIMGR_ENTRY_BUFFER];
	struct   inimgr_params   *params = (struct inimgr_params *)uid;
	struct   inimgr_section  *section;
	struct   inimgr_entry    *entry;
	struct   inimgr_callback *callback;
	
	fuid = filehOpen( inipath, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777 );
	
	if( ! fuid ){
		return CG_ERROR_NOT_ENOUGH_MEMORY;
	} else if( filehGetLastError( fuid ) < 0 ){
		unsigned int ferror = filehGetLastError( fuid );
		filehDestroy( fuid );
		
		return ferror;
	}
	
	/* 先頭には必ずデフォルトセクションが存在するので、まずそれを書き込む */
	callback = inimgr_search_callback( params, params->index->name );
	if( callback ){
		buf[0] = '\0';
		( callback->scb )( fuid, buf, sizeof( buf ) );
	} else{
		for( entry = params->index->entry; entry; entry = entry->next ){
			filehWritef( fuid, buf, INIMGR_ENTRY_BUFFER, "%s = %s\x0D\x0A", entry->key, entry->value );
		}
	}
	if( params->index->entry ) filehWrite( fuid, "\x0D\x0A", 2 );
	
	/* 後続するセクションがあれば書き込み */
	for( section = params->index->next; section; section = section->next ){
		filehWritef( fuid, buf, INIMGR_SECTION_BUFFER + 5, "[%s]\x0D\x0A", section->name );
		
		callback = inimgr_search_callback( params, section->name );
		if( callback && callback->scb ){
			buf[0] = '\0';
			( callback->scb )( fuid, buf, sizeof( buf ) );
		} else{
			for( entry = section->entry; entry; entry = entry->next ){
				filehWritef( fuid, buf, INIMGR_ENTRY_BUFFER, "%s = %s\x0D\x0A", entry->key, entry->value );
			}
		}
		filehWrite( fuid, "\x0D\x0A", 2 );
	}
	
	filehClose( fuid );
	
	return 0;
}

void inimgrDestroy( IniUID uid )
{
	struct inimgr_params   *params = (struct inimgr_params *)uid;
	struct inimgr_section  *current_section, *next_section;
	struct inimgr_callback *current_cb, *next_cb;
	
	if( ! params ) return;
	
	for( current_cb = params->callbacks; current_cb; current_cb = next_cb ){
		next_cb = current_cb->next;
		memsceFree( current_cb );
	}
	
	for( current_section = params->index; current_section; current_section = next_section ){
		next_section = current_section->next;
		inimgr_delete_section( params, current_section );
	}
	memsceFree( params );
}

int inimgrAddSection( IniUID uid, const char *section )
{
	struct inimgr_section *last_section;
	struct inimgr_section *new_section;
	
	for( last_section = ((struct inimgr_params *)uid)->index; last_section->next; last_section = last_section->next );
	
	new_section = (struct inimgr_section *)memsceMalloc( sizeof( struct inimgr_section ) );
	if( ! new_section ) return CG_ERROR_NOT_ENOUGH_MEMORY;
	
	strutilSafeCopy( new_section->name, section, INIMGR_SECTION_BUFFER );
	new_section->entry = NULL;
	new_section->next  = NULL;
	new_section->prev  = last_section;
	last_section->next = new_section;
	
	return 0;
}

void inimgrDeleteSection( IniUID uid, const char *section )
{
	/* デフォルトセクションは削除させない */
	if( strcmp( section, INIMGR_DEFAULT_SECTION_NAME ) == 0 ) return;
	
	inimgr_delete_section( (struct inimgr_params *)uid, inimgr_search_section( (struct inimgr_params *)uid, section ) );
}

long inimgrGetInt( IniUID uid, const char *section, const char *key, const long def )
{
	char *value = inimgr_get_value( (struct inimgr_params *)uid, section, key );
	
	if( value ){
		return strtol( value, NULL, 10 );
	} else{
		return def;
	}
}

unsigned int inimgrGetString( IniUID uid, const char *section, const char *key, const char *def, char *buf, size_t bufsize )
{
	char *value = inimgr_get_value( (struct inimgr_params *)uid, section, key );
	
	if( value ){
		strutilSafeCopy( buf, value, bufsize );
	} else if( buf != def ){
		strutilSafeCopy( buf, def, bufsize );
	}
	
	return strlen( buf );
}

bool inimgrGetBool( IniUID uid, const char *section, const char *key, const bool def )
{
	char *value = inimgr_get_value( (struct inimgr_params *)uid, section, key );
	
	if( value ){
		strutilToUpper( value );
		if( strcmp( value, "ON" ) == 0 ){
			return true;
		} else if( strcmp( value, "OFF" ) == 0 ){
			return false;
		}
	}
	
	return def;
}

int inimgrSetInt( IniUID uid, const char *section, const char *key, const long num )
{
	char value[32];
	
	value[0] = '\0';
	snprintf( value, sizeof( value ), "%ld", num );
	
	return inimgr_set_value( (struct inimgr_params *)uid, section, key, value );
}

int inimgrSetString( IniUID uid, const char *section, const char *key, const char *str )
{
	return inimgr_set_value( (struct inimgr_params *)uid, section, key, str );
}

int inimgrSetBool( IniUID uid, const char *section, const char *key, const bool on )
{
	char value[4];
	
	if( on ){
		strcpy( value, "ON" );
	} else{
		strcpy( value, "OFF" );
	}
	
	return inimgr_set_value( (struct inimgr_params *)uid, section, key, value );
}

/*-------------------------------------
	コールバック関数用API
--------------------------------------*/
int inimgrCBReadln( FilehUID fuid, char *buf, size_t max )
{
	int readsize;
	SceOff rollback_point = filehTell( fuid );
	
	readsize = filehReadln( fuid, buf, max );
	
	if( readsize > 0 ){
		char *line_start, *line_end;
		if( inimgr_parse_line( buf, &line_start, &line_end ) == INIMGR_LINE_SECTION ){
			filehSeek( fuid, rollback_point, FW_SEEK_SET );
			return 0;
		}
	}
	return readsize;
}

int inimgrCBWriteln( FilehUID fuid, char *buf )
{
	int ret = filehWrite( fuid, buf, strlen( buf ) );
	filehWrite( fuid, "\x0D\x0A", 2 );
	
	return ret;
}

/*-------------------------------------
	スタティック関数
-------------------------------------*/
static enum inimgr_line_type inimgr_parse_line( const char *line, char **start, char **end )
{
	*start = strutilCounterPbrk( line, "\t\x20" );
	if( ! *start || **start == ';' ) return INIMGR_LINE_NULL;
	
	*end = strutilCounterReversePbrk( *start, "\t\x20" );
	
	if( **start == '[' && **end == ']' ){
		(*start)++;
		(*end)--;
		return INIMGR_LINE_SECTION;
	}
	
	return INIMGR_LINE_ENTRY;

}

static bool inimgr_split_keyval( char *line, char **key, char **value )
{
	char *delim = strchr( line, '=' );
	if( ! delim ) return false;
	
	*delim = '\0';
	
	*key   = line;
	*value = delim + 1;
	
	delim = strutilCounterReversePbrk( *key, "\t\x20" );
	if( ! delim ){
		return false;
	} else{
		*(delim + 1) = '\0';
	}
	
	delim = strutilCounterPbrk( *value, "\t\x20" );
	if( delim ){
		*value = delim;
	}
	
	return true;
}

static char *inimgr_get_value( struct inimgr_params *params, const char *section, const char *key )
{
	struct inimgr_section *target_section;
	
	if( ! params ) return NULL;
	
	target_section = inimgr_search_section( params, section );
	
	if( target_section ){
		struct inimgr_entry *target_entry = inimgr_search_entry( target_section, key );
		if( target_entry ){
			return target_entry->value;
		}
	}
	
	return NULL;
}

static int inimgr_set_value( struct inimgr_params *params, const char *section, const char *key, const char *value )
{
	struct inimgr_section *target_section;
	struct inimgr_entry   *target_entry;
	
	if( ! params ) return 0;
	
	target_section = inimgr_search_section( params, section );
	if( ! target_section ){
		int ret = inimgrAddSection( (IniUID)params, section );
		if( ret < 0 ) return ret;
		target_section = inimgr_search_section( params, section );
	}
	
	target_entry = inimgr_search_entry( target_section, key );
	if( ! target_entry ){
		struct inimgr_entry *last_entry;
		
		target_entry = inimgr_create_entry( key, value );
		if( ! target_entry ) return CG_ERROR_NOT_ENOUGH_MEMORY;
		
		if( ! target_section->entry ){
			target_section->entry = target_entry;
		} else{
			for( last_entry = target_section->entry; last_entry->next; last_entry = last_entry->next );
			last_entry->next = target_entry;
			target_entry->prev = last_entry;
		}
	} else{
		strutilSafeCopy( target_entry->value, value, target_entry->vspace );
	}
	
	return 0;
}

static struct inimgr_section *inimgr_search_section( struct inimgr_params *params, const char *section )
{
	struct inimgr_section *current_section;
	
	if( params->last && strcmp( params->last->name, section ) == 0 ) return params->last;
	
	for( current_section = params->index; current_section; current_section = current_section->next ){
		if( strcmp( current_section->name, section ) == 0 ){
			params->last = current_section;
			break;
		}
	}
	return current_section;
}

static struct inimgr_entry *inimgr_search_entry( struct inimgr_section *section, const char *key )
{
	struct inimgr_entry *current_entry;
	
	if( ! section ) return NULL;
	
	for( current_entry = section->entry; current_entry; current_entry = current_entry->next ){
		if( strcmp( current_entry->key, key ) == 0 ) break;
	}
	return current_entry;
}

static struct inimgr_callback *inimgr_search_callback( struct inimgr_params *params, const char *section )
{
	struct inimgr_callback *current_callback;
	
	for( current_callback = params->callbacks; current_callback; current_callback = current_callback->next ){
		if( strcmp( current_callback->section, section ) == 0 ) break;
	}
	
	return current_callback;
}

static struct inimgr_section *inimgr_create_section( const char *section )
{
	struct inimgr_section *new_section = (struct inimgr_section *)memsceMalloc( sizeof( struct inimgr_section ) );
	if( ! new_section ) return NULL;
	
	strutilSafeCopy( new_section->name, section, INIMGR_SECTION_BUFFER );
	new_section->entry = NULL;
	new_section->prev  = NULL;
	new_section->next  = NULL;
	
	return new_section;
}

static struct inimgr_entry *inimgr_create_entry( const char *key, const char *value )
{
	int keylen;
	struct inimgr_entry *new_entry = (struct inimgr_entry *)memsceMalloc( sizeof( struct inimgr_entry ) );
	if( ! new_entry ) return NULL;
	
	new_entry->key = (char *)memsceMalloc( INIMGR_ENTRY_BUFFER );
	if( ! new_entry->key ) return NULL;
	keylen = strlen( key );
	
	new_entry->value  = new_entry->key + keylen + 1;
	new_entry->vspace = INIMGR_ENTRY_BUFFER - ( keylen + 1 );
	new_entry->prev   = NULL;
	new_entry->next   = NULL;
	
	strcpy( new_entry->key, key );
	strutilSafeCopy( new_entry->value, value, new_entry->vspace );
	
	return new_entry;
}

static void inimgr_delete_section( struct inimgr_params *params, struct inimgr_section *section )
{
	struct inimgr_entry *current_entry, *next_entry;
	
	if( ! section ) return;
	
	/* sectionをインデックスから切り離す */
	if( section->prev ) section->prev->next = section->next;
	if( section->next ) section->next->prev = section->prev;
	if( params->index == section ) params->index = section->next;
	if( params->last  == section ) params->last  = NULL;
	
	for( current_entry = section->entry; current_entry; current_entry = next_entry ){
		next_entry = current_entry->next;
		inimgr_delete_entry( section, current_entry );
	}
	
	memsceFree( section );
}

static void inimgr_delete_entry( struct inimgr_section *section, struct inimgr_entry *entry )
{
	if( ! entry ) return;
	
	/* entryをインデックスから切り離す */
	if( entry->prev ) entry->prev->next = entry->next;
	if( entry->next ) entry->next->prev = entry->prev;
	if( section->entry == entry ) section->entry = entry->next;
	
	memsceFree( entry->key );
	memsceFree( entry );
}
