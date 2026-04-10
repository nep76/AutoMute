/*
	inimgr.c
*/

#include "inimgr.h"

/*-----------------------------------------------
	マクロ
-----------------------------------------------*/

/* INIMGR_SECTION_BUFFER - 1 以下でなければならない */
#define INIMGR_DEFAULT_SECTION_NAME "__default"

/* シグネチャ、バージョン 合わせて INIMGR_ENTRY_BUFFER - 2 以下でなければならない */
#define INIMGR_SIGNATURE_BUFFER ( INIMGR_ENTRY_BUFFER - 66 )
#define INIMGR_VERSION_BUFFER   ( INIMGR_ENTRY_BUFFER - INIMGR_SIGNATURE_BUFFER - 2 )

/*-----------------------------------------------
	ローカル宣言
-----------------------------------------------*/
enum inimgr_line_type {
	INIMGR_LINE_NULL = 0,
	INIMGR_LINE_SECTION,
	INIMGR_LINE_ENTRY
};

/*-----------------------------------------------
	ローカル関数
-----------------------------------------------*/
static enum inimgr_line_type inimgr_parse_line( const char *line, char **start );
static char *inimgr_get_value( struct inimgr_params *params, const char *section, const char *key );
static int inimgr_set_value( struct inimgr_params *params, const char *section, const char *key, const char *value );
static struct inimgr_section *inimgr_find_section( struct inimgr_params *params, const char *section );
static struct inimgr_entry *inimgr_find_entry( struct inimgr_section *section, const char *key );
static struct inimgr_callback *inimgr_find_callback( struct inimgr_params *params, const char *section );
static struct inimgr_section *inimgr_create_section( struct inimgr_params *params, const char *section );
static struct inimgr_entry *inimgr_create_entry( struct inimgr_params *params, const char *key, const char *value );
static void inimgr_delete_section( struct inimgr_params *params, struct inimgr_section *section );
static void inimgr_delete_entry( struct inimgr_params *params, struct inimgr_section *section, struct inimgr_entry *entry );

/*-----------------------------------------------
	ローカル変数
-----------------------------------------------*/


/*=============================================*/

IniUID inimgrNew( void )
{
	struct inimgr_params *params;
	DmemUID dmem = dmemNew( 0, PSP_SMEM_High );
	
	if( ! dmem ) return CG_ERROR_NOT_ENOUGH_MEMORY;
	
	params = (struct inimgr_params *)dmemAlloc( dmem, sizeof( struct inimgr_params ) );
	if( ! params ) return CG_ERROR_NOT_ENOUGH_MEMORY;
	
	params->dmem      = dmem;
	params->callbacks = NULL;
	
	/* エントリの有無にかかわらず、デフォルトセクションは作成 */
	params->index = (struct inimgr_section *)dmemAlloc( params->dmem, sizeof( struct inimgr_section ) + strlen( INIMGR_DEFAULT_SECTION_NAME ) + 1 );
	if( ! params->index ){
		dmemDestroy( params->dmem );
		return CG_ERROR_NOT_ENOUGH_MEMORY;
	}
	params->index->name = (char *)( (uintptr_t)(params->index) + sizeof( struct inimgr_section ) );
	
	
	strcpy( params->index->name, INIMGR_DEFAULT_SECTION_NAME );
	params->index->entry = NULL;
	params->index->prev  = NULL;
	params->index->next  = NULL;
	params->last  = params->index;
	
	return (IniUID)params;
}

int inimgrSetCallback( IniUID uid, const char *section, InimgrCallback cb, void *arg )
{
	unsigned short section_len;
	struct inimgr_params   *params = (struct inimgr_params *)uid;
	struct inimgr_callback *new_cb;
	
	if( ! section || *section == '\0' ) section = INIMGR_DEFAULT_SECTION_NAME;
	section_len = strlen( section );
	
	if( section_len + 1 > INIMGR_SECTION_BUFFER ) return CG_ERROR_OUT_OF_BUFFER;
	
	new_cb = (struct inimgr_callback *)dmemAlloc( params->dmem, sizeof( struct inimgr_callback ) + section_len + 1 );
	if( ! new_cb ) return CG_ERROR_NOT_ENOUGH_MEMORY;
	
	new_cb->section = (char *)( (uintptr_t)new_cb + sizeof( struct inimgr_callback ) );
	
	strcpy( new_cb->section, section );
	
	if( new_cb->section[section_len - 1] == '*' ){
		new_cb->cmplen = section_len - 1;
	} else{
		new_cb->cmplen = -1;
	}
	new_cb->cb       = cb;
	new_cb->arg      = arg;
	new_cb->infoList = NULL;
	
	if( params->callbacks ){
		struct inimgr_callback *last_cb;
		for( last_cb = params->callbacks; last_cb->next; last_cb = last_cb->next );
		last_cb->next = new_cb;
		new_cb->next  = NULL;
	} else{
		new_cb->next = NULL;
		params->callbacks = new_cb;
	}
	
	return CG_ERROR_OK;
}

int inimgrLoad( IniUID uid, const char *inipath, const char *sig, unsigned char major, unsigned char minor, unsigned char rev )
{
	FiomgrHandle fh;
	char   buf[INIMGR_ENTRY_BUFFER];
	struct inimgr_callback_info *callback_info    = NULL;
	struct inimgr_params        *params           = (struct inimgr_params *)uid;
	struct inimgr_section       *current_section  = NULL;
	struct inimgr_entry         *current_entry    = NULL;
	struct inimgr_callback      *callback         = NULL;
	
	fh = fiomgrOpen( inipath, FH_O_RDONLY | FH_O_ALLOC_HIGH, 0777 );
	if( fh < 0 ) return CG_ERROR_INVALID_ARGUMENT;
	
	/* シグネチャとバージョンをチェック */
	if( sig ){
		char *ini_version;
		
		if( strchr( sig, '\x20' ) ){
			fiomgrClose( fh );
			return CG_ERROR_INVALID_ARGUMENT;
		}
		
		while( fiomgrReadln( fh, buf, sizeof( buf ) ) ){
			if( buf[0] != ';' && buf[0] != '\0' ) break;
		}
		
		ini_version = strchr( buf, '\x20' );
		if( ini_version ){
			*ini_version = '\0';
			for( ini_version++; *ini_version != '\0' && ( *ini_version == '\x20' || *ini_version == '=' ); ini_version++ );
		}
		
		/* シグネチャ比較 */
		if( strcasecmp( sig, buf ) != 0 ) goto EXCEPTION_INVALID_SIGNATURE;
		
		if( major || minor || rev ){
			char *tmp, *save_ptr;
			unsigned char num;
			
			if( *ini_version == '\0' ) goto EXCEPTION_INVALID_VERSION;
			
			/* メジャー番号 */
			tmp = strtok_r( ini_version, ".", &save_ptr );
			num = (unsigned char)( tmp ? strtoul( tmp, NULL, 10 ) : 0 );
			if( major > num ) goto EXCEPTION_INVALID_VERSION;
			
			/* マイナー番号 */
			tmp = strtok_r( NULL, ".", &save_ptr );
			num = (unsigned char)( tmp ? strtoul( tmp, NULL, 10 ) : 0 );
			if( minor > num ) goto EXCEPTION_INVALID_VERSION;
			
			/* リビジョン番号 */
			tmp = strtok_r( NULL, ".", &save_ptr );
			num = (unsigned char)( tmp ? strtoul( tmp, NULL, 10 ) : 0 );
			if( rev > num ) goto EXCEPTION_INVALID_VERSION;
		}
	}
	
	current_section = params->index;
	
	/* データ読み込み */
	while( fiomgrReadln( fh, buf, sizeof( buf ) ) ){
		struct inimgr_section  *new_section;
		struct inimgr_entry    *new_entry;
		char *line_start = NULL, *key = NULL, *value = NULL;
		
		switch( inimgr_parse_line( buf, &line_start ) ){
			case INIMGR_LINE_NULL:
				break;
			case INIMGR_LINE_SECTION:
				if( callback_info ){
					callback_info->length = ( fiomgrTell( fh ) - strlen( buf ) ) - callback_info->offset;
					callback_info = NULL;
				}
				
				if( ( callback = inimgr_find_callback( params, line_start ) ) ){
					if( ! callback->infoList ){
						callback->infoList = dmemAlloc( params->dmem, sizeof( struct inimgr_callback_info ) );
						callback_info = callback->infoList;
					} else{
						for( callback_info = callback->infoList; callback_info->next; callback_info = callback_info->next );
						callback_info->next = dmemAlloc( params->dmem, sizeof( struct inimgr_callback_info ) );
						callback_info = callback_info->next;
					}
					if( callback->cmplen >= 0 ){
						callback_info->section = dmemAlloc( params->dmem, strlen( line_start ) + 1 );
						strcpy( callback_info->section, line_start );
					} else{
						callback_info->section = NULL;
					}
					callback_info->offset = fiomgrTell( fh );
					callback_info->length = 0;
					callback_info->next   = NULL;
				}
				
				new_section = inimgr_create_section( params, line_start );
				if( ! new_section ) goto EXCEPTION_NOT_ENOUGH_MEMORY;
				
				new_section->prev = current_section;
				
				current_entry = NULL;
				current_section->next = new_section;
				current_section = new_section;
				
				break;
			case INIMGR_LINE_ENTRY:
				if( callback_info || ! current_section || ! inimgrParseEntry( line_start, &key, &value ) ) break;
				
				new_entry = inimgr_create_entry( params, key, value );
				if( ! new_entry ) goto EXCEPTION_NOT_ENOUGH_MEMORY;
				
				new_entry->prev = current_entry;
				if( current_entry ){
					current_entry->next = new_entry;
				} else{
					current_section->entry = new_entry;
				}
				current_entry = new_entry;
				
				break;
		}
	}
	if( callback_info ) callback_info->length = fiomgrTell( fh ) - callback_info->offset;
	
	/* コールバックセクションの処理 */
	for( callback = params->callbacks; callback; callback = callback->next ){
		InimgrCallbackParams cb_params;
		
		if( ! callback->cb ) continue;
		
		cb_params.uid    = uid;
		cb_params.ini    = fh;
		
		for( callback_info = callback->infoList; callback_info; callback_info = callback_info->next ){
			cb_params.cbinfo = callback_info;
			
			fiomgrSeek( fh, callback_info->offset, FH_SEEK_SET );
			buf[0] = '\0';
			
			( (InimgrCallback)callback->cb )( INIMGR_CB_LOAD, &cb_params, buf, sizeof( buf ), callback->arg );
		}
	}
	
	fiomgrClose( fh );
	
	return CG_ERROR_OK;
	
	/* シグネチャ異常 */
	EXCEPTION_INVALID_SIGNATURE:
		fiomgrClose( fh );
		return INIMGR_ERROR_INVALID_SIGNATURE;
	
	EXCEPTION_INVALID_VERSION:
		fiomgrClose( fh );
		return INIMGR_ERROR_INVALID_VERSION;
	
	/* メモリ不足の際に飛ばすgotoラベル */
	EXCEPTION_NOT_ENOUGH_MEMORY:
		fiomgrClose( fh );
		return CG_ERROR_NOT_ENOUGH_MEMORY;
}

int inimgrSave( IniUID uid, const char *inipath, const char *sig, unsigned char major, unsigned char minor, unsigned char rev )
{
	FiomgrHandle fh;
	char     buf[INIMGR_ENTRY_BUFFER];
	struct   inimgr_params   *params = (struct inimgr_params *)uid;
	struct   inimgr_section  *section;
	struct   inimgr_entry    *entry;
	struct   inimgr_callback *callback;
	
	fh = fiomgrOpen( inipath, FH_O_WRONLY | FH_O_CREAT | FH_O_TRUNC | FH_O_ALLOC_HIGH, 0777 );
	if( fh < 0 ) return CG_ERROR_INVALID_ARGUMENT;
	
	/* シグネチャ */
	if( sig ){
		unsigned short len;
		char *version = &(buf[INIMGR_SIGNATURE_BUFFER]);
		if( strchr( sig, '\x20' ) ) return CG_ERROR_INVALID_ARGUMENT;
		
		len = strutilCopy( buf, sig, INIMGR_SIGNATURE_BUFFER );
		
		if( major || minor || rev ){
			if( ! major || rev ){
				snprintf( version, INIMGR_VERSION_BUFFER, " %d.%d.%d", major, minor, rev );
			} else if( minor ){
				snprintf( version, INIMGR_VERSION_BUFFER, " %d.%d", major, minor );
			} else{
				snprintf( version, INIMGR_VERSION_BUFFER, " %d", major );
			}
			len = strutilCat( buf, version, sizeof( buf ) );
		}
		
		fiomgrWriteln( fh, (void *)buf, --len );
	}
	
	/* セクションを書き込み */
	for( section = params->index; section; section = section->next ){
		
		callback = inimgr_find_callback( params, section->name );
		
		/* デフォルトセクションはセクション名を書き込まない, セクション名に"*"サフィックスがなければセクション名を書き込む */
		if( strcasecmp( section->name, INIMGR_DEFAULT_SECTION_NAME ) != 0 && ( ! callback || callback->cmplen < 0 ) ){
			int len = inimgrMakeSection( buf, sizeof( buf ), section->name );
			
			if( fiomgrTell( fh ) != 0 ) fiomgrWriteln( fh, "", 0 );
			fiomgrWriteln( fh, (void *)buf, len );
		}
		
		if( callback && callback->cb ){
			InimgrCallbackParams cb_params;
			
			if( ! callback->cb ) continue;
			
			buf[0] = '\0';
			
			cb_params.uid    = uid;
			cb_params.ini    = fh;
			cb_params.cbinfo = NULL;
			
			( (InimgrCallback)callback->cb )( INIMGR_CB_SAVE, &cb_params, buf, sizeof( buf ), callback->arg );
		} else{
			for( entry = section->entry; entry; entry = entry->next ){
				int len = inimgrMakeEntry( buf, sizeof( buf ), entry->key, entry->value );
				fiomgrWriteln( fh, (void *)buf, len );
			}
		}
	}
	
	fiomgrClose( fh );
	
	return CG_ERROR_OK;
}

void inimgrDestroy( IniUID uid )
{
	struct inimgr_params *params = (struct inimgr_params *)uid;
	
	if( ! params ) return;
	
	dmemDestroy( params->dmem );
}

int inimgrAddSection( IniUID uid, const char *section )
{
	struct inimgr_params  *params = (struct inimgr_params *)uid;
	struct inimgr_section *last_section;
	struct inimgr_section *new_section;
	size_t section_buf;
	
	if( ! section || strcasecmp( section, INIMGR_DEFAULT_SECTION_NAME ) == 0 ) return CG_ERROR_INVALID_ARGUMENT;
	
	section_buf = strlen( section ) + 1;
	if( section_buf > INIMGR_SECTION_BUFFER ) return CG_ERROR_OUT_OF_BUFFER;
	
	for( last_section = ((struct inimgr_params *)uid)->index; last_section->next; last_section = last_section->next );
	
	new_section = inimgr_create_section( params, section );
	if( ! new_section ) return CG_ERROR_NOT_ENOUGH_MEMORY;
	
	new_section->name = (char *)( (uintptr_t)new_section + sizeof( struct inimgr_section ) );
	new_section->prev  = last_section;
	last_section->next = new_section;
	
	return CG_ERROR_OK;
}

void inimgrDeleteSection( IniUID uid, const char *section )
{
	/* デフォルトセクションは削除させない */
	if( ! section || strcasecmp( section, INIMGR_DEFAULT_SECTION_NAME ) == 0 ) return;
	
	inimgr_delete_section( (struct inimgr_params *)uid, inimgr_find_section( (struct inimgr_params *)uid, section ) );
}

bool inimgrExistSection( IniUID uid, const char *section )
{
	if( ! section || strcasecmp( section, INIMGR_DEFAULT_SECTION_NAME ) == 0 || inimgr_find_section( (struct inimgr_params *)uid, section ) ) return true;
	return false;
}

bool inimgrGetInt( IniUID uid, const char *section, const char *key, int *var )
{
	char *value = inimgr_get_value( (struct inimgr_params *)uid, section, key );
	
	if( value ){
		*var = strtol( value, NULL, 10 );
		return true;
	} else{
		return false;
	}
}

int inimgrGetString( IniUID uid, const char *section, const char *key, char *buf, size_t bufsize )
{
	char *value = inimgr_get_value( (struct inimgr_params *)uid, section, key );
	
	if( value ){
		strutilCopy( buf, value, bufsize );
		return strlen( buf );
	} else {
		return -1;
	}
}

bool inimgrGetBool( IniUID uid, const char *section, const char *key, bool *var )
{
	char *value = inimgr_get_value( (struct inimgr_params *)uid, section, key );
	
	if( value ){
		strupr( value );
		if( strcasecmp( value, "On" ) == 0 ){
			*var = true;
		} else if( strcasecmp( value, "Off" ) == 0 ){
			*var = false;
		}
		return true;
	}
	
	return false;
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
		strcpy( value, "On" );
	} else{
		strcpy( value, "Off" );
	}
	
	return inimgr_set_value( (struct inimgr_params *)uid, section, key, value );
}

/*-------------------------------------
	コールバック関数用API
--------------------------------------*/
inline IniUID inimgrCbGetIniHandle( InimgrCallbackParams *params )
{
	return params->uid;
}

int inimgrCbReadln( InimgrCallbackParams *params, char *buf, size_t buflen )
{
	if( fiomgrTell( params->ini ) >= ( params->cbinfo->offset + params->cbinfo->length ) ) return 0;
	return fiomgrReadln( params->ini, buf, buflen );
}

int inimgrCbSeekSet( InimgrCallbackParams *params, SceOff offset )
{
	if( offset > 0 ){
		if( offset <= params->cbinfo->length ){
			return fiomgrSeek( params->ini, params->cbinfo->offset + offset, FH_SEEK_SET );
		} else{
			return fiomgrSeek( params->ini, params->cbinfo->offset + params->cbinfo->length, FH_SEEK_SET );
		}
	} else{
		return fiomgrSeek( params->ini, params->cbinfo->offset, FH_SEEK_SET );
	}
}

int inimgrCbSeekCur( InimgrCallbackParams *params, SceOff offset )
{
	SceOff pos = fiomgrTell( params->ini ) - params->cbinfo->offset;
	
	if( pos < 0 ){
		pos = 0;
	} else if( pos > params->cbinfo->length ){
		pos = params->cbinfo->length;
	}
	
	pos += offset;
	
	if( pos < 0 ){
		pos = 0;
	} else if( pos > params->cbinfo->length ){
		pos = params->cbinfo->length;
	}
	
	return fiomgrSeek( params->ini, params->cbinfo->offset + pos, FH_SEEK_SET );
}

int inimgrCbSeekEnd( InimgrCallbackParams *params, SceOff offset )
{
	SceOff endpos = params->cbinfo->offset + params->cbinfo->length;
	
	if( offset < 0 ){
		if( abs( offset ) <= params->cbinfo->length ){
			return fiomgrSeek( params->ini, endpos + offset, FH_SEEK_SET );
		} else{
			return fiomgrSeek( params->ini, params->cbinfo->offset, FH_SEEK_SET );
		}
	} else{
		return fiomgrSeek( params->ini, endpos, FH_SEEK_SET );
	}
}

int inimgrCbTell( InimgrCallbackParams *params )
{
	return params->cbinfo->offset - fiomgrTell( params->ini );
}

int inimgrCbWriteln( InimgrCallbackParams *params, char *buf, size_t buflen )
{
	return fiomgrWriteln( params->ini, buf, buflen );
}

bool inimgrParseEntry( char *entry, char **key, char **value )
{
	char *delim = strchr( entry, '=' );
	if( ! delim ) return false;
	
	*delim = '\0';
	*key   = entry;
	*value = delim + 1;
	
	delim = strutilCounterReversePbrk( *key, "\t\x20" );
	if( delim ){
		*(delim + 1) = '\0';
	}
	
	delim = strutilCounterPbrk( *value, "\t\x20" );
	if( delim ){
		*value = delim;
	}
	
	return true;
}

int inimgrMakeEntry( char *buf, size_t len, char *key, char *value )
{
	int ret = snprintf( buf, len, "%s = %s", key, value );
	
	len--;
	return ( ret > len ? len : ret );
}

int inimgrMakeSection( char *buf, size_t len, char *section )
{
	int ret = snprintf( buf, len, "[%s]", section );
	
	len--;
	return ( ret > len ? len : ret );
}

/*-------------------------------------
	スタティック関数
-------------------------------------*/
static enum inimgr_line_type inimgr_parse_line( const char *line, char **start )
{
	char *end;
	
	*start = strutilCounterPbrk( line, "\t\x20" );
	if( ! *start || **start == ';' ) return INIMGR_LINE_NULL;
	
	end = strutilCounterReversePbrk( *start, "\t\x20" );
	
	if( **start == '[' && *end == ']' ){
		(*start)++;
		*end = '\0';
		return INIMGR_LINE_SECTION;
	}
	
	*(end + 1) = '\0';
	
	return INIMGR_LINE_ENTRY;
}

static char *inimgr_get_value( struct inimgr_params *params, const char *section, const char *key )
{
	struct inimgr_section *target_section;
	
	if( ! params  ) return NULL;
	if( ! section ) section = INIMGR_DEFAULT_SECTION_NAME;
	
	target_section = inimgr_find_section( params, section );
	
	if( target_section ){
		struct inimgr_entry *target_entry = inimgr_find_entry( target_section, key );
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
	
	if( ! params ) return CG_ERROR_INVALID_ARGUMENT;
	if( ! section ) section = INIMGR_DEFAULT_SECTION_NAME;
	
	target_section = inimgr_find_section( params, section );
	if( ! target_section ){
		int ret = inimgrAddSection( (IniUID)params, section );
		if( ret < 0 ) return ret;
		target_section = inimgr_find_section( params, section );
	}
	
	target_entry = inimgr_find_entry( target_section, key );
	if( ! target_entry ){
		struct inimgr_entry *last_entry;
		
		target_entry = inimgr_create_entry( params, key, value );
		if( ! target_entry ) return CG_ERROR_NOT_ENOUGH_MEMORY;
		
		if( ! target_section->entry ){
			target_section->entry = target_entry;
		} else{
			for( last_entry = target_section->entry; last_entry->next; last_entry = last_entry->next );
			last_entry->next = target_entry;
			target_entry->prev = last_entry;
		}
	} else{
		strutilCopy( target_entry->value, value, target_entry->vspace );
	}
	
	return CG_ERROR_OK;
}

static struct inimgr_section *inimgr_find_section( struct inimgr_params *params, const char *section )
{
	struct inimgr_section *current_section;
	
	if( params->last && strcasecmp( params->last->name, section ) == 0 ) return params->last;
	
	for( current_section = params->index; current_section; current_section = current_section->next ){
		if( strcasecmp( current_section->name, section ) == 0 ){
			params->last = current_section;
			break;
		}
	}
	return current_section;
}

static struct inimgr_entry *inimgr_find_entry( struct inimgr_section *section, const char *key )
{
	struct inimgr_entry *current_entry;
	
	if( ! section ) return NULL;
	
	for( current_entry = section->entry; current_entry; current_entry = current_entry->next ){
		if( strcasecmp( current_entry->key, key ) == 0 ) break;
	}
	return current_entry;
}

static struct inimgr_callback *inimgr_find_callback( struct inimgr_params *params, const char *section )
{
	struct inimgr_callback *current_callback;
	
	for( current_callback = params->callbacks; current_callback; current_callback = current_callback->next ){
		if(
			( current_callback->cmplen < 0 && strcasecmp( current_callback->section, section ) == 0 ) ||
			( current_callback->cmplen == 0 ) ||
			( strncasecmp( current_callback->section, section, current_callback->cmplen ) == 0 )
		){
			break;
		}
	}
	
	return current_callback;
}

static struct inimgr_section *inimgr_create_section( struct inimgr_params *params, const char *section )
{
	struct inimgr_section *new_section;
	size_t section_buf = strlen( section ) + 1;
	
	if( section_buf > INIMGR_SECTION_BUFFER ) return NULL;
	
	new_section = (struct inimgr_section *)dmemAlloc( params->dmem, sizeof( struct inimgr_section ) + section_buf );
	if( ! new_section ) return NULL;
	
	new_section->name = (char *)( (uintptr_t)new_section + sizeof( struct inimgr_section ) );
	strcpy( new_section->name, section );
	new_section->entry = NULL;
	new_section->prev  = NULL;
	new_section->next  = NULL;
	
	return new_section;
}

static struct inimgr_entry *inimgr_create_entry( struct inimgr_params *params, const char *key, const char *value )
{
	int keylen;
	struct inimgr_entry *new_entry = (struct inimgr_entry *)dmemAlloc( params->dmem, sizeof( struct inimgr_entry ) );
	if( ! new_entry ) return NULL;
	
	new_entry->key = (char *)dmemAlloc( params->dmem, INIMGR_ENTRY_BUFFER );
	if( ! new_entry->key ) return NULL;
	keylen = strlen( key );
	
	new_entry->value  = new_entry->key + keylen + 1;
	new_entry->vspace = INIMGR_ENTRY_BUFFER - ( keylen + 1 );
	new_entry->prev   = NULL;
	new_entry->next   = NULL;
	
	strcpy( new_entry->key, key );
	strutilCopy( new_entry->value, value, new_entry->vspace );
	
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
		inimgr_delete_entry( params, section, current_entry );
	}
	
	dmemFree( params->dmem, section );
}

static void inimgr_delete_entry( struct inimgr_params *params, struct inimgr_section *section, struct inimgr_entry *entry )
{
	if( ! entry ) return;
	
	/* entryをインデックスから切り離す */
	if( entry->prev ) entry->prev->next = entry->next;
	if( entry->next ) entry->next->prev = entry->prev;
	if( section->entry == entry ) section->entry = entry->next;
	
	dmemFree( params->dmem, entry->key );
	dmemFree( params->dmem, entry );
}
