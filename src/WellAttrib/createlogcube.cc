/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		June 2011
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: createlogcube.cc,v 1.13 2012-06-28 11:57:40 cvsbruno Exp $";

#include "createlogcube.h"

#include "seistrc.h"
#include "seiswrite.h"
#include "survinfo.h"
#include "ctxtioobj.h"
#include "welldata.h"
#include "welld2tmodel.h"
#include "wellextractdata.h"
#include "welllog.h"
#include "welllogset.h"
#include "welltrack.h"
#include "stattype.h"


LogCubeCreator::LogCubeData::~LogCubeData()
{
    delete seisctio_.ioobj;
}


LogCubeCreator::LogCubeCreator( const Well::Data& wd )
    : wd_(wd)
    , hrg_(false)
    , nrduplicatetrcs_(0)		 
{
    Well::SimpleTrackSampler wtextr( wd_.track(), wd_.d2TModel() );
    if ( !wtextr.execute() )
	pErrMsg( "unable to extract position" );
    wtextr.getBIDs( binids_ );
    extractparams_.setFixedRange( SI().zRange( true ), true );
}


LogCubeCreator::~LogCubeCreator()
{
    deepErase( logdatas_ );
}


void LogCubeCreator::setInput( ObjectSet<LogCubeData>& lcds, int nrdupltrcs )
{
    while ( !lcds.isEmpty() )
	logdatas_ += lcds.remove(0);

    nrduplicatetrcs_ = nrdupltrcs;
}


void LogCubeCreator::setInput( ObjectSet<LogCubeData>& lcds, int nrdupltrcs,
			const Well::ExtractParams& pars )
{
    setInput( lcds, nrdupltrcs );
    extractparams_ = pars;
}


#define mErrRet(msg)\
{ errmsg_= msg; errmsg_ += " for "; errmsg_ += wd_.name(); return false; }
bool LogCubeCreator::doPrepare( int )
{
    if ( binids_.isEmpty() )
	mErrRet( "No valid position extracted along the track" );

    hrg_ = HorSampling( false );
    for ( int idx=0; idx<binids_.size(); idx++ )
	hrg_.include( binids_[idx] );

    BinID bidvar( nrduplicatetrcs_-1, nrduplicatetrcs_-1 );
    hrg_.stop += bidvar;
    hrg_.start -= bidvar;
    hrg_.snapToSurvey();

    extractparams_.zstep_ = SI().zRange( true ).step;
    extractparams_.extractzintime_ = true;
    
    return true;
}


bool LogCubeCreator::doWork( od_int64 start, od_int64 stop, int )
{
    if ( SI().zIsTime() && !wd_.haveD2TModel() )
	mErrRet( "No depth/time model found" );

    for ( int idx=start; idx<=stop; idx++ )
    {
	if ( !shouldContinue() )
	    return false;

	if ( !writeLog2Cube( *logdatas_[idx] ) )
	{
	    errmsg_ = "One or several logs could not be written"; 
	    errmsg_ += " for "; errmsg_ += wd_.name(); 
	}

	addToNrDone( 1 );
    }
    return true;
}


bool LogCubeCreator::writeLog2Cube( const LogCubeData& lcd ) const
{
    SeisTrcWriter writer( lcd.seisctio_.ioobj );

    SeisTrc trc( SI().zRange(true).nrSteps()+1 );
    trc.info().sampling = SI().zRange(true);

    BufferStringSet lognms; lognms.add( lcd.lognm_ );
    Well::LogSampler ls( wd_, extractparams_, lognms );
    if ( !ls.execute( false ) )
	return false;

    for ( int idz=0; idz<trc.size(); idz++ )
	trc.set( idz, ls.getLogVal( 0, idz ), 0 );

    HorSamplingIterator hsit( hrg_ );
    bool succeeded = true;
    BinID bid;
    while ( hsit.next( bid ) )
    {
	trc.info().binid = bid;
	if ( !writer.put(trc) )
	    { pErrMsg( "cannot write new trace" ); succeeded = false; }
    }
    return succeeded;
}


const char* LogCubeCreator::errMsg() const 
{ return errmsg_.isEmpty() ? 0 : errmsg_.buf(); }
