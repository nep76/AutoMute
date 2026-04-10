/*=========================================================
	
	string.c
	
	pspsysclib用のstring.h代用品。
	PSPSDKのコードを参考にしています。
	
=========================================================*/
#include <string.h>

char *strupr( char *s )
{
	while( *s != '\0' ){
		*s = toupper( *s );
		s++;
	}
	return s;
}

char *strlwr( char *s )
{
	while( *s != '\0' ){
		*s = tolower( *s );
		s++;
	}
	return s;
}

int strcasecmp( const char *s1, const char *s2 )
{
	register char ss1, ss2;
	
	while( ( ss1 = toupper( *s1 ) ) == ( ss2 = toupper( *s2 ) ) ){
		if( *s1 == '\0' ){
			return 0;
		} else{
			s1++;
			s2++;
		}
	}
	
	return ss1 - ss2;
}

int strncasecmp( const char *s1, const char *s2, size_t n )
{
	register char ss1, ss2;
	
	if( ! n ) return 0;
	
	n--;
	while( ( ss1 = toupper( *s1 ) ) == ( ss2 = toupper( *s2 ) ) ){
		if( *s1 == '\0' || ! n ){
			return 0;
		} else{
			n--;
			s1++;
			s2++;
		}
	}
	
	return ss1 - ss2;
}

int isupper( int c )
{
	if( c >= 'A' && c <= 'Z' ) return 1;
	return 0;
}

int islower( int c )
{
	if( c >= 'a' && c <= 'z' ) return 1;
	return 0;
}

int isalpha( int c )
{
	if( islower( c ) || isupper( c ) ) return 1;
	return 0;
}

int isdigit( int c )
{
	if( c >= '0' && c <= '9' ) return 1;
	return 0;
}

int isalnum( int c )
{
	if ( isalpha( c ) || isdigit( c ) ) return 1;
	return 0;
}

int isspace( int c )
{
	if( ( c >= 0x09 && c <= 0x0D ) || c == 0x20 ) return 1;
	return 0;
}

int iscntrl( int c ) 
{
	if( c < 0x20 || c == 0x7F ) return 1;
	return 0;
}

int isgraph( int c )
{
	if( iscntrl( c ) || isspace( c ) ) return 0;
	return 1;
}

int isprint( int c )
{
	if( ! iscntrl( c ) ) return 1;
	return 0;
}

int ispunct( int c )
{
	if( iscntrl( c ) || isalnum( c ) || isspace( c ) ) return 0;
	return 1;
}

int isxdigit( int c )
{
	if ( isdigit( c ) || ( toupper( c ) >= 'A' && toupper( c ) <= 'F' ) ) return 1;
	return 0;
}
