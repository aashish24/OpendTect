#ifndef periodicvalue_h
#define periodicvalue_h

/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kris Tingdahl
 Date:		12-4-1999
 Contents:	Periodic value interpolation and so forth
 RCS:		$Id: periodicvalue.h,v 1.2 2003-11-07 12:21:50 bert Exp $
________________________________________________________________________

*/

#include <simpnumer.h>
#include <interpol.h>


/*!>
 dePeriodize returns a periodic (defined by y(x) = y(x) + N * P) value's value
 in the functions first period (between 0 and P).
*/

template <class T>
inline T dePeriodize( T val, T period )
{
    int n = (int) (val / period);
    if ( val < 0 ) n--;

    return n ? val - n * period : val; 
}

/*!>
  \brief
  PeriodicValue handles periodic data through mathematical operations.
*/


template <class T,int P>
class PeriodicValue
{
public:
    T				val(bool positive=true) const
				{
				    T res = dePeriodize(val_,(T)P);
				    if ( !positive && res > ((T)P)/2 )
					return res-P;
				    return res;
				}
				/*!< Returns the value between 0 and P if
				     positive is true, or between -P/2 and P/2
				     if positive is false;
				*/

    PeriodicValue<T,P>		operator+(T nv) const
				{ return PeriodicValue<T,P>(val_+nv); }
    PeriodicValue<T,P>		operator-(T nv) const
				{ return PeriodicValue<T,P>(val_-nv); }
    PeriodicValue<T,P>		operator*(T nv) const
				{ return PeriodicValue<T,P>(val_*nv); }
    PeriodicValue<T,P>		operator/(T nv) const
				{ return PeriodicValue<T,P>(val_/nv); }
    PeriodicValue<T,P>		operator+(const PeriodicValue<T,P>& nv) const
				{ return PeriodicValue<T,P>(val_+nv.val()); }
    PeriodicValue<T,P>		operator-(const PeriodicValue<T,P>& nv) const
				{ return PeriodicValue<T,P>(val_-nv.val()); }
    PeriodicValue<T,P>		operator*(const PeriodicValue<T,P>& nv) const
				{ return PeriodicValue<T,P>(val_*nv.val()); }
    PeriodicValue<T,P>		operator/(const PeriodicValue<T,P>& nv) const
				{ return PeriodicValue<T,P>(val_/nv.val()); }

    const PeriodicValue<T,P>&	operator=(T nv) const
				{ val_ = nv; return this; }
    const PeriodicValue<T,P>&	operator+=(T nv)
				{ val_ += nv; return this; }
    const PeriodicValue<T,P>&	operator-=(T nv)
				{ val_ -= nv; return this; }
    const PeriodicValue<T,P>&	operator*=(T nv)
				{ val_ *= nv; return this; }
    const PeriodicValue<T,P>&	operator/=(T nv)
				{ val_ /= nv; return this; }
    const PeriodicValue<T,P>&	operator=(const PeriodicValue<T,P>& nv) const
				{ val_ = nv.val(); return this; }
    const PeriodicValue<T,P>&	operator+=(const PeriodicValue<T,P>& nv)
				{ val_ += nv.val(); return this; }
    const PeriodicValue<T,P>&	operator-=(const PeriodicValue<T,P>& nv)
				{ val_ -= nv.val(); return this; }
    const PeriodicValue<T,P>&	operator*=(const PeriodicValue<T,P>& nv)
				{ val_ *= nv.val(); return this; }
    const PeriodicValue<T,P>&	operator/=(const PeriodicValue<T,P>& nv)
				{ val_ /= nv.val(); return this; }

    bool			operator<(const PeriodicValue<T,P>& b) const
				{
				    PeriodicValue<T,P> tmp = *this-b;
				    if ( tmp.val(true)>P/2 ) return true;
				    return false;
				}
    bool			operator>(const PeriodicValue<T,P>& b) const
				{
				    PeriodicValue<T,P> tmp = *this-b;
				    if ( tmp.val(true)<=P/2 ) return true;
				    return false;
				}
    bool			operator<(T b) const
				{ return *this < PeriodicValue<T,P>(b); }
    bool			operator>(T b) const
				{ return *this > PeriodicValue<T,P>(b); }
				

				PeriodicValue(T nv) : val_( nv ) {}

protected:
    T				val_;
};


/*!>
 interpolateYPeriodicSampled interpolates in an indexable storage with
 periodic entities ( defined by y(x) = y(x) + N*P )
*/

template <class T, class RT>
inline void interpolateYPeriodicSampled( const T& idxabl, int sz, float pos,
				RT& ret, RT period,
				bool extrapolate=NO,
				RT undefval=(RT)mUndefValue )
{
    const float halfperiod = period / 2;
    int intpos = mNINT( pos );
    float dist = pos - intpos;
    if( mIsZero(dist) && intpos >= 0 && intpos < sz ) 
	{ ret = idxabl[intpos]; return; }

    int prevpos = dist > 0 ? intpos : intpos - 1;
    if ( !extrapolate && (prevpos > sz-2 || prevpos < 0) )
	ret = undefval;
    else if ( prevpos < 1 )
    {
	const float val0 = idxabl[0];
	RT val1 = idxabl[1];
	while ( val1 - val0 > halfperiod ) val1 -= period; 
	while ( val1 - val0 < -halfperiod ) val1 += period; 

	ret = dePeriodize(linearInterpolate( val0, val1, pos ), period );
    }
    else if ( prevpos > sz-3 )
    {
	const RT val0 = idxabl[sz-2];
	RT val1 = idxabl[sz-1];
	while ( val1 - val0 > halfperiod ) val1 -= period; 
	while ( val1 - val0 < -halfperiod ) val1 += period; 
	ret = dePeriodize(linearInterpolate( val0, val1, pos-(sz-2) ), period );
    }
    else
    {
	const RT val0 = idxabl[prevpos-1];

	RT val1 = idxabl[prevpos];
	while ( val1 - val0 > halfperiod ) val1 -= period; 
	while ( val1 - val0 < -halfperiod ) val1 += period; 

	RT val2 = idxabl[prevpos+1];
	while ( val2 - val1 > halfperiod ) val2 -= period; 
	while ( val2 - val1 < -halfperiod ) val2 += period; 

	RT val3 = idxabl[prevpos+2];
	while ( val3 - val2 > halfperiod ) val3 -= period; 
	while ( val3 - val2 < -halfperiod ) val3 += period; 

	ret = dePeriodize(polyInterpolate( val0, val1, val2, val3,
			  pos - prevpos ), period );
    }
}


template <class T>
inline float interpolateYPeriodicSampled( const T& idxabl, int sz, float pos,
                                 float period, bool extrapolate=NO,
                                 float undefval=mUndefValue )
{
    float ret = undefval;
    interpolateYPeriodicSampled( idxabl, sz, pos, ret,
				 period, extrapolate, undefval );
    return ret;
}


/*!>
 interpolateXPeriodicSampled interpolates in an periodic indexable ( where
 the position is periodic ), defined by y(x) = x(x+n*P). The period is equal
 to the size of the given idxabl.
*/

template <class T, class RT>
inline void interpolateXPeriodicSampled( const T& idxabl, int sz, float pos,
					RT& ret)
{
    int intpos = mNINT( pos );
    float dist = pos - intpos;
    if( mIsZero(dist) && intpos >= 0 && intpos < sz ) 
	{ ret = idxabl[intpos]; return; }

    int prevpos = dist > 0 ? intpos : intpos - 1;
    const float relpos = pos - prevpos;
    prevpos = dePeriodize( prevpos, sz );

    int prevpos2 = prevpos - 1; 
    prevpos2 = dePeriodize( prevpos2, sz );

    const int nextpos = prevpos + 1;
    nextpos = dePeriodize( nextpos, sz );

    const int nextpos2 = prevpos + 2;
    nextpos2 = dePeriodize( nextpos2, sz );

    const RT prevval2 = idxabl[prevpos2];
    const RT prevval = idsabl[prevpos];
    const RT nextval = idxabl[nextpos];
    const RT nextval2 = idxabl[nextpos2];

    ret = polyInterpolate( prevval2, 
			   prevval, 
			   nextval,
			   nextval2, relpos );
}


#endif
