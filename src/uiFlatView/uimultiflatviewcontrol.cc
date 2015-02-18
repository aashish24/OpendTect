/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Feb 2012
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uimultiflatviewcontrol.h"

#include "uiflatviewer.h"
#include "uimainwin.h"
#include "uigraphicsscene.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"
#include "uirgbarraycanvas.h"
#include "uiworld2ui.h"

#include "flatviewzoommgr.h"
#include "scaler.h"
#include "survinfo.h"


void MFVCViewManager::setViewerType( const uiFlatViewer* vwr, bool isintime )
{
    const int vwridx = vwrs_.indexOf( vwr );
    if ( vwridx<0 )
    {
	vwrs_ += vwr;
	zintimeflags_ += BoolTypeSetType( isintime );
	return;
    }
    zintimeflags_[vwridx] = isintime;
}


bool MFVCViewManager::getViewRect( const uiFlatViewer* activevwr,
				   const uiFlatViewer* curvwr,
				   uiWorldRect& viewwr ) const
{
    const int activevwridx = vwrs_.indexOf( activevwr );
    const int curvwridx = vwrs_.indexOf( curvwr );
    if ( !vwrs_.validIdx(curvwridx) || !vwrs_.validIdx(activevwridx) ||
	 activevwridx<0 )
	return false;
    const uiWorldRect& wr = activevwr->curView();
    if ( d2tmodels_.isEmpty() )
    {
	const uiWorldRect& masterbbox = activevwr->boundingBox();
	const uiWorldRect bbox = vwrs_[curvwridx]->boundingBox();
	LinScaler sclr( masterbbox.left(), bbox.left(),
			masterbbox.right(), bbox.right() );
	LinScaler sctb( masterbbox.top(), bbox.top(),
			masterbbox.bottom(), bbox.bottom() );
	viewwr = uiWorldRect( sclr.scale(wr.left()), sctb.scale(wr.top()),
			      sclr.scale(wr.right()), sctb.scale(wr.bottom()) );
    }
    else
    {
	if ( !zintimeflags_.validIdx(curvwridx) ||
	     !zintimeflags_.validIdx(activevwridx) )
	    return false;
	const bool isactiveintime = zintimeflags_[activevwridx];
	const bool iscurintime = zintimeflags_[curvwridx];
	if ( isactiveintime == iscurintime )
	{
	    viewwr = wr;
	    return true;
	}
	viewwr.setLeft( wr.left() );
	viewwr.setRight( wr.right() );

	if ( isactiveintime )
	{
	    Interval<float> timerg( mCast(float,wr.top()),
		    		    mCast(float,wr.bottom()) );
	    Interval<double> depthrg( d2tmodels_[0]->getDepth(timerg.start),
				      d2tmodels_[0]->getDepth(timerg.stop) );
	    for ( int idx=1; idx<d2tmodels_.size(); idx++ )
	    {
		const TimeDepthModel& d2t = *d2tmodels_[idx];
		Interval<double> curdepthrg( d2t.getDepth(timerg.start),
					     d2t.getDepth(timerg.stop) );
		if ( !curdepthrg.isUdf() )
		    depthrg.include( curdepthrg );
	    }

	    depthrg.shift( SI().seismicReferenceDatum() );
	    viewwr.setTop( depthrg.start );
	    viewwr.setBottom( depthrg.stop );
	}
	else
	{
	    Interval<float> depthrg( mCast(float,wr.top()),
		    		     mCast(float,wr.bottom()) );
	    Interval<double> timerg( d2tmodels_[0]->getTime(depthrg.start),
				      d2tmodels_[0]->getTime(depthrg.stop) );
	    for ( int idx=1; idx<d2tmodels_.size(); idx++ )
	    {
		const TimeDepthModel& d2t = *d2tmodels_[idx];
		Interval<double> curtimerg( d2t.getTime(depthrg.start),
					     d2t.getTime(depthrg.stop) );
		if ( !curtimerg.isUdf() )
		    timerg.include( curtimerg );
	    }

	    viewwr.setTop( timerg.start );
	    viewwr.setBottom( timerg.stop );
	}
    }

    return true;
}


uiMultiFlatViewControl::uiMultiFlatViewControl( uiFlatViewer& vwr,
				    const uiFlatViewStdControl::Setup& setup )
    : uiFlatViewStdControl(vwr,setup)
    , iszoomcoupled_(true)
    , drawzoomboxes_(false)			  
    , activevwr_(&vwr)  
{
    setViewerType( &vwr, true );
    mAttachCB( vwr.viewChanged, uiMultiFlatViewControl::setZoomBoxesCB );
    mAttachCB( vwr.viewChanged, uiMultiFlatViewControl::setZoomAreasCB );
    parsbuts_ += parsbut_;
    toolbars_ += tb_;
}


uiMultiFlatViewControl::~uiMultiFlatViewControl()
{
    detachAllNotifiers();
    
    for ( int idx=0; idx<zoomboxes_.size(); idx++ )
	vwrs_[idx]->removeAuxData( zoomboxes_[idx] );
    
    deepErase( zoomboxes_ );
}


bool uiMultiFlatViewControl::setActiveVwr( int vwridx )
{
    if ( !vwrs_.validIdx(vwridx) )
	return false;

    activevwr_ = vwrs_[vwridx];
    return true;
}


void uiMultiFlatViewControl::setNewView(Geom::Point2D<double> mousepos,
					Geom::Size2D<double> sz)
{
    const uiWorldRect wr = getZoomOrPanRect( mousepos,sz,activevwr_->curView(),
	    				     activevwr_->boundingBox() );
    activevwr_->setView( wr );
    addSizesToZoomMgr();
}


#define mAddBut(but,fnm,cbnm,tt) \
    but = new uiToolButton(tb_,fnm,tt,mCB(this,uiFlatViewStdControl,cbnm) ); \
    tb_->addButton( but );

void uiMultiFlatViewControl::vwrAdded( CallBacker* )
{
    const int ivwr = vwrs_.size()-1;
    uiFlatViewer& vwr = *vwrs_[ivwr];
    MouseEventHandler& mevh = vwr.rgbCanvas().getNavigationMouseEventHandler();
    mAttachCB( mevh.wheelMove, uiMultiFlatViewControl::wheelMoveCB );
    
    toolbars_ += new uiToolBar(mainwin(),"Flat Viewer Tools",tb_->prefArea());

    parsbuts_ += new uiToolButton( toolbars_[ivwr],"2ddisppars",
				   tr("Set display parameters"),
				   mCB(this,uiMultiFlatViewControl,parsCB) );
    toolbars_[ivwr]->addButton( parsbuts_[ivwr] );

    vwr.setRubberBandingOn( !manip_ );
    vwr.appearance().annot_.editable_ = false;
    mAttachCB( vwr.viewChanged, uiMultiFlatViewControl::setZoomAreasCB );
    mAttachCB( vwr.viewChanged, uiMultiFlatViewControl::setZoomBoxesCB );

    reInitZooms();
    setViewerType( &vwr, true );
}


void uiMultiFlatViewControl::rubBandCB( CallBacker* cb )
{
    mDynamicCastGet( uiGraphicsView*,cnv, cb );
    for ( int idx=0; idx<vwrs_.size(); idx++ )
	if ( &vwrs_[idx]->rgbCanvas()  == cnv )
	    { activevwr_ = vwrs_[idx]; break; }

    const uiRect* selarea = activevwr_->rgbCanvas().getSelectedArea();
    if ( !selarea || (selarea->topLeft() == selarea->bottomRight()) ||
	(selarea->width()<5 && selarea->height()<5) )
	return;

    uiWorld2Ui w2u;
    activevwr_->getWorld2Ui(w2u);
    uiWorldRect wr = w2u.transform(*selarea);
    wr = getZoomOrPanRect( wr.centre(), wr.size(), wr,
	    		   activevwr_->boundingBox() );
    activevwr_->setView( wr );
    addSizesToZoomMgr();
}


void uiMultiFlatViewControl::reInitZooms()
{
    for ( int idx=0; idx<vwrs_.size(); idx++ )
	vwrs_[idx]->setViewToBoundingBox();
    zoommgr_.reInit( getBoundingBoxes() );
    zoommgr_.toStart();
}


void uiMultiFlatViewControl::addSizesToZoomMgr()
{
    if ( !iszoomcoupled_ )
    {
	zoommgr_.add( activevwr_->curView().size(), vwrs_.indexOf(activevwr_) );
	zoomChanged.trigger();
	return;
    }

    for ( int idx=0; idx<vwrs_.size(); idx++ )
	zoommgr_.add( vwrs_[idx]->curView().size(), idx );

    zoomChanged.trigger();
}


void uiMultiFlatViewControl::wheelMoveCB( CallBacker* cb )
{
    mDynamicCastGet( const MouseEventHandler*, meh, cb );
    if ( !meh || !meh->hasEvent() ) return;

    const int vwridx = getViewerIdx( meh, false );
    if ( vwridx<0 ) return;
    activevwr_ = vwrs_[vwridx];

    const MouseEvent& ev = meh->event();
    if ( mIsZero(ev.angle(),0.01) )
	return;

    zoomCB( ev.angle()<0 ? zoominbut_ : zoomoutbut_ );
}


void uiMultiFlatViewControl::zoomCB( CallBacker* but )
{
    const MouseEventHandler& meh =
	activevwr_->rgbCanvas().getNavigationMouseEventHandler();
    const bool zoomin = but == zoominbut_;
    const int vwridx = vwrs_.indexOf(activevwr_);
    if ( !zoomin && !meh.hasEvent() && zoommgr_.atStart(vwridx) )
	for ( int idx=0; idx<vwrs_.size(); idx++ )
	    if ( !zoommgr_.atStart(idx) ) { activevwr_ = vwrs_[idx]; break; }

    doZoom( zoomin, *activevwr_ );
}


void uiMultiFlatViewControl::pinchZoomCB( CallBacker* cb )
{
    mDynamicCastGet(const GestureEventHandler*,evh,cb);
    if ( !evh || evh->isHandled() )
	return;

    const GestureEvent* gevent = evh->getPinchEventInfo();
    if ( !gevent )
	return;

    int vwridx = -1;
    for ( int idx=0; idx<vwrs_.size(); idx++ )
    {
	if ( evh == &(vwrs_[idx]->rgbCanvas().gestureEventHandler()) )
	{
	    vwridx = idx;
	    break;
	}
    }

    if ( vwridx<0 ) return;
    activevwr_ = vwrs_[vwridx];
    const Geom::Size2D<double> cursz = activevwr_->curView().size();

    const float scalefac = gevent->scale();
    Geom::Size2D<double> newsz( cursz.width() * (1/scalefac), 
				cursz.height() * (1/scalefac) );
    uiWorld2Ui w2ui;
    activevwr_->getWorld2Ui( w2ui );
    Geom::Point2D<double> pos = w2ui.transform( gevent->pos() );

    const uiWorldRect wr = getZoomOrPanRect( pos, newsz, activevwr_->curView(),
	    				     activevwr_->boundingBox());
    vwrs_[vwridx]->setView( wr );

    if ( gevent->getState() == GestureEvent::Finished )
	addSizesToZoomMgr();
}


void uiMultiFlatViewControl::setZoomAreasCB( CallBacker* cb )
{
    mDynamicCastGet(uiFlatViewer*,vwr,cb);
    if ( !iszoomcoupled_ || !vwrs_.isPresent(vwr) ) return;
    activevwr_ = vwr;

    for ( int idx=0; idx<vwrs_.size(); idx++ )
    {
	if ( vwrs_[idx] == activeVwr() )
	    continue;

	uiWorldRect newwr;
	if ( !viewmgr_.getViewRect(activeVwr(),vwrs_[idx],newwr) )
	    continue;

	NotifyStopper ns( vwrs_[idx]->viewChanged );
	vwrs_[idx]->setView( newwr );
    }
}


bool uiMultiFlatViewControl::handleUserClick( int vwridx )
{
    return true;
}


void uiMultiFlatViewControl::parsCB( CallBacker* cb )
{
    mDynamicCastGet(uiToolButton*,but,cb);
    const int idx = parsbuts_.indexOf( but ); 
    if ( idx >= 0 )
	doPropertiesDialog( idx );
}


void uiMultiFlatViewControl::setZoomBoxesCB( CallBacker* cb )
{
    for ( int idx=0; idx<zoomboxes_.size(); idx++ )
	delete vwrs_[idx]->removeAuxData( zoomboxes_[idx] );

    zoomboxes_.erase();

    if ( iszoomcoupled_ || !drawzoomboxes_ ) 
	return;

    const uiWorldRect& masterbbox = activeVwr()->boundingBox();
    const uiWorldRect& wr = activeVwr()->curView();
    if ( wr == masterbbox )
	return;
    for ( int idx=0; idx<vwrs_.size(); idx++ )
    {
	FlatView::AuxData* ad = vwrs_[idx]->createAuxData( "Zoom box" );
	vwrs_[idx]->addAuxData( ad );
	zoomboxes_ += ad;
	ad->linestyle_ = LineStyle( LineStyle::Dash, 3, Color::Black() );
	ad->zvalue_ = uiFlatViewer::auxDataZVal();

	if ( vwrs_[idx] == activeVwr() )
	    continue;

	uiWorldRect newwr;
       	if ( !viewmgr_.getViewRect(activeVwr(),vwrs_[idx],newwr) )
	    continue;
	ad->poly_ += newwr.topLeft(); 
	ad->poly_ += newwr.topRight();
	ad->poly_ += newwr.bottomRight();
	ad->poly_ += newwr.bottomLeft(); 
	ad->poly_ += newwr.topLeft(); 
	vwrs_[idx]->handleChange( FlatView::Viewer::Auxdata ); 
    }
}
