/*
	confmgr.c
*/

#include "confmgr.h"

int confmgrStore( ConfmgrHandlerParams *params, int count, const char *fullpath )
{
	FilehUID fuid;
	char line[CONFMGR_LINE_BUFFER], *buf;
	int i, linesize, bufsize, store_cnt = 0;
	
	fuid = filehOpen( fullpath, PSP_O_RDWR | PSP_O_CREAT | PSP_O_TRUNC, 0777 );
	
	if( ! fuid ){
		return -1;
	} else if( filehGetLastError( fuid ) < 0 ){
		unsigned int ferror = filehGetLastError( fuid );
		filehDestroy( fuid );
		
		return ferror;
	}
	
	for( i = 0; i < count; i++ ){
		snprintf( line, sizeof( line ), "%s = ", params[i].label );
		
		linesize = strlen( line );
		buf      = line + linesize;
		bufsize  = sizeof( line ) - linesize;
		
		if( params[i].type == CH_LONGINT ){
			snprintf( buf, bufsize, "%ld", *((long *)params[i].handler) );
		} else if( params[i].type == CH_CHAR ){
			strutilSafeCopy( buf, (char *)(params[i].handler), bufsize );
		} else if( params[i].type == CH_CALLBACK_STORE ){
			((ConfmgrStoreCallback)(params[i].handler))( buf, bufsize, params[i].args );
		} else{
			continue;
		}
		
		filehWrite( fuid, line, strlen( line ) );
		filehWrite( fuid, "\n", 1 );
		store_cnt++;
	}
	
	filehClose( fuid );
	
	return store_cnt;
}

int confmgrLoad( ConfmgrHandlerParams *params, int count, const char *fullpath )
{
	FilehUID fuid;
	char line[CONFMGR_LINE_BUFFER], *name, *value, *boundary;
	int i, load_cnt = 0;
	
	fuid = filehOpen( fullpath, PSP_O_RDONLY, 0777 );
	
	if( ! fuid ){
		return -1;
	} else{
		unsigned int ferror = filehGetLastError( fuid );
		filehDestroy( fuid );
		if( ferror == FILEH_ERROR_GETSTAT_FAILED ){
			return 0;
		} else if( ferror < 0 ){
			return ferror;
		}
	}
	
	while( filehReadln( fuid, line, sizeof( line ) ) ){
		if( line[0] == '#' || line[0] == '\0' ) continue;
		
		boundary = strchr( line, '=' );
		if( ! boundary ) continue;
		
		*boundary = '\0';
		name      = line;
		value     = ++boundary;
		
		strutilRemoveChar( name, "\x20\t" );
		strutilToUpper( name );
		
		if( name[0] == '\0' ) continue;
		
		for( i = 0; i < count; i++ ){
			if( strcmp( name, params[i].label ) == 0 ){
				if( params[i].type == CH_LONGINT ){
					*((long *)params[i].handler) = strtol( value, NULL, 10 );
				} else if( params[i].type == CH_CHAR ){
					strutilSafeCopy( (char *)(params[i].handler), value, *((size_t *)params[i].args) );
				} else if( params[i].type == CH_CALLBACK_LOAD ){
			//		amNoticef( name );
					((ConfmgrLoadCallback)(params[i].handler))( name, value, params[i].args );
				} else{
					continue;
				}
				load_cnt++;
				break;
			}
		}
	}
	
	filehClose( fuid );
	
	return load_cnt;
}
