/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 11-4-1994
 * FUNCTION : Functions concerning delimiter separated string lists
-*/

static const char* rcsID = "$Id: separstr.cc,v 1.7 2003-11-07 12:21:57 bert Exp $";

#include <string.h>
#include <stdlib.h>
#include "separstr.h"
#include "string2.h"

SeparString::SeparString( const char* str, char separ )
{
    rep = str ? str : "";
    sep = separ;
    sepstr[0] = separ;
    sepstr[1] = '\0';
}


const char* SeparString::operator[]( unsigned int elemnr ) const
{
    static char buf_[mMaxSepItem+1];
    char* bufptr = buf_;
    const char* repptr = rep;
    buf_[0] = '\0';

    while ( *repptr )
    {
	if ( !elemnr )
	    *bufptr = *repptr;

	if ( *repptr == sep )
	{
	    if ( !elemnr || bufptr-buf_ == mMaxSepItem )
	    {
		*bufptr = '\0';
		return buf_;
	    }
	    elemnr--;
	}
	else if ( !elemnr )
	    bufptr++;

	repptr++;
    }

    *bufptr = '\0';
    return buf_;
}


const char* SeparString::from( unsigned int idx ) const
{
    const char* ptr = rep;
    for ( ; idx!=0; idx-- )
    {
	ptr = strchr( ptr, sep );
	if ( ptr ) ptr++;
    }
    return ptr;
}


void SeparString::add( const char* str )
{
    rep += sepstr;
    rep += str;
}



SeparString& SeparString::operator += ( const char* str )
{
    if ( str )
    {
	if ( *rep ) rep += sepstr;
	rep += str;
    }
    return *this;
}


unsigned int SeparString::size() const
{
    if ( !*rep ) return 0;

    unsigned int idx = *rep == sep ? 1 : 0;
    const char* ptr = rep;
    while ( ptr )
    {
	idx++;
	ptr = strchr( ptr+1, sep );
    }

    return idx;
}


SeparString& SeparString::operator +=( int i )
{
    *this += getStringFromInt( 0, i );
    return *this;
}


SeparString& SeparString::operator +=( float f )
{
    *this += getStringFromFloat( 0, f );
    return *this;
}


SeparString& SeparString::operator +=( double d )
{
    *this += getStringFromDouble( 0, d );
    return *this;
}
