/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          Feb 2005
 RCS:           $Id: horizonscanner.cc,v 1.23 2008-06-18 06:22:21 cvsraman Exp $
________________________________________________________________________

-*/

#include "horizonscanner.h"
#include "binidvalset.h"
#include "emhorizon3d.h"
#include "posgeomdetector.h"
#include "iopar.h"
#include "strmprov.h"
#include "survinfo.h"
#include "oddirs.h"
#include "cubesampling.h"
#include "keystrs.h"
#include "tabledef.h"


HorizonScanner::HorizonScanner( const BufferStringSet& fnms,
				Table::FormatDesc& fd, bool isgeom )
    : Executor("Scan horizon file(s)")
    , geomdetector_(*new PosGeomDetector(true))
    , fd_(fd)
    , isgeom_(isgeom)
    , ascio_(0)
    , isxy_(false)
    , selxy_(false)
    , bvalset_(0)
    , fileidx_(0)
{
    filenames_ = fnms;
    init();
}


HorizonScanner::~HorizonScanner()
{
    delete &geomdetector_;
}


void HorizonScanner::init()
{
    totalnr_ = -1;
    firsttime_ = true;
    valranges_.erase();
    geomdetector_.reInit();
    analyzeData();
}


const char* HorizonScanner::message() const
{
    return "Scanning";
}


const char* HorizonScanner::nrDoneText() const
{
    return "Positions handled";
}


int HorizonScanner::nrDone() const
{
    return geomdetector_.nrpositions;
}


int HorizonScanner::totalNr() const
{
    if ( totalnr_ > 0 ) return totalnr_;

    totalnr_ = 0;
    for ( int idx=0; idx<filenames_.size(); idx++ )
    {
	StreamProvider sp( filenames_.get(0).buf() );
	StreamData sd = sp.makeIStream();
	if ( !sd.usable() ) continue;

	char buf[80];
	while ( *sd.istrm )
	{
	    sd.istrm->getline( buf, 80 );
	    totalnr_++;
	}
	sd.close();
	totalnr_ -= fd_.nrhdrlines_;
    }

    return totalnr_;
}


void HorizonScanner::report( IOPar& iopar ) const
{
    iopar.clear();

    const int firstattribidx = isgeom_ ? 1 : 0;
    BufferString str = "Report for horizon file(s):\n";
    for ( int idx=0; idx<filenames_.size(); idx++ )
    {
	str += filenames_.get(idx).buf(); str += "\n";
    }
    str += "\n\n";
    iopar.setName( str.buf() );

    if ( isxy_ != selxy_ )
    {
	iopar.add( "->", "Warning" );
	const char* selected = selxy_ ? "X/Y" : "Inl/Crl";
	const char* actual = isxy_ ? "X/Y" : "Inl/Crl";
	iopar.add( "You have selected positions in", selected );
	iopar.add( "But the positions in input file appear to be in", actual );
	BufferString msg = "OpendTect will use ";
	msg += actual; msg += " for final import";
	iopar.add( msg.buf(), "" );
    }

    iopar.add( "->", "Geometry" );
    HorSampling hs;
    hs.set( geomdetector_.inlrg, geomdetector_.crlrg );
    hs.fillPar( iopar );
    if ( isgeom_ && valranges_.size() > 0 )
	iopar.set( sKey::ZRange, valranges_[0].start, valranges_[0].stop );
    iopar.set( "Nr. of  positions", nrPositions() );
    iopar.setYN( "Inline gaps found", gapsFound(true) );
    iopar.setYN( "Crossline gaps found", gapsFound(false) );

    if ( valranges_.size() > firstattribidx )
    {
	iopar.add( "->", "Data values" );
	for ( int idx=firstattribidx; idx<valranges_.size(); idx++ )
	{
	    const char* attrnm = fd_.bodyinfos_[idx+1]->name().buf();
	    iopar.set( IOPar::compKey(attrnm,"Minimum value"),
		       valranges_[idx].start );
	    iopar.set( IOPar::compKey(attrnm,"Maximum value"),
		       valranges_[idx].stop );
	}
    }
    else
	iopar.add( "->", "No attribute data values" );

    if ( !rejectedlines_.isEmpty() )
    {
	iopar.add( "->", "Warning" );
	iopar.add( "These positions were rejected", "" );
	for ( int idx=0; idx<rejectedlines_.size(); idx++ )
	    iopar.add( BufferString(idx).buf(), rejectedlines_.get(idx).buf() );
    }
}



const char* HorizonScanner::defaultUserInfoFile()
{
    static BufferString ret;
    ret = GetProcFileName( "scan_horizon" );
    if ( GetSoftwareUser() )
	{ ret += "_"; ret += GetSoftwareUser(); }
    ret += ".txt";
    return ret.buf();
}


void HorizonScanner::launchBrowser( const char* fnm ) const
{
    if ( !fnm || !*fnm )
	fnm = defaultUserInfoFile();
    IOPar iopar; report( iopar );
    iopar.write( fnm, "_pretty" );

    ExecuteScriptCommand( "FileBrowser", fnm );
}


bool HorizonScanner::reInitAscIO( const char* fnm )
{
    StreamProvider sp( fnm );
    StreamData sd = sp.makeIStream();
    if ( !sd.usable() )
	return false;

    ascio_ = new EM::Horizon3DAscIO( fd_, *sd.istrm );
    if ( !ascio_ ) return false;

    return true;
}


bool HorizonScanner::analyzeData()
{
    if ( !reInitAscIO( filenames_.get(0).buf() ) ) return false;

    const float fac = SI().zIsTime() ? 0.001
				     : (SI().zInMeter() ? .3048 : 3.28084);
    Interval<float> validrg( SI().zRange(false) );
    const float zwidth = validrg.width();
    validrg.sort();
    validrg.start -= zwidth;
    validrg.stop += zwidth;

    int maxcount = 100;
    int count, nrxy, nrbid, nrscale, nrnoscale;
    count = nrxy = nrbid = nrscale = nrnoscale = 0;
    Coord crd;
    float val;
    TypeSet<float> data;
    while ( ascio_->getNextLine(data) > 0 )
    {
	if ( data.size() < 3 ) break;

	if ( count > maxcount ) 
	{
	    if ( nrscale == nrnoscale ) maxcount *= 2;
	    else break;
	}

	crd.x = data[0];
	crd.y = data[1];
	BinID bid( mNINT(crd.x), mNINT(crd.y) );

	bool validplacement = false;
	if ( SI().isReasonable(crd) ) { nrxy++; validplacement=true; }
	if ( SI().isReasonable(bid) ) { nrbid++; validplacement=true; }

	if ( !isgeom_ )
	{
	    if ( validplacement ) count++;
	    continue;
	}

	val = data[2];
	bool validvert = false;
	if ( !mIsUdf(val) ) 
	{
	    if ( validrg.includes(val) ) { nrnoscale++; validvert=true; }
	    else if ( validrg.includes(val*fac) ) { nrscale++; validvert=true; }
	}

	if ( validplacement && validvert )
	    count++;
    }

    isxy_ = nrxy > nrbid;
    selxy_ = ascio_->isXY();
    doscale_ = nrscale > nrnoscale;
    delete ascio_;
    ascio_ = 0;
    return true;
}


static bool isInsideSurvey( const BinID& bid, float zval )
{
    const CubeSampling& cs = SI().sampling( false );
    return cs.hrg.includes( bid ) && cs.zrg.includes( zval );
}


int HorizonScanner::nextStep()
{
    if ( fileidx_ >= filenames_.size() )
	return Executor::Finished;

    if ( !ascio_ && !reInitAscIO( filenames_.get(fileidx_).buf() ) )
	return Executor::ErrorOccurred;

    TypeSet<float> data;
    const int ret = ascio_->getNextLine( data );
    if ( ret < 0 ) return Executor::ErrorOccurred;
    if ( ret == 0 ) 
    {
	fileidx_++;
	delete ascio_;
	ascio_ = 0;
	sections_ += bvalset_;
	bvalset_ = 0;
	return Executor::MoreToDo;
    }

    if ( data.size() < 3 ) return Executor::ErrorOccurred;

    if ( !bvalset_ ) bvalset_ = new BinIDValueSet( data.size()-2, false );

    Coord crd;
    BinID bid;

    float fac = 1;
    if ( doscale_ )
    fac = SI().zIsTime() ? 0.001 : (SI().zInMeter() ? .3048 : 3.28084);

    if ( isxy_ )
    {
	crd.x = data[0];
	crd.y = data[1];
	bid = SI().transform( crd );
    }
    else
    {
	bid.inl = mNINT( data[0] );
	bid.crl = mNINT( data[1] );
	crd = SI().transform( bid );
    }

    bool validpos = true;
    int validx = 0;
    while ( validx < data.size()-2 )
    {
	if ( firsttime_ )
	    valranges_ += Interval<float>(mUdf(float),-mUdf(float));

	const float val = data[validx+2];
	if ( isgeom_ && validx==0 && !isInsideSurvey(bid,fac*val) )
	{
	    validpos = false;
	    break;
	}

	if ( !mIsUdf(val) )
	    valranges_[validx].include( fac*val, false );
	validx++;
    }

    if ( validpos && validx == 0 )
	validpos = false;

    if ( validpos )
    {
	geomdetector_.add( bid, crd );
	bvalset_->add( bid, data.arr()+2 );
    }

    else if ( rejectedlines_.size()<1024 )
    {
	BufferString rej( data[0] );
	rej += "\t";
	rej += data[1];
	if ( isgeom_ )
	{ rej += "\t"; rej += data[2]; }
	rejectedlines_.add( rej.buf() );
    }

    firsttime_ = false;
    return Executor::MoreToDo;
}


int HorizonScanner::nrPositions() const
{ return geomdetector_.nrpositions; }

StepInterval<int> HorizonScanner::inlRg() const
{ return geomdetector_.inlrg; }

StepInterval<int> HorizonScanner::crlRg() const
{ return geomdetector_.crlrg; }

bool HorizonScanner::gapsFound( bool inl ) const
{ return inl ? geomdetector_.inlgapsfound : geomdetector_.crlgapsfound; }

