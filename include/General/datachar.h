#ifndef datachar_H
#define datachar_H

/*
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		Nov 2000
 Contents:	Binary data interpretation
 RCS:		$Id: datachar.h,v 1.2 2001-02-19 11:28:45 bert Exp $
________________________________________________________________________

*/


#include <bindatadesc.h>


/*!\brief byte-level data characteristics of stored data.

Used for the interpretation (read or write) of data in buffers that are read
directly from disk into buffer. In that case cross-platform issues arise:
byte-ordering and int/float layout.
The Ibm Format is only supported for the types that are used in SEG-Y sample
data handling. SGI is a future option.

*/

#define mDeclConstr(T,ii,is) \
DataCharacteristics( const T* ) \
: BinDataDesc(ii,is,sizeof(T)), fmt(Ieee), littleendian(__islittle__) {} \
DataCharacteristics( const T& ) \
: BinDataDesc(ii,is,sizeof(T)), fmt(Ieee), littleendian(__islittle__) {}


class DataCharacteristics : public BinDataDesc
{
public:

    enum Format		{ Ieee, Ibm };

    Format		fmt;
    bool		littleendian;

			DataCharacteristics( bool ii=false, bool is=true,
					     ByteCount n=N4, Format f=Ieee,
					     bool l=__islittle__ )
			: BinDataDesc(ii,is,n)
			, fmt(f), littleendian(l)		{}

    inline bool		isIeee() const		{ return fmt == Ieee; }

			DataCharacteristics( unsigned short c )	{ set(c); }
			DataCharacteristics( const char* s )	{ set(s); }

    virtual unsigned short	dump() const;
    virtual BufferString	toString() const;
    virtual void		set(unsigned short);
    virtual void		set(const char*);

			mDeclConstr(signed char,true,true)
			mDeclConstr(short,true,true)
			mDeclConstr(int,true,true)
			mDeclConstr(long long,true,true)
			mDeclConstr(unsigned char,true,false)
			mDeclConstr(unsigned short,true,false)
			mDeclConstr(unsigned int,true,false)
			mDeclConstr(unsigned long long,true,false)
			mDeclConstr(float,false,true)
			mDeclConstr(double,false,true)

    bool		operator ==( const DataCharacteristics& dc ) const
			{ return isEqual(dc); }
    bool		operator !=( const DataCharacteristics& dc ) const
			{ return !isEqual(dc); }

    bool		needSwap() const
			{ return (int)nrbytes > 1
			      && littleendian != __islittle__; }

};

#undef mDeclConstr

#endif
