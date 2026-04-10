/* inimgrLoad.c */

#include "inimgr_types.h"

static enum inimgr_line_type inimgr_parse_line( const char *line, char **start );

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
				
				if( ( callback = __inimgr_find_callback( params, line_start ) ) ){
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
				
				new_section = __inimgr_create_section( params, line_start );
				if( ! new_section ) goto EXCEPTION_NOT_ENOUGH_MEMORY;
				
				new_section->prev = current_section;
				
				current_entry = NULL;
				current_section->next = new_section;
				current_section = new_section;
				
				break;
			case INIMGR_LINE_ENTRY:
				if( callback_info || ! current_section || ! inimgrParseEntry( line_start, &key, &value ) ) break;
				
				new_entry = __inimgr_create_entry( params, key, value );
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
