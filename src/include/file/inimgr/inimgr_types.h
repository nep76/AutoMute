/* inimgr_types.h                 */
/* 内部で使用。外から参照しない。 */

#include "../inimgr.h"
#include "sysclib.h"

/* INIMGR_SECTION_BUFFER - 1 以下でなければならない */
#define INIMGR_DEFAULT_SECTION_NAME "__default"

/* シグネチャ、バージョン 合わせて INIMGR_ENTRY_BUFFER - 2 以下でなければならない */
#define INIMGR_SIGNATURE_BUFFER ( INIMGR_ENTRY_BUFFER - 66 )
#define INIMGR_VERSION_BUFFER   ( INIMGR_ENTRY_BUFFER - INIMGR_SIGNATURE_BUFFER - 2 )

enum inimgr_line_type {
	INIMGR_LINE_NULL = 0,
	INIMGR_LINE_SECTION,
	INIMGR_LINE_ENTRY
};

char *__inimgr_get_value( struct inimgr_params *params, const char *section, const char *key );
int __inimgr_set_value( struct inimgr_params *params, const char *section, const char *key, const char *value );
struct inimgr_section *__inimgr_find_section( struct inimgr_params *params, const char *section );
struct inimgr_entry *__inimgr_find_entry( struct inimgr_section *section, const char *key );
struct inimgr_callback *__inimgr_find_callback( struct inimgr_params *params, const char *section );
struct inimgr_section *__inimgr_create_section( struct inimgr_params *params, const char *section );
struct inimgr_entry *__inimgr_create_entry( struct inimgr_params *params, const char *key, const char *value );
void __inimgr_delete_section( struct inimgr_params *params, struct inimgr_section *section );
void __inimgr_delete_entry( struct inimgr_params *params, struct inimgr_section *section, struct inimgr_entry *entry );
