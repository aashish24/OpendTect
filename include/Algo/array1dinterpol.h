#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          December 2009
________________________________________________________________________


-*/

#include "algomod.h"
#include "arraynd.h"
#include "executor.h"
#include "uistrings.h"


/*!\brief Base class for one dimensional array interpolators. */

mExpClass(Algo) Array1DInterpol : public Executor
{ mODTextTranslationClass(Array1DInterpol);
public:

    typedef float		ValT;
    typedef Array1D<float>	ArrT;
    typedef ArrT::IdxType	IdxType;
    typedef ArrT::SzType	SzType;

    virtual		~Array1DInterpol();

    void		setMaxGapSize( SzType sz )	{ maxgapsize_ = sz; }
    SzType		getMaxGapSize() const		{ return maxgapsize_; }

    void		setArray(Array1D<float>&);

    uiString		nrDoneText() const
			{ return uiStrings::sPointsDone(); }
    od_int64		nrDone() const		{ return nrdone_; }
    od_int64		nrIterations() const;
    void		setExtrapol( bool yn )	{ doextrapol_ = yn; }
    void		setFillWithExtremes( bool yn )
			{ fillwithextremes_ = yn; }
    void		reset();

protected:
			Array1DInterpol();

    Array1D<float>*	arr_;
    bool		arrstarted_;
    bool		doextrapol_;
    bool		fillwithextremes_; //extrapolate with last valid values
    SzType		maxgapsize_;
    SzType		nrdone_;

};


/*!\brief Does linear interpolation of one dimensional arrays. */

mExpClass(Algo) LinearArray1DInterpol : public Array1DInterpol
{ mODTextTranslationClass(LinearArray1DInterpol);
public:

			LinearArray1DInterpol();

    uiString		message() const
			{ return tr("Interpolating points (Linear)"); }
protected:

    int			nextStep();
    void		extrapolate(bool start);

};


/*!\brief Uses a 3rd degree polynomial for interpolation of one dimensional
  arrays.  */

mExpClass(Algo) PolyArray1DInterpol : public Array1DInterpol
{ mODTextTranslationClass(PolyArray1DInterpol);
public:

			PolyArray1DInterpol();

    uiString		message() const
			{ return tr("Interpolating points (Polynomial)"); }
    int			nextStep();

protected:

    bool		getPositions(IdxType pos,TypeSet<IdxType>& posidxs);
    void		extrapolate(bool start);

};
