/* inimgrCbGetIniHandle.c */

#include "inimgr_types.h"

inline IniUID inimgrCbGetIniHandle( InimgrCallbackParams *params )
{
	return params->uid;
}
