/*=========================================================
	
	inimgr.h
	
	INIマネージャ。
	
	コールバック関数で低レベル操作ができすぎる気がする。
	もう少し抽象的にしたい。
	
	コールバックのサフィックスがださい。
	
=========================================================*/
#ifndef INIMGR_H
#define INIMGR_H

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "memory/dmem.h"
#include "file/fiomgr.h"
#include "util/strutil.h"
#include "cgerrs.h"

#ifdef PSP_USE_KERNEL_LIBC
#include "sysclib.h"
#endif
	
/*-----------------------------------------------
	定数
-----------------------------------------------*/
#define INIMGR_SECTION_BUFFER  64
#define INIMGR_ENTRY_BUFFER   255

#define INIMGR_ERROR_INVALID_SIGNATURE -1
#define INIMGR_ERROR_INVALID_VERSION   -2

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------
	型宣言
-----------------------------------------------*/
typedef uintptr_t IniUID;

typedef enum {
	INIMGR_CB_LOAD = 0,
	INIMGR_CB_SAVE
} InimgrCallbackMode;

struct inimgr_callback_info {
	char *section;
	SceOff offset;
	unsigned int length;
	struct inimgr_callback_info *next;
};

struct inimgr_callback {
	char *section;
	int cmplen;
	void *cb;
	void *arg;
	struct inimgr_callback_info *infoList;
	struct inimgr_callback *next;
};

struct inimgr_entry {
	char   *key;
	char   *value;
	size_t vspace;
	struct inimgr_entry *prev, *next;
};

struct inimgr_section {
	char   *name;
	struct inimgr_entry *entry;
	struct inimgr_section *prev, *next;
};

struct inimgr_params {
	DmemUID dmem;
	struct inimgr_callback *callbacks; /* コールバック設定 */
	struct inimgr_section  *index;     /* セクションのリンクリスト */
	struct inimgr_section  *last;      /* 常に終端のセクションを指す */
};
	
typedef struct {
	IniUID uid;
	FiomgrHandle ini;
	struct inimgr_callback_info *cbinfo;
} InimgrCallbackParams;

typedef int ( *InimgrCallback )( InimgrCallbackMode, InimgrCallbackParams*, char*, size_t, void* );

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
	通常のINIファイル実装に存在しない拡張機能。
	これにより、key = paramという形式以外のデータや、
	INIマネージャを通さず直接ファイルを読んでデータを読むことで省メモリにできる。
	
	このセクションを便宜上コールバックセクションと呼ぶ。
	
	次に出現するセクションまでの全てのデータはINIマネージャの手を離れるため、
	INIファイル中に全く異なるフォーマットのデータを唐突に出現させることができてしまうことや、
	セクションごとにしか設定できないため、セクションが細かく分かれてしまう場合があるなどのデメリットもある。
	
	inimgrLoad()/inimgrSave()によるINIファイルの読み込み/保存時に、
	コールバック関数を呼び出すセクション名を指定する。
	
	inimgrSave()は、これにより設定されたセクション名の記録を開始する時に、コールバック関数を呼び出す。
	コールバック関数はinimgrMakeEntry()やinimgrWriteln()などを使って設定値を書き出す。
	inimgrSave()から呼び出されたコールバック関数では、inimgrCbSeek*()を使ってはいけない。
	
	inimgrLoad()は、これにより設定されたセクション名を探し当てると、その位置を記録する。
	その後、一度INIファイルを全て読み出した後、コールバック関数を呼び出す。
	従って、コールバック関数内から、INIファイル中の他の設定値を呼び出すことは可能。
	ただし、他のコールバックセクションは発見した順に処理されるため、読めるかどうかはファイルの内容による。
	コールバック関数はinimgrReadln()やinimgrParseEntry()を使って必要な情報を抽出する。
	
	また、対象となるセクション名には、"*"というサフィックスをつけることができる。
	これをつけると、指定されたセクション名はセクション名のプリフィックスとして扱われる。
	仮に "MySection*" というセクション名を指定したとすると、"MySection" という文字列で始まるセクション名、
	たとえば、
	    MySection
	    MySection1
	    MySectionA
	    MySectionHoge
	    MySectionFoobar
	    MySection...
	などは、設定した同じコールバック関数が呼ばれる。
	
	一見ワイルドカードのように見えるが、``ワイルドカードではない''。
	あくまでサフィックスであり、"*MySection" や "My*Section" のような指定はできない。
	
	また、"*"という1文字をセクション名として指定すると全てのセクションで同じコールバック関数を呼ばせることができる。
	ただし、INIマネージャはコールバック関数の設定を調べるとき、設定された順に一致するセクション名を探し、
	最初に見つけたコールバック関数を実行して次のセクションに制御を移す。
	
	したがって、コールバック関数の設定順序で動作が以下のように異なる。
	    "MySection" にコールバック関数Aを設定後、"*" にコールバック関数Bを設定 -> "MySection" では関数Aが実行され、それ以外では関数Bが実行される。
	    "*" にコールバック関数Yを設定後、"MySection" にコールバック関数Zを設定 -> "MySection" を含む、全てのセクションで関数Yのみが実行される。
	
	@param IniUID uid
		INIハンドル。
	
	@param const char *section
		コールバック関数と関連付けるセクション名。
	
	@param InimgrCallback cb
		コールバック関数。
	
	@param void *arg
		コールバック関数に渡される引数。
	
	@return int
		0  : 成功
		< 0: 失敗
*/
int inimgrSetCallback( IniUID uid, const char *section, InimgrCallback cb, void *arg );

/*
	INIファイルを読み込み、設定内容をメモリに取り込む。
	設定内容が巨大な場合、ヒープを食いつぶすので注意。
	
	@param IniUID uid
		INIハンドル。
	
	@param const char *inipath
		読み込むINIファイルのパス。
	
	@param const char *sig
		チェックするシグネチャ。
		空白文字を含むことはできない。
		一致しない場合は読み込みを中断。
		NULLでシグネチャ、バージョンともにチェックしない。
	
	@param unsigned char major
		チェックするメジャーバージョン番号。
		シグネチャのチェックを通過してから判定される。
		この番号に満たない場合は読み込みを中断。
		
		後続の二つの番号も併せて、全てに0が指定されるとバージョンをチェックしない。
	
	@param unsigned char minor
		チェックするマイナーバージョン番号。
		メジャーバージョン番号のチェックを通過してから判定される。
		この番号に満たない場合は読み込みを中断。
	
	@param unsigned char rev
		チェックするリビジョン番号。
		マイナーバージョン番号のチェックを通過してから判定される。
		この番号に満たない場合は読み込みを中断。
	
	@return int
		0  : 成功
		< 0: 失敗
		-1 : 一致しないシグネチャ
		-2 : 不正なバージョン
*/
int inimgrLoad( IniUID uid, const char *inipath, const char *sig, unsigned char major, unsigned char minor, unsigned char rev );

/*
	メモリ上のINI情報をファイルへ書き出す。
	
	@param IniUID uid
		INIハンドル。
	
	@param const char *inipath
		書き込み先のINIファイルのパス。
		すでに存在するファイルであれば上書きし、存在しなければ作成する。
	
	@param const char *sig
		シグネチャ。
		空白文字を含むことはできない。
		NULLでシグネチャとバージョンを記録しない。
	
	@param unsigned char major
		メジャーバージョン番号。
		
		後続の二つの番号も併せて、全てに0が指定されるとバージョンを記録しない。
	
	@param unsigned char minor
		マイナーバージョン番号。
	
	@param unsigned char rev
		リビジョン番号。
	
	@return int
		0  : 成功
		< 0: 失敗
*/
int inimgrSave( IniUID uid, const char *inipath, const char *sig, unsigned char major, unsigned char minor, unsigned char rev );

/*
	INIハンドルとそれに対応するINI情報を破棄する。
	対象のINIハンドルは無効になるため、使ってはいけない。
	
	@param IniUID uid
		INIハンドル。
*/
void inimgrDestroy( IniUID uid );

/*
	現在のINI情報に新しいセクションを追加/削除する。
	セクション名は大文字と小文字を区別しない。
	
	また "__default"というセクション名は予約されているため使用できない。
	
	@param IniUID uid
		INIハンドル。
	
	@param const char *section
		セクション名。
	
	@return int
		0  : 成功
		< 0: 失敗
*/
int inimgrAddSection( IniUID uid, const char *section );
void inimgrDeleteSection( IniUID uid, const char *section );
bool inimgrExistSection( IniUID uid, const char *section );

/*
	INI情報から指定されたセクション内にある指定されたキーを検索し、
	それに関連づけられている値を取得する。
	
	セクション、あるいはキーが存在しない場合は偽が返り、値のセットは行わない。
	セクション名にNULLを渡すと、デフォルトセクションが指定されたものと見なす。
	
	@param IniUID uid
		INIハンドル。
	
	@param const char *section
		検索対象のセクション名。
	
	@param const char *key
		検索対象のキー名。
	
	@param int *var
		検索対象の値をセットする変数のポインタ。
	
	@return bool
		検索対象が見つかった場合に真、そうでなければ偽。
*/	
bool inimgrGetInt( IniUID uid, const char *section, const char *key, int *var );

/*
	INI情報から指定されたセクション内にある指定されたキーを検索し、
	それに関連づけられている値を文字列として指定されたバッファにコピーする。
	
	セクション、あるいはキーが存在しない場合は-1が返り、値のセットは行わない。
	セクション名にNULLを渡すと、デフォルトセクションが指定されたものと見なす。
	
	@param IniUID uid
		INIハンドル。
	
	@param const char *section
		検索対象のセクション名。
	
	@param const char *key
		検索対象のキー名。
	
	@param char *buf
		検索対象の文字列をコピーするバッファ。
	
	@param size_t bufsize
		上記のバッファのサイズ。
	
	@return unsigned int
		バッファにコピーされたバイト数。
*/
int inimgrGetString( IniUID uid, const char *section, const char *key, char *buf, size_t bufsize );

/*
	INI情報から指定されたセクション内にある指定されたキーを検索し、
	それに関連づけられている値を文字列として扱い、
	ONと等しければtrueを、OFFと等しければfalseを返す。
	
	ON/OFFは大文字と小文字を区別しない。
	
	セクション、あるいはキーが存在しない、または値がON/OFFのいずれでもない場合は偽が返り、値のセットは行わない。
	セクション名にNULLを渡すと、デフォルトセクションが指定されたものと見なす。
	
	@param IniUID uid
		INIハンドル。
	
	@param const char *section
		検索対象のセクション名。
	
	@param const char *key
		検索対象のキー名。
	
	@param const bool *var
		検索対象の値をセットする変数のポインタ。
	
	@return bool
		検索対象が見つかった場合に真、そうでなければ偽。
*/	
bool inimgrGetBool( IniUID uid, const char *section, const char *key, bool *var );

/*
	inimgrSetInt
	inimgrSetString
	inimgrSetBool
	
	セクションに対してキーを設定する。
	すでに存在する場合はキーの値を更新する。
	
	キー名は大文字と小文字を区別しない。
	セクション名にNULLを渡すと、デフォルトセクションが指定されたものと見なす。
*/
int inimgrSetInt( IniUID uid, const char *section, const char *key, const long num );
int inimgrSetString( IniUID uid, const char *section, const char *key, const char *str );
int inimgrSetBool( IniUID uid, const char *section, const char *key, const bool on );

inline IniUID inimgrCbGetIniHandle( InimgrCallbackParams *params );
int inimgrCbReadln( InimgrCallbackParams *params, char *buf, size_t buflen );
int inimgrCbSeekSet( InimgrCallbackParams *params, SceOff offset );
int inimgrCbSeekCur( InimgrCallbackParams *params, SceOff offset );
int inimgrCbSeekEnd( InimgrCallbackParams *params, SceOff offset );
int inimgrCbTell( InimgrCallbackParams *params );
int inimgrCbWriteln( InimgrCallbackParams *params, char *buf, size_t buflen );
bool inimgrParseEntry( char *entry, char **key, char **value );
int inimgrMakeEntry( char *entry, size_t len, char *key, char *value );
int inimgrMakeSection( char *buf, size_t len, char *section );



#ifdef __cplusplus
}
#endif

#endif
