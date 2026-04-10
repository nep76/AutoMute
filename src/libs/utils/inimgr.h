/*
	INIマネージャ
	
	とりあえずinimgrNew時にすべての設定をメモリに取り込む仕様。
	確保されたメモリは
		inimgrAddSection()
		inimgrDeleteSection()
		inimgrSetInt()
		inimgrSetString()
		inimgrSetBool()
		inimgrDeleteEntry()
	により動的に増減し、
		inimgrDestroy()
	により全て解放される。
*/

#ifndef INIMGR_H
#define INIMGR_H

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "utils/strutil.h"
#include "psp/fileh.h"
#include "cgerrs.h"

/*-----------------------------------------------
	定数
-----------------------------------------------*/
#define INIMGR_DEFAULT_SECTION_NAME "default" /* 63文字以下でなければならない */
#define INIMGR_SECTION_BUFFER  64
#define INIMGR_ENTRY_BUFFER   256

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------
	型宣言
-----------------------------------------------*/
typedef int  IniUID;
typedef void ( *InimgrLoadCallback )( IniUID, FilehUID, char*, size_t );
typedef void ( *InimgrSaveCallback )( FilehUID, char*, size_t );

enum inimgr_line_type {
	INIMGR_LINE_NULL = 0,
	INIMGR_LINE_SECTION,
	INIMGR_LINE_ENTRY
};

struct inimgr_callback {
	char section[INIMGR_SECTION_BUFFER];
	InimgrLoadCallback lcb;
	InimgrSaveCallback scb;
	SceOff offset;
	struct inimgr_callback *next;
};

struct inimgr_entry {
	char   *key;
	char   *value;
	size_t vspace;
	struct inimgr_entry *prev, *next;
};

struct inimgr_section {
	char name[INIMGR_SECTION_BUFFER];
	struct inimgr_entry *entry;
	struct inimgr_section *prev, *next;
};

struct inimgr_params {
	struct inimgr_callback *callbacks;
	struct inimgr_section  *index;
	struct inimgr_section  *last;
};

/*-----------------------------------------------
	関数プロトタイプ
-----------------------------------------------*/

/*
	INIマネージャを初期化し、INIハンドルを得る
	
	@return IniUID
		>= 0: 有効なINIハンドル
		< 0 : 失敗
*/
IniUID inimgrNew( void );

/*
	通常のINIファイル仕様に存在しない拡張機能。
	これにより、key = paramという形式以外のデータや、
	INIマネージャを通さず直接ファイルを読んでデータを読むことによる省メモリを実現可能。
	
	ただし、次に出現するセクションまでの全ての行がコールバック関数に渡されるので、
	セクション名が細かく分かれてしまうことや、
	INIファイル中に全く異なるフォーマットのデータを唐突に出現させることができてしまうという
	デメリットもある。
	
	inimgrLoad()/inimgrSave()によるINIファイルの読み込み/保存時に、
	コールバック関数を呼び出すセクション名を指定する。
	
	inimgrLoad()/inimgrSave()は、これにより設定されたセクション名を探し当てると、
	コールバック関数を呼び出す。
	
	@param IniUID uid
		INIハンドル。
	
	@param const char *section
		コールバック関数と関連付けるセクション名。
	
	@param InimgrLoadCallback cb
		コールバック関数。
	
	@return
*/
int inimgrSetCallback( IniUID uid, const char *section, InimgrLoadCallback lcb, InimgrSaveCallback scb );

/*
	INIファイルを読み込み、設定内容をメモリに取り込む。
	
	@param IniUID uid
		INIハンドル。
	
	@param const char *inipath
		読み込むINIファイルのパス。
	
	@return int
		0  : 成功
		< 0: 失敗
*/
int  inimgrLoad( IniUID uid, const char *inipath );

/*
	メモリ上のINI情報をファイルへ書き出す。
	
	@param IniUID uid
		INIハンドル。
	
	@param const char *inipath
		書き込み先のINIファイルのパス。
		すでに存在するファイルであれば上書きし、存在しなければ作成する。
	
	@return int
		0  : 成功
		< 0: 失敗
*/
int  inimgrSave( IniUID uid, const char *inipath );

/*
	INIハンドルとそれに対応するINI情報を破棄する。
	対象のINIハンドルは無効になるため、使ってはいけない。
	
	@param IniUID uid
		INIハンドル。
*/
void inimgrDestroy( IniUID uid );

/*
	現在のINI情報に新しいセクションを追加する。
	
	@param IniUID uid
		INIハンドル。
	
	@param const char *section
		新しいセクション名。
	
	@return int
		0  : 成功
		< 0: 失敗
*/
int inimgrAddSection( IniUID uid, const char *section );
void inimgrDeleteSection( IniUID uid, const char *section );

/*
	INI情報から指定されたセクション内にある指定されたキーを検索し、
	それに関連づけられている値を数値で返す。
	
	セクション、あるいはキーが存在しない場合は第4引数のconst long defの値が返る。
	
	@param IniUID uid
		INIハンドル。
	
	@param const char *section
		検索対象のセクション名。
	
	@param const char *key
		検索対象のキー名。
	
	@param const long def
		検索対象が存在しない場合のデフォルト値。
	
	@return long
		検索対象の値。
*/	
long inimgrGetInt( IniUID uid, const char *section, const char *key, const long def );

/*
	INI情報から指定されたセクション内にある指定されたキーを検索し、
	それに関連づけられている値を文字列として指定されたバッファにコピーする。
	
	セクション、あるいはキーが存在しない場合は第4引数のconst char *defの文字列がコピーされる。
	
	@param IniUID uid
		INIハンドル。
	
	@param const char *section
		検索対象のセクション名。
	
	@param const char *key
		検索対象のキー名。
	
	@param const char *def
		検索対象が存在しない場合のデフォルト文字列。
	
	@param char *buf
		検索対象の文字列をコピーするバッファ。
	
	@param size_t bufsize
		上記のバッファのサイズ。
	
	@return unsigned int
		バッファにコピーされたバイト数。
*/
unsigned int inimgrGetString( IniUID uid, const char *section, const char *key, const char *def, char *buf, size_t bufsize );

/*
	INI情報から指定されたセクション内にある指定されたキーを検索し、
	それに関連づけられている値を文字列として扱い、
	ONと等しければtrueを、OFFと等しければfalseを返す。
	
	セクション、あるいはキーが存在しない、または値がON/OFFのいずれでもない場合は第4引数のconst bool defの値が返る。
	
	@param IniUID uid
		INIハンドル。
	
	@param const char *section
		検索対象のセクション名。
	
	@param const char *key
		検索対象のキー名。
	
	@param const bool def
		検索対象が存在しない場合のデフォルト値。
	
	@return bool
		検索対象の値。
*/	
bool inimgrGetBool( IniUID uid, const char *section, const char *key, const bool def );

int inimgrSetInt( IniUID uid, const char *section, const char *key, const long num );
int inimgrSetString( IniUID uid, const char *section, const char *key, const char *str );
int inimgrSetBool( IniUID uid, const char *section, const char *key, const bool on );

int inimgrCBReadln( FilehUID fuid, char *buf, size_t max );
int inimgrCBWriteln( FilehUID fuid, char *buf );

#ifdef __cplusplus
}
#endif

#endif
