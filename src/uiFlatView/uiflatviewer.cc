/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2007
 RCS:           $Id: uiflatviewer.cc,v 1.17 2007-03-12 18:44:10 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiflatviewer.h"
#include "uirgbarraycanvas.h"
#include "uirgbarray.h"
#include "flatviewbitmapmgr.h"
#include "flatviewbmp2rgb.h"
#include "flatviewaxesdrawer.h"
#include "datapackbase.h"
#include "bufstringset.h"
#include "iodrawtool.h"
#include "drawaxis2d.h"
#include "uiworld2ui.h"
#include "uimsg.h"


uiFlatViewer::uiFlatViewer( uiParent* p )
    : uiGroup(p,"Flat viewer")
    , canvas_(*new uiRGBArrayCanvas(this,*new uiRGBArray))
    , axesdrawer_(*new FlatView::AxesDrawer(*this,canvas_))
    , reportedchange_(All)
    , dim0extfac_(0.5)
    , wvabmpmgr_(0)
    , vdbmpmgr_(0)
    , anysetviewdone_(false)
    , extraborders_(0,0,0,0)
    , annotsz_(50,20) //TODO: should be dep on font size
    , viewChanged(this)
{
    bmp2rgb_ = new FlatView::BitMap2RGB( appearance(), canvas_.rgbArray() );
    canvas_.newFillNeeded.notify( mCB(this,uiFlatViewer,canvasNewFill) );
    canvas_.postDraw.notify( mCB(this,uiFlatViewer,canvasPostDraw) );
}


uiFlatViewer::~uiFlatViewer()
{
    delete &canvas_.rgbArray();
    delete &canvas_;
    delete &axesdrawer_;
    delete wvabmpmgr_;
    delete vdbmpmgr_;
    delete bmp2rgb_;
}


uiRGBArray& uiFlatViewer::rgbArray()
{
    return canvas_.rgbArray();
}


Color uiFlatViewer::color( bool foreground ) const
{
    return appearance().darkBG() == foreground ? Color::White : Color::Black;
}


void uiFlatViewer::initView()
{
    setView( boundingBox() );
}


void uiFlatViewer::setDarkBG( bool yn )
{
    appearance().setDarkBG( yn );
    canvas_.setBGColor( color(false) );
}


void uiFlatViewer::setExtraBorders( const uiSize& lfttp, const uiSize& rghtbt )
{
    extraborders_.setLeft( lfttp.width() );
    extraborders_.setRight( rghtbt.width() );
    extraborders_.setTop( lfttp.height() );
    extraborders_.setBottom( rghtbt.height() );
}


void uiFlatViewer::setInitialSize( uiSize sz )
{
    canvas_.setPrefWidth( sz.width() );
    canvas_.setPrefHeight( sz.height() );
}


uiWorldRect uiFlatViewer::getBoundingBox( bool wva ) const
{
    const FlatPosData& pd = data().pos( wva );
    StepInterval<double> rg0( pd.range(true) ); rg0.sort( true );
    StepInterval<double> rg1( pd.range(false) ); rg1.sort( true );
    rg0.start -= dim0extfac_ * rg0.step; rg0.stop += dim0extfac_ * rg0.step;
    return uiWorldRect( rg0.start, rg1.stop, rg0.stop, rg1.start );
}


uiWorldRect uiFlatViewer::boundingBox() const
{
    const bool wvavisible = isVisible( true );
    uiWorldRect wr1 = getBoundingBox( wvavisible );
    if ( wvavisible && isVisible(false) )
    {
	uiWorldRect wr2 = getBoundingBox( false );
	if ( wr1.left() > wr2.left() )
	    wr1.setLeft( wr2.left() );
	if ( wr1.right() < wr2.right() )
	    wr1.setRight( wr2.right() );
	if ( wr1.top() < wr2.top() )
	    wr1.setTop( wr2.top() );
	if ( wr1.bottom() > wr2.bottom() )
	    wr1.setBottom( wr2.bottom() );
    }

    return wr1;
}


void uiFlatViewer::setView( uiWorldRect wr )
{
    anysetviewdone_ = true;
    wr_ = wr;
    if ( wr_.left() > wr.right() != appearance().annot_.x1_.reversed_ )
	wr_.swapHor();
    if ( wr_.bottom() > wr.top() != appearance().annot_.x2_.reversed_ )
	wr_.swapVer();
    handleChange( All );
    viewChanged.trigger();
}


void uiFlatViewer::handleChange( DataChangeType dct )
{
    reportedchange_ = dct;
    const FlatView::Annotation& annot = appearance().annot_;
    int l = extraborders_.left(); int r = extraborders_.right();
    int t = extraborders_.top(); int b = extraborders_.bottom();
    if ( annot.haveTitle() )
	t += annotsz_.height();
    if ( annot.haveAxisAnnot(true) )
	l += annotsz_.width();
    if ( annot.haveAxisAnnot(false) )
	{ b += annotsz_.height(); t += annotsz_.height(); }
    canvas_.setBorders( uiSize(l,t), uiSize(r,b) );
    canvas_.forceNewFill();
}


void uiFlatViewer::drawBitMaps()
{
    if ( !anysetviewdone_ ) initView();

    uiRGBArray& rgbarr = canvas_.rgbArray();
    uiSize uisz( rgbarr.getSize(true), rgbarr.getSize(false) );

    //TODO: use prev bmp data and changetype to optimise
    delete wvabmpmgr_; wvabmpmgr_ = new FlatView::BitMapMgr(*this,true);
    delete vdbmpmgr_; vdbmpmgr_ = new FlatView::BitMapMgr(*this,false);

    if ( !wvabmpmgr_->generate(wr_,uisz) || !vdbmpmgr_->generate(wr_,uisz) )
    {
	uiMSG().error( "No memory for bitmaps" );
	delete wvabmpmgr_; wvabmpmgr_ = 0; delete vdbmpmgr_; vdbmpmgr_ = 0;
	return;
    }

    bmp2rgb_->draw( wvabmpmgr_->bitMap(), vdbmpmgr_->bitMap() );
}


void uiFlatViewer::drawAnnot()
{
    const FlatView::Annotation& annot = appearance().annot_;
    ioDrawTool& dt = *canvas_.drawTool();
    dt.beginDraw();

    if ( annot.color_ != Color::NoColor )
    {
	dt.setPenColor( annot.color_ );
	drawGridAnnot();
    }

    for ( int idx=0; idx<annot.auxdata_.size(); idx++ )
	drawAux( *annot.auxdata_[idx] );

    if ( !annot.title_.isEmpty() )
    {
	dt.setPenColor( color(true) );
	dt.drawText( uiPoint(canvas_.arrArea().centre().x,2), annot.title_,
		     Alignment(Alignment::Middle,Alignment::Start) );
    }

    dt.endDraw();
    reportedchange_ = None;
}


int uiFlatViewer::getAnnotChoices( BufferStringSet& bss ) const
{
    const FlatDataPack* fdp = getPack( false );
    if ( !fdp ) fdp = getPack( true );
    if ( fdp )
	fdp->getAltDim0Keys( bss );
    if ( !bss.isEmpty() )
	bss.addIfNew( appearance().annot_.x1_.name_ );
    return axesdrawer_.altdim0_;
}


void uiFlatViewer::setAnnotChoice( int sel )
{
    BufferStringSet bss; getAnnotChoices( bss );
    if ( bss.get(sel) == appearance().annot_.x1_.name_ )
	mSetUdf(sel);
    axesdrawer_.altdim0_ = sel;
}


void uiFlatViewer::getWorld2Ui( uiWorld2Ui& w2u ) const
{
    w2u.set( canvas_.arrArea(), wr_ );
}


void uiFlatViewer::drawGridAnnot()
{
    const FlatView::Annotation& annot = appearance().annot_;
    const FlatView::Annotation::AxisData& ad1 = annot.x1_;
    const FlatView::Annotation::AxisData& ad2 = annot.x2_;
    const bool showanyx1annot = ad1.showannot_ || ad1.showgridlines_;
    const bool showanyx2annot = ad2.showannot_ || ad2.showgridlines_;
    if ( !showanyx1annot && !showanyx2annot )
	return;

    ioDrawTool& dt = *canvas_.drawTool();
    const uiRect datarect( canvas_.arrArea() );
    dt.drawRect( datarect );
    axesdrawer_.draw( datarect, wr_ );

    const uiSize totsz( canvas_.width(), canvas_.height() );
    if ( showanyx1annot && !ad1.name_.isEmpty() )
	dt.drawText( uiPoint(totsz.width()-2,totsz.height()-2), ad1.name_,
		     Alignment(Alignment::Stop,Alignment::Stop));
    if ( showanyx2annot && !ad2.name_.isEmpty() )
	dt.drawText( uiPoint(2,2), ad2.name_,
		     Alignment(Alignment::Start,Alignment::Start) );
}


void uiFlatViewer::drawAux( const FlatView::Annotation::AuxData& ad )
{
    pErrMsg( "TODO: implement Aux data draw" );
}
