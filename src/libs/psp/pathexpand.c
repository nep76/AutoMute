/*
	pathexpand.c
*/

#include "pathexpand.h"

/*
	パスから "drive:" の長さを取得する。
	存在しない場合は0。
*/
static int pathexpand_get_drive_offset( const char *path )
{
	int i;
	for( i = 0; path[i]; i++ ){
		if( ! isalnum( path[i] ) ) break;
	}
	if( path[i] == ':' ) return i + 1;
	return 0;
}

/*
	絶対パスを正規化する。
	/../や/./、dir/../dir2/と言った冗長表現を捨てる。
	その性質上、文字列の長さが増えることはないので、バッファサイズを考慮しない。
*/
static void pathexpand_normalize_abspath( char *path )
{
	int i, j;
	
	/* ドライブ部を飛ばす */
	path += pathexpand_get_drive_offset( path );
	
	/* "//","///"などの無駄なスラッシュの連続を排除 */
	for( i = 0, j = 0; path[i + 1]; i++ ){
		if( path[i] == '/' && path[i + 1] == '/' ) continue;
		if( j < i ) path[j] = path[i];
		j++;
	}
	path[j++] = path[i];
	path[j]   = '\0';
	
	/* "." というカレントディレクトリ指定を排除 */
	for( i = 0, j = 0; path[i + 1]; i++ ){
		if(
			path[i] == '/' &&
			path[i + 1] == '.' &&
			( path[i + 2] == '/' || path[i + 2] == '\0' )
		){
			i += 1;
			continue;
		}
		if( j < i ) path[j] = path[i];
		j++;
	}
	path[j++] = path[i];
	path[j]   = '\0';
	
	/* 親ディレクトリ指定の".."を解決する */
	for( i = 0, j = 0; path[i]; i++ ){
		if(
			path[i] == '/' &&
			path[i + 1] == '.' &&
			path[i + 2] == '.' &&
			( path[i + 3] == '/' || path[i + 3] == '\0' )
		){
			i += 2;
			if( j ) for( j--; j && path[j] != '/'; j-- );
			continue;
		}
		if( j < i ) path[j] = path[i];
		j++;
	}
	path[j] = '\0';
	
	if( path[j - 1] == '/' ) path[j - 1] = '\0';
}

bool pathexpandFromBase( const char* basepath, const char *path, char *resolved_path, size_t len )
{
	if( ! path || ! resolved_path || ! len ) return false;
	
	if( pathexpand_get_drive_offset( path ) ){
		/* パスがドライブ部を含む場合は既に絶対パスなのでコピー */
		strutilSafeCopy( resolved_path, path, len );
	} else{
		/* ドライブ部が無い場合は、basepathの存在によって分岐 */
		if( ! basepath ){
			/* basepathが無い場合は、カレントディレクトリをそのまま使用 */
			getcwd( resolved_path, len );
		} else{
			/* basepathとresolved_pathのポインタが重なっている可能性を考慮してbasepathを退避 */
			char *base = (char *)memsceMalloc( strlen( basepath ) + 1 );
			strcpy( base, basepath );
			if( pathexpand_get_drive_offset( base ) ){
				/* basepathがドライブ部を含む場合は、絶対パスなのでコピー */
				strutilSafeCopy( resolved_path, base, len );
			} else{
				/* basepathがドライブ部を含まない場合は、それも相対パスなのでカレントディレクトリを取得 */
				getcwd( resolved_path, len );
				strutilSafeCat( resolved_path, "/", len );
				strutilSafeCat( resolved_path, base, len );
			}
			memsceFree( base );
		}
		
		if( path[0] == '/' ){
			/* パスがスラッシュから始まっていれば、現在のドライブのルートを設定 */
			int drive_offset = pathexpand_get_drive_offset( resolved_path );
			resolved_path[drive_offset] = '\0';
		}
		
		strutilSafeCat( resolved_path, "/", len );
		strutilSafeCat( resolved_path, path, len );
	}
	
	pathexpand_normalize_abspath( resolved_path );
	
	return true;
}

bool pathexpandFromCwd( const char *path, char *resolved_path, size_t len )
{
	return pathexpandFromBase( NULL, path, resolved_path, len );
}
