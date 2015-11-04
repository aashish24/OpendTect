#ifndef trckeysampling_h
#define trckeysampling_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id$
________________________________________________________________________

-*/

#include "basicmod.h"
#include "binid.h"
#include "ranges.h"
#include "typeset.h"
#include "trckey.h"

typedef TypeSet<TrcKey> TrcKeyPath;

/*!
\brief Horizontal sampling (inline and crossline range and steps).
*/

mExpClass(Basic) TrcKeySampling
{
public:

			TrcKeySampling();
			TrcKeySampling(const TrcKeySampling&);
			TrcKeySampling(Pos::GeomID);

    TrcKeySampling&	set(const Interval<int>& linerg,
			    const Interval<int>& trcnrrg);
			    //!< steps copied if available
    void		get(Interval<int>& linerg,Interval<int>& trcnrrg) const;
			    //!< steps filled if available

    StepInterval<int>	lineRange() const;
    StepInterval<int>	trcRange() const;
    float		lineDistance() const;
			/*!< real world distance between 2 lines incremented by
			     one times the step_ */
    float		trcDistance() const;
			/*!< real world distance between 2 traces incremented by
			     one times the step_ */
    void		setLineRange(const Interval<int>&);
    void		setTrcRange(const Interval<int>&);

    bool		includes(const TrcKeySampling&,
				 bool ignoresteps=false) const;
    bool		includes(const TrcKey&) const;
    bool		lineOK(Pos::LineID) const;
    bool		trcOK(Pos::TraceID) const;

    void		include(const TrcKey&);
    void		includeLine(Pos::LineID);
    void		includeTrc(Pos::TraceID);
    void		include(const TrcKeySampling&, bool ignoresteps=false );
    bool		isDefined() const;
    void		limitTo(const TrcKeySampling&,bool ignoresteps=false);
    void		limitToWithUdf(const TrcKeySampling&);
			    /*!< handles undef values +returns reference HS
				 nearest limit if HS's do not intersect */
    void		expand(int nrlines,int nrtrcs);

    int			lineIdx(Pos::LineID) const;
    int			trcIdx(Pos::TraceID) const;

    od_int64		globalIdx(const TrcKey&) const;
    od_int64		globalIdx(const BinID&) const;
    BinID		atIndex(int i0,int i1) const;
    BinID		atIndex(od_int64 globalidx) const;
    TrcKey		trcKeyAt(int i0,int i1) const;
    TrcKey		trcKeyAt(od_int64 globalidx) const;
    TrcKey		toTrcKey(const Coord&,float* distance=0) const;
    Coord		toCoord(const BinID&) const;
    TrcKey		center() const;
    int			nrLines() const;
    int			nrTrcs() const;
    od_int64		totalNr() const;
    bool		isEmpty() const;
    void		neighbors(od_int64 globalidx,TypeSet<od_int64>&) const;
    void		neighbors(const TrcKey&,TypeSet<TrcKey>&) const;

    void		init(bool settoSI=true);
			//!< Sets to survey values or mUdf(int) (but step 1)
    bool		init(Pos::GeomID);

    void		set2DDef();
			    //!< Sets ranges to 0-maxint
    void		normalise();
			    //!< Makes sure start_<stop_ and steps are non-zero
    void		getRandomSet(int nr,TypeSet<TrcKey>&) const;

    bool		getInterSection(const TrcKeySampling&,
					TrcKeySampling&) const;
			    //!< Returns false if intersection is empty

    TrcKey		getNearest(const TrcKey&) const;
			    /*!< step_-snap and outside -> edge.
				Assumes inldist == crldist */
    void		snapToSurvey();
			    /*!< Checks if it is on valid bids. If not, it will
				 expand until it is */

    bool		operator==(const TrcKeySampling&) const;
    bool		operator!=(const TrcKeySampling&) const;
    TrcKeySampling&	operator=(const TrcKeySampling&);

    bool		usePar(const IOPar&);	//!< Keys as in keystrs.h
    void		fillPar(IOPar&) const;	//!< Keys as in keystrs.h
    static void		removeInfo(IOPar&);
    void		toString(BufferString&) const; //!< Nice text for info

    Pos::SurvID		survid_;
    BinID		start_;
    BinID		stop_;
    BinID		step_;

    //Legacy. Will be removed
			TrcKeySampling(bool settoSI);
    StepInterval<int>	inlRange() const	{ return lineRange(); }
    StepInterval<int>	crlRange() const	{ return trcRange(); }
    void		setInlRange(const Interval<int>& rg) {setLineRange(rg);}
    void		setCrlRange(const Interval<int>& rg) {setTrcRange(rg);}

    int			nrInl() const { return nrLines(); }
    int			nrCrl() const { return nrTrcs(); }

    int			inlIdx( Pos::LineID lid ) const {return lineIdx(lid);}
    int			crlIdx( Pos::TraceID tid ) const { return trcIdx(tid); }
    inline void		include( const BinID& bid )
			{ includeLine(bid.inl()); includeTrc(bid.crl()); }
    void		includeInl( int inl ) { includeLine(inl); }
    void		includeCrl( int crl ) { includeTrc(crl); }
    inline bool		includes( const BinID& bid ) const
			{ return lineOK(bid.inl()) && trcOK(bid.crl()); }
    inline bool		inlOK( int inl ) const { return lineOK(inl); }
    inline bool		crlOK( int crl ) const { return trcOK(crl); }

    mDeprecated BinID&	start;
    mDeprecated BinID&	stop;
    mDeprecated BinID&	step;
};


mExpClass(Basic) TrcKeySamplingSet : public TypeSet<TrcKeySampling>
{
public:

    void			isOK() const;
    void			add(Pos::GeomID);
    bool			isPresent(Pos::GeomID);
};




/*!
\brief Finds next BinID in TrcKeySampling; initializes to first position.
*/

mExpClass(Basic) TrcKeySamplingIterator
{
public:
		TrcKeySamplingIterator() : tks_( true ) { reset(); }
		TrcKeySamplingIterator( const TrcKeySampling& hs )
		    : tks_(hs)	{ reset(); }

    void	setSampling( const TrcKeySampling& tks )
		{ tks_ = tks; reset(); }

    void	reset();
    void	setNextPos(const TrcKey& trk) { curpos_ = tks_.globalIdx(trk); }
    bool	next(TrcKey&) const;
    bool	next(BinID&) const;

    od_int64	curIdx() const		     { return curpos_; }
    TrcKey	curTrcKey() const 	     { return tks_.atIndex( curIdx() );}

protected:

    TrcKeySampling			tks_;
    od_int64				totalnr_;
    mutable Threads::Atomic<od_int64>	curpos_;
};




typedef TrcKeySampling HorSampling;
typedef TrcKeySamplingIterator	HorSamplingIterator;


inline int TrcKeySampling::lineIdx( Pos::LineID line ) const
{
    return step_.lineNr()
	? (line-start_.lineNr()) / step_.lineNr()
	: (line==start_.lineNr() ? 0 : -1);
}


inline int TrcKeySampling::trcIdx( Pos::TraceID trcid ) const
{
    return step_.trcNr()
	? (trcid-start_.trcNr()) / step_.trcNr()
	: (trcid==start_.trcNr() ? 0 : -1);
}




#endif

