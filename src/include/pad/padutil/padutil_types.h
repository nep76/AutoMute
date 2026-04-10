/* padutil_types.h                */
/* 内部で使用。外から参照しない。 */

#include "../padutil.h"

#include "graphic/pb.h"

#define PADUTIL_INVALID_RIGHT_TRIANGLE_DEGREE 45
#define PADUTIL_DEGREE_TO_RADIAN( d )         ( ( d ) *  ( 3.14 / 180 ) )
#define PADUTIL_SQUARE( x )                   ( ( x ) * ( x ) )
#define PADUTIL_IN_DEADZONE( x, y, r )        ( PADUTIL_SQUARE( x ) + PADUTIL_SQUARE( y ) <= ( PADUTIL_SQUARE( r ) ) )
