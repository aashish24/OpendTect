/*
___________________________________________________________________

 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahk
 * DATE     : Jun 2003
___________________________________________________________________

-*/

#include "emsurfauxdataio.h"

#include "ascstream.h"
#include "datainterp.h"
#include "datachar.h"
#include "emsurface.h"
#include "geomgridsurface.h"
#include "iopar.h"
#include "survinfo.h"

#include <fstream>

static const char* rcsID = "$Id: emsurfauxdataio.cc,v 1.4 2003-07-07 15:12:38 nanne Exp $";

const char* EM::dgbSurfDataWriter::attrnmstr = "Attribute";
const char* EM::dgbSurfDataWriter::infostr = "Info";
const char* EM::dgbSurfDataWriter::intdatacharstr = "Int data";
const char* EM::dgbSurfDataWriter::floatdatacharstr = "Float data";

EM::dgbSurfDataWriter::dgbSurfDataWriter( const EM::Surface& surf_,int dataidx_,
				    const BinIDSampler* sel_, bool binary_,
       				    const char* filename )
    : Executor( "Aux data writer" )
    , stream( 0 )
    , chunksize( 100 )
    , dataidx( dataidx_ )
    , surf( surf_ )
    , sel( sel_ )
    , patchindex( 0 )
    , binary( binary_ )
{
    IOPar par( "Surface Data" );
    par.set( attrnmstr, surf.auxDataName(dataidx) );

    if ( binary )
    {
	int dummy;
	unsigned char dc[2];
	DataCharacteristics(dummy).dump(dc[0], dc[1]);
	par.set(intdatacharstr, (int) dc[0],(int)dc[1]);
					 
	float fdummy;
	DataCharacteristics(fdummy).dump(dc[0], dc[1]);
	par.set(floatdatacharstr, (int) dc[0],(int)dc[1]);
    }

    stream = binary ? new ofstream( filename, ios::binary )
		    : new ofstream( filename);

    if ( !(*stream) ) return;

    ascostream astream( *stream );

    par.putTo( astream );

    int nrnodes = 0;
    for ( int idx=0; idx<surf.nrPatches(); idx++ )
    {
	EM::PatchID patchid = surf.patchID(idx);
	const Geometry::GridSurface* gridsurf = surf.getSurface(patchid);

	nrnodes += gridsurf->size();
    }

    totalnr = nrnodes/chunksize+1;
}


EM::dgbSurfDataWriter::~dgbSurfDataWriter()
{
    stream->close();
    delete stream;
}


int EM::dgbSurfDataWriter::nextStep()
{
    for ( int idx=0; idx<chunksize; idx++ )
    {
	while ( !subids.size() )
	{
	    if ( nrdone )
	    {
		patchindex++;
		if ( patchindex>=surf.nrPatches() )
		    return Finished;
	    }
	    else
	    {
		if ( !writeLong(surf.nrPatches()))
		    return ErrorOccurred;
	    }


	    const EM::PatchID patchid = surf.patchID( patchindex );
	    const Geometry::GridSurface* gridsurf = surf.getSurface(patchid);

	    const int nrnodes = gridsurf->size();
	    for ( int idy=0; idy<nrnodes; idx++ )
	    {
		EM::SubID subid = gridsurf->getPosID(idy);
		Coord3 coord = gridsurf->getPos( subid );

		if ( sel && sel->excludes(SI().transform(coord)) )
		    continue;

		const EM::PosID posid( surf.id(), patchid, subid);
		const float auxvalue = surf.getAuxDataVal(dataidx,posid);
		if ( mIsUndefined( auxvalue ) )
		    continue;

		subids += subid;
		values += auxvalue;
	    }

	    if ( !writeLong( patchid ) || !writeLong(subids.size()))
		return ErrorOccurred;
	}

	const int subidindex = subids.size()-1;
	const EM::SubID subid = subids[subidindex];
	const float auxvalue = values[subidindex];

	if ( !writeLong(subid) || !writeFloat(auxvalue) )
	    return ErrorOccurred;

	subids.remove( subidindex );
    }

    nrdone++;
    return MoreToDo;
}


int EM::dgbSurfDataWriter::nrDone() const 
{
    return nrdone;
}


int EM::dgbSurfDataWriter::totalNr() const
{
    return totalnr;
}


BufferString EM::dgbSurfDataWriter::createHovName( const char* base, int idx )
{
        BufferString res( base );
	res += "^"; res += idx; res += ".hov";
	return res;
}


bool EM::dgbSurfDataWriter::writeLong(long val)
{
    if ( binary )
	stream->write((char*) &val, sizeof(val));
    else
	(*stream) << val << '\n' ;

    return (*stream);
}


bool EM::dgbSurfDataWriter::writeFloat(float val)
{
    if ( binary )
	stream->write((char*) &val ,sizeof(val));
    else
	(*stream) << val << '\n';

    return (*stream);
}


EM::dgbSurfDataReader::dgbSurfDataReader( const char* filename )
    : Executor( "Aux data writer" )
    , stream( new ifstream(filename) )
    , subidinterpreter( 0 )
    , datainterpreter( 0 )
    , chunksize( 100 )
    , dataidx( -1 )
    , surf( 0 )
    , patchindex( 0 )
    , error( true )
{
    if ( !(*stream) )
	return;

    ascistream astream( *stream );
    const IOPar par( astream );
    if ( !par.get( dgbSurfDataWriter::attrnmstr, dataname ) )
	return;

    if ( !par.get( dgbSurfDataWriter::attrnmstr, datainfo ) )
	return;

    int dc[2];
    if ( par.get(EM::dgbSurfDataWriter::intdatacharstr, dc[0], dc[1] ))
    {
	DataCharacteristics writtendatachar;
	writtendatachar.set( dc[0], dc[1] );
	subidinterpreter = new DataInterpreter<int>( writtendatachar );
					     
	if ( !par.get(EM::dgbSurfDataWriter::floatdatacharstr, dc[0], dc[1]))
	    return;
						     
	writtendatachar.set( dc[0], dc[1] );
	datainterpreter = new DataInterpreter<float>( writtendatachar );
    }

    error = false;
}


EM::dgbSurfDataReader::~dgbSurfDataReader()
{
    stream->close();
    delete stream;
}


const char* EM::dgbSurfDataReader::dataName() const
{
    return dataname[0] ? dataname.buf() : 0;
}


const char* EM::dgbSurfDataReader::dataInfo() const
{
    return datainfo[0] ? datainfo.buf() : 0;
}



void EM::dgbSurfDataReader::setSurface( EM::Surface& surf_ )
{
    surf = & surf_;
}


int EM::dgbSurfDataReader::nextStep()
{
    if ( error )
	return ErrorOccurred; 

    for ( int idx=0; idx<chunksize; idx++ )
    {
	while ( !valsleftonpatch )
	{
	    if ( nrdone )
	    {
		patchindex++;
		if ( patchindex>=nrpatches )
		    return Finished;
	    }
	    else
	    {
		if ( !readLong(nrpatches) )
		    return ErrorOccurred;
	    }

	    int cp;
	    if ( !readLong(cp) || !readLong(valsleftonpatch))
		return ErrorOccurred;

	    currentpatch = cp;
	}

	int subid;
	float val;
	if ( !readLong(subid) && !readFloat(val) )
	    return ErrorOccurred;

	surf->setAuxDataVal( dataidx,
		EM::PosID( surf->id(),currentpatch,subid ), val);

	valsleftonpatch--;
    }

    nrdone++;
    return MoreToDo;
}


int EM::dgbSurfDataReader::nrDone() const 
{
    return nrdone;
}


int EM::dgbSurfDataReader::totalNr() const
{
    return totalnr;
}

bool EM::dgbSurfDataReader::readLong(int& res)
{
    if ( subidinterpreter )
    {
	char buf[sizeof(res)];
	stream->read(buf,sizeof(res));
	res = subidinterpreter->get(buf,0);
    }
    else
    {
	(*stream) >> res;
    }

    return (*stream);
}


bool EM::dgbSurfDataReader::readFloat(float& res)
{
    if ( datainterpreter )
    {
	char buf[sizeof(res)];
	stream->read(buf,sizeof(res));
	res = datainterpreter->get(buf,0);
    }
    else
    {
	(*stream) >> res;
    }

    return (*stream);
}
