/* inimgrSave.c */

#include "inimgr_types.h"

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
		
		callback = __inimgr_find_callback( params, section->name );
		
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
