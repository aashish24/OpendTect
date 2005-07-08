/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 28-2-1996
 * FUNCTION : Seismic trace informtaion
-*/

static const char* rcsID = "$Id: seisinfo.cc,v 1.28 2005-07-08 11:22:15 cvsbert Exp $";

#include "seisinfo.h"
#include "seistrc.h"
#include "susegy.h"
#include "posauxinfo.h"
#include "binidselimpl.h"
#include "survinfo.h"
#include "strmprov.h"
#include "strmdata.h"
#include "filegen.h"
#include "iopar.h"
#include "cubesampling.h"
#include <math.h>
#include <timeser.h>
#include <float.h>
#include <iostream>

const char* SeisTrcInfo::sSamplingInfo = "Sampling information";
const char* SeisTrcInfo::sNrSamples = "Nr of samples";
const char* SeisPacketInfo::sBinIDs = "BinID range";
const char* SeisPacketInfo::sZRange = "Z range";


static BufferString getUsrInfo()
{
    BufferString bs;
    const char* envstr = getenv( "DTECT_SEIS_USRINFO" );
    if ( !envstr || !File_exists(envstr) ) return bs;

    StreamData sd = StreamProvider(envstr).makeIStream();
    if ( sd.usable() )
    {
	char buf[1024];
	while ( *sd.istrm )
	{
	    sd.istrm->getline( buf, 1024 );
	    if ( *(const char*)bs ) bs += "\n";
	    bs += buf;
	}
    }

    return bs;
}

BufferString SeisPacketInfo::defaultusrinfo = getUsrInfo();


DefineEnumNames(Seis,WaveType,0,"Wave type")
{
	"P",
	"Sh",
	"Sv",
	"Other",
	0
};

DefineEnumNames(Seis,DataType,0,"Data type")
{
	"Amplitude",
	"Dip",
	"Frequency",
	"Phase",
	"AVO Gradient",
	"Azimuth",
	"Other",
	0
};
 

const char* SeisTrcInfo::attrnames[] = {
	"Trace number",
	"Pick position",
	"Reference position",
	"X-coordinate",
	"Y-coordinate",
	"In-line",
	"Cross-line",
	"Offset",
	"Azimuth",
	0
};


void SeisPacketInfo::clear()
{
    usrinfo = defaultusrinfo;
    nr = 0;
    SI().sampling(false).hrg.get( inlrg, crlrg );
    zrg = SI().zRange();
    inlrev = crlrev = false;
}


float SeisTrcInfo::defaultSampleInterval( bool forcetime )
{
    float defsr = SI().zRange().step;
    if ( SI().zIsTime() || !forcetime )
	return defsr;

    defsr /= SI().zInFeet() ? 5000 : 2000; // div by velocity
    int ival = (int)(defsr * 1000 + .5);
    return ival * 0.001;
}


int SeisTrcInfo::attrNr( const char* nm )
{
    return getEnum( nm, attrnames, 0, 1 );
}


double SeisTrcInfo::getAttr( int attrnr ) const
{
    switch ( attrnr )
    {
    case 1:	return pick;
    case 2:	return refpos;
    case 3:	return coord.x;
    case 4:	return coord.y;
    case 5:	return binid.inl;
    case 6:	return binid.crl;
    case 7:	return offset;
    case 8:	return azimuth;
    default:	return nr;
    }
}


bool SeisTrcInfo::dataPresent( float t, int trcsz ) const
{
    if ( t < sampling.start || t > samplePos(trcsz-1) )
	return false;
    if ( Values::isUdf(mute_pos) || t > mute_pos )
	return true;
    return false;
}


void SeisTrcInfo::usePar( const IOPar& iopar )
{
    iopar.get( attrnames[0], nr );
    iopar.get( attrnames[1], pick );
    iopar.get( attrnames[2], refpos );
    iopar.get( attrnames[3], coord.x );
    iopar.get( attrnames[4], coord.y );
    iopar.get( attrnames[5], binid.inl );
    iopar.get( attrnames[6], binid.crl );
    iopar.get( attrnames[7], offset );
    iopar.get( attrnames[8], azimuth );

    iopar.get( sSamplingInfo, sampling.start, sampling.step );
}

void SeisTrcInfo::fillPar( IOPar& iopar ) const
{
    iopar.set( attrnames[0], nr );
    iopar.set( attrnames[1], pick );
    iopar.set( attrnames[2], refpos );
    iopar.set( attrnames[3], coord.x );
    iopar.set( attrnames[4], coord.y );
    iopar.set( attrnames[5], binid.inl );
    iopar.set( attrnames[6], binid.crl );
    iopar.set( attrnames[7], offset );
    iopar.set( attrnames[8], azimuth );

    iopar.set( sSamplingInfo, sampling.start, sampling.step );
}


int SeisTrcInfo::nearestSample( float t ) const
{
    float s = Values::isUdf(t) ? 0 : (t - sampling.start) / sampling.step;
    return mNINT(s);
}


SampleGate SeisTrcInfo::sampleGate( const Interval<float>& tg ) const
{
    SampleGate sg;

    sg.start = sg.stop = 0;
    if ( Values::isUdf(tg.start) && Values::isUdf(tg.stop) )
	return sg;

    Interval<float> vals(
	Values::isUdf(tg.start) ? 0 : (tg.start-sampling.start) / sampling.step,
	Values::isUdf(tg.stop) ? 0 : (tg.stop-sampling.start) / sampling.step );

    if ( vals.start < vals.stop )
    {
	sg.start = (int)floor(vals.start+1e-3);
	sg.stop =  (int)ceil(vals.stop-1e-3);
    }
    else
    {
	sg.start =  (int)ceil(vals.start-1e-3);
	sg.stop = (int)floor(vals.stop+1e-3);
    }

    if ( sg.start < 0 ) sg.start = 0;
    if ( sg.stop < 0 ) sg.stop = 0;

    return sg;
}



void SeisTrcInfo::gettr( SUsegy& trc ) const
{
    trc.tracl = trc.fldr = trc.tracf = nr;
    trc.trid = 1;
    const float zfac = SI().zFactor();
    trc.dt = (int)(sampling.step * 1e3 * zfac + .5);
    trc.delrt = (short)mNINT(sampling.start * zfac);

    SULikeHeader* head = (SULikeHeader*)(&trc);
    head->indic = 123456;
    head->inl = binid.inl;
    head->crl = binid.crl;
    head->pick = pick;
    head->refpos = refpos;
    head->startpos = sampling.start;
    head->offset = offset;
    head->azimuth = azimuth;
}


void SeisTrcInfo::puttr( const SUsegy& trc )
{
    nr = trc.tracl;
    const float zfac = 1. / SI().zFactor();
    sampling.step = trc.dt * 0.001 * zfac;
    sampling.start = trc.delrt * zfac;
    SULikeHeader* head = (SULikeHeader*)(&trc);
    if ( head->indic == 123456 )
    {
	binid.inl = head->inl;
	binid.crl = head->crl;
	pick = head->pick;
	offset = head->offset;
	azimuth = head->azimuth;
	refpos = head->refpos;
	if ( !mIsZero(head->startpos,mDefEps) )
	    sampling.start = head->startpos;
    }
}


void SeisTrcInfo::putTo( PosAuxInfo& auxinf ) const
{
    auxinf.binid = binid;
    auxinf.startpos = sampling.start;
    auxinf.coord = coord;
    auxinf.offset = offset;
    auxinf.azimuth = azimuth;
    auxinf.pick = pick;
    auxinf.refpos = refpos;
}


void SeisTrcInfo::getFrom( const PosAuxInfo& auxinf )
{
    binid = auxinf.binid;
    sampling.start = auxinf.startpos;
    coord = auxinf.coord;
    offset = auxinf.offset;
    azimuth = auxinf.azimuth;
    pick = auxinf.pick;
    refpos = auxinf.refpos;
}
