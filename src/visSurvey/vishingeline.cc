/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Mar 2002
 RCS:           $Id: vishingeline.cc,v 1.7 2004-08-20 07:54:06 kristofer Exp $
________________________________________________________________________

-*/


#include "vishingeline.h"

#include "emsurface.h"
#include "emsurfaceedgeline.h"
#include "emsurfacegeometry.h"
#include "emmanager.h"
#include "settings.h"
#include "trackingedgeline.h"
#include "viscoord.h"
#include "visevent.h"
#include "vismaterial.h"
#include "vispolyline.h"
#include "vistransform.h"

mCreateFactoryEntry( visSurvey::EdgeLineSetDisplay );

namespace visSurvey
{

#define mStopColor	1
#define mCutColor	2
#define mConnectColor	3

EdgeLineSetDisplay::EdgeLineSetDisplay()
    : VisualObjectImpl(true)
    , edgelineset( 0 )
    , transformation( 0 )
    , connect( true )
    , showdefault( true )
{
    getMaterial()->setColor( Color(255,255,0), 0 );
    getMaterial()->setAmbience(0, 0);
    getMaterial()->setDiffIntensity(1,0);
    getMaterial()->setColor( Color(0,0,255), mCutColor );
    getMaterial()->setAmbience(0, mCutColor);
    getMaterial()->setDiffIntensity(1,mCutColor);
    getMaterial()->setColor( Color(255,0,0), mStopColor );
    getMaterial()->setAmbience(0, mStopColor);
    getMaterial()->setDiffIntensity(1,mStopColor);
    getMaterial()->setColor( Color(0,255,0), mConnectColor );
    getMaterial()->setAmbience(0, mConnectColor);
    getMaterial()->setDiffIntensity(1,mConnectColor);

    Settings::common().getYN(IOPar::compKey("dTect","Surface.Show default edgelinesegments"),showdefault);
}


EdgeLineSetDisplay::~EdgeLineSetDisplay()
{
    if ( transformation ) transformation->unRef();
    for ( int idx=0; idx<polylines.size(); idx++ )
    {
	removeChild( polylines[idx]->getInventorNode() );
	polylines[idx]->unRef();
	polylines.remove(idx--);
    }

    deepErase( polylinesegments );
    deepErase( polylinesegmentpos );

    if ( edgelineset )
	const_cast<EM::EdgeLineSet*>(edgelineset)->changenotifier.remove(
		mCB(this,EdgeLineSetDisplay,updateEdgeLineSetChangeCB));
}


void EdgeLineSetDisplay::setConnect(bool yn)
{
    if ( yn==connect ) return;
    connect = yn;
    updateEdgeLineSetChangeCB(0);
}


bool EdgeLineSetDisplay::getConnect() const { return connect; }



void EdgeLineSetDisplay::setShowDefault(bool yn)
{
    if ( yn==showdefault ) return;
    showdefault = yn;
    updateEdgeLineSetChangeCB(0);
}


bool EdgeLineSetDisplay::getShowDefault() const { return showdefault; }



void EdgeLineSetDisplay::setEdgeLineSet( const EM::EdgeLineSet* nhl )
{
    if ( edgelineset )
	const_cast<EM::EdgeLineSet*>(edgelineset)->changenotifier.remove(
		mCB(this,EdgeLineSetDisplay,updateEdgeLineSetChangeCB));

    edgelineset = nhl;

    if ( edgelineset )
	const_cast<EM::EdgeLineSet*>(edgelineset)->changenotifier.notify(
		mCB(this,EdgeLineSetDisplay,updateEdgeLineSetChangeCB));

    updateEdgeLineSetChangeCB(0);
}


bool EdgeLineSetDisplay::setEdgeLineSet( int emobjid )
{
    EM::EMManager& em = EM::EMM();
    mDynamicCastGet(EM::EdgeLineSet*,emhl,em.getObject(emobjid))
    if ( !emhl ) return false;
    setEdgeLineSet( emhl );
    return true;
}


void EdgeLineSetDisplay::setTransformation( visBase::Transformation* nt)
{
    if ( transformation ) transformation->unRef();
    transformation = nt;
    if ( transformation ) transformation->ref();

    for ( int idx=0; idx<polylines.size(); idx++ )
	polylines[idx]->setTransformation( nt );
}


visBase::Transformation* EdgeLineSetDisplay::getTransformation()
{ return transformation; }



void EdgeLineSetDisplay::updateEdgeLineSetChangeCB(CallBacker*)
{
    if ( !edgelineset ) return;
    const EM::Surface& surface = edgelineset->getSurface();
    const EM::SectionID section = edgelineset->getSection();

    for ( int lineidx=0; lineidx<edgelineset->nrLines(); lineidx++ )
    {
	if ( lineidx>=polylines.size() )
	{
	    visBase::IndexedPolyLine3D* polyline =
				    visBase::IndexedPolyLine3D::create();
	    polylines += polyline;
	    polyline->ref();
	    polyline->setTransformation( transformation );
	    addChild( polyline->getInventorNode() );
	    polyline->setMaterialBinding( 2 );
	    polylinesegments += new TypeSet<int>;
	    polylinesegmentpos += new TypeSet<int>;
	}

	visBase::IndexedPolyLine3D* polyline = polylines[lineidx];
	TypeSet<int>& segments = *polylinesegments[lineidx];
	TypeSet<int>& segmentpos = *polylinesegmentpos[lineidx];
	segments.erase();
	segmentpos.erase();

	int coordindex = 0;
	int coordindexindex = 0;
	const EM::EdgeLine* edgeline = edgelineset->getLine(lineidx);

	const int nrsegments = edgeline->nrSegments();
	RowCol prevrc; bool defprevrc = false; 
	RowCol firstrc; bool deffirstrc = false;
	for ( int segmentidx=0; segmentidx<nrsegments; segmentidx++ )
	{
	    const EM::EdgeLineSegment* edgelinesegment =
					edgeline->getSegment(segmentidx);
	    int materialindex = 0;
	    bool showsegment = true;
	    if ( dynamic_cast<const Tracking::TerminationEdgeLineSegment*>(
						    edgelinesegment) )
	    {
		materialindex = mStopColor;
		showsegment = false;
	    }
	    else if ( dynamic_cast<const Tracking::SurfaceConnectLine*>
						    (edgelinesegment) )
	    {
		mDynamicCastGet( const Tracking::SurfaceConnectLine*,
				    connline, edgelinesegment );
		materialindex = mConnectColor;
		showsegment =
		    connline->getSection()<connline->connectingSection();
	    }
	    else if ( dynamic_cast<const Tracking::SurfaceCutLine*>
						    (edgelinesegment) )
	    {
		materialindex = mCutColor;
		showsegment = false;
	    }

	    if ( !showdefault && showsegment )
	    {
		if ( coordindexindex &&
			polyline->getCoordIndex(coordindexindex-1)!=-1 )
		{
		    polyline->setCoordIndex( coordindexindex, -1 );
		    polyline->setMaterialIndex( coordindexindex, -1 );
		    coordindexindex++;
		    defprevrc = false;
		}
		continue;
	    }

	    const int nrnodes = edgelinesegment->size();
	    for ( int nodeidx=0; nodeidx<nrnodes; nodeidx++ )
	    {
		const RowCol& rc = (*edgelinesegment)[nodeidx];
		if ( defprevrc &&
			!rc.isNeighborTo(prevrc, surface.geometry.step()) &&
			coordindexindex &&
			polyline->getCoordIndex(coordindexindex-1)!=-1 )
		{
		    polyline->setCoordIndex( coordindexindex, -1 );
		    polyline->setMaterialIndex( coordindexindex, -1 );
		    coordindexindex++;
		    defprevrc = false;
		    continue;
		}

		const Coord3 pos = surface.geometry.getPos(section,rc);
		if ( !pos.isDefined() )
		{
		    polyline->setCoordIndex( coordindexindex, -1 );
		    polyline->setMaterialIndex( coordindexindex, -1 );
		    coordindexindex++;
		    defprevrc = false;
		    continue;
		}

		polyline->getCoordinates()->setPos(coordindex,pos);
		segments += segmentidx;
		segmentpos += nodeidx;
		polyline->setCoordIndex( coordindexindex, coordindex );
		polyline->setMaterialIndex( coordindexindex, materialindex );
		coordindex++; coordindexindex++;
		prevrc = rc;
		defprevrc = true;

		if ( !deffirstrc ) { firstrc = rc; deffirstrc=true; }
	    }
	}

	if ( connect && deffirstrc && defprevrc &&
		firstrc.isNeighborTo(prevrc, surface.geometry.step()) )
	{
	    polyline->setCoordIndex( coordindexindex,
		    		     polyline->getCoordIndex(0) );
	    polyline->setMaterialIndex( coordindexindex,
				    polyline->getMaterialIndex(0) );
	    coordindexindex++;
	}

	
	polyline->setCoordIndex( coordindexindex, -1 );
	polyline->setMaterialIndex( coordindexindex, -1 );
	coordindexindex++;
	polyline->removeCoordIndexAfter(coordindexindex-1);
	polyline->removeMaterialIndexAfter(coordindexindex-1);
    }

    for ( int idx=polylines.size()-1; idx>=edgelineset->nrLines(); idx-- )
    {
	removeChild( polylines[idx]->getInventorNode() );
	polylines[idx]->unRef();
	polylines.remove( idx );

	delete polylinesegments[idx];
	polylinesegments.remove(idx);
	delete polylinesegmentpos[idx];
	polylinesegmentpos.remove(idx);
    }
}


void EdgeLineSetDisplay::triggerRightClick(const visBase::EventInfo* ei)
{
    rightclidedline = rightclidedsegment = rightclidedsegmentpos = -1;
    if ( ei ) 
    {
	int line = -1;
	for ( int lineidx=0; lineidx<polylines.size(); lineidx++ )
	{
	    if ( ei->pickedobjids.indexOf(polylines[lineidx]->id())!=-1 )
	    {
		line = lineidx;
		break;
	    }
	}

	if ( line!=-1 )
	{
	    const visBase::IndexedPolyLine3D* polyline = polylines[line];
	    const int pickedcoordidx = polyline->getClosestCoordIndex(*ei);
	    if ( pickedcoordidx!=-1 )
	    {
		rightclidedline = line;
		rightclidedsegment = (*polylinesegments[line])[pickedcoordidx];
		rightclidedsegmentpos =
		    (*polylinesegmentpos[line])[pickedcoordidx];
	    }
	}
    }

    visBase::VisualObject::triggerRightClick(ei);
}

}; // namespace visSurvey
