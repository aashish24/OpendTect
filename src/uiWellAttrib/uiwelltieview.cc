/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Jan 2009
________________________________________________________________________

-*/

#include "uiwelltieview.h"
#include "uiwelltiecontrolview.h"

#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uiflatviewer.h"
#include "uifunctiondisplay.h"
#include "uilabel.h"
#include "uirgbarraycanvas.h"
#include "uiwelllogdisplay.h"
#include "uiwelldisplaycontrol.h"
#include "uiworld2ui.h"

#include "flatposdata.h"
#include "integerid.h"
#include "seistrc.h"
#include "seistrcprop.h"
#include "seisbufadapters.h"
#include "welldata.h"
#include "welllogset.h"
#include "welllog.h"
#include "welld2tmodel.h"
#include "wellmarker.h"

#include "welltiedata.h"
#include "welltiegeocalculator.h"
#include "welltiepickset.h"
#include "zaxistransform.h"


#define mGetWD(act) const Well::Data* wd = data_.wd_; if ( !wd ) act;

namespace WellTie
{

uiTieView::uiTieView( uiParent* p, uiFlatViewer* vwr, const Data& data )
    : vwr_(vwr)
    , parent_(p)
    , trcbuf_(*new SeisTrcBuf(true))
    , params_(data.dispparams_)
    , data_(data)
    , synthpickset_(data.pickdata_.synthpicks_)
    , seispickset_(data.pickdata_.seispicks_)
    , zrange_(data.getTraceRange())
    , checkshotitm_(0)
    , wellcontrol_(0)
    , seisdp_(0)
    , nrtrcs_(5)
    , infoMsgChanged(this)
{
    initFlatViewer();
    initLogViewers();
    initWellControl();

    linelog1_ = logsdisp_[0]->scene().addItem( new uiLineItem );
    linelog2_ = logsdisp_[1]->scene().addItem( new uiLineItem );
    lineseis_ = vwr_->rgbCanvas().scene().addItem( new uiLineItem );
    OD::LineStyle ls( OD::LineStyle::Dot, 1, Color::Black() );
    linelog1_->setPenStyle( ls );
    linelog2_->setPenStyle( ls );
    lineseis_->setPenStyle( ls );
}


uiTieView::~uiTieView()
{
    delete wellcontrol_;
    delete &trcbuf_;
}


void uiTieView::setNrTrcs( int nrtrcs )
{
    nrtrcs_ = nrtrcs > 0 ? nrtrcs : 5;
    redrawViewer();
}


void uiTieView::initWellControl()
{
    wellcontrol_ = new uiWellDisplayControl( *logsdisp_[0] );
    wellcontrol_->addDahDisplay( *logsdisp_[1] );
    wellcontrol_->posChanged.notify( mCB(this,uiTieView,setInfoMsg) );
}


void uiTieView::fullRedraw()
{
    setLogsParams();
    drawLog( data_.sKeySonic(), true, 0, !data_.isSonic() );
    drawLog( data_.sKeyDensity(), false, 0, false );
    drawLog( data_.sKeyAI(), true, 1, false );
    drawLog( data_.sKeyReflectivity(), false, 1, false );
    drawLogDispWellMarkers();

    redrawViewer();
}


void uiTieView::redrawViewer()
{
    drawTraces();
    redrawViewerAuxDatas();

    vwr_->handleChange( mCast(unsigned int,FlatView::Viewer::All) );
    mDynamicCastGet(uiControlView*,ctrl,vwr_->control())
    if ( ctrl )
	ctrl->setSelView( false, false );
    zoomChg(0);
}


void uiTieView::redrawViewerAuxDatas()
{
    drawUserPicks();
    drawViewerWellMarkers();
    drawHorizons();
    vwr_->handleChange( FlatView::Viewer::Auxdata );
}


void uiTieView::redrawLogsAuxDatas()
{
    drawLogDispWellMarkers();
}


void uiTieView::initLogViewers()
{
    displaygrp_ = new uiGroup( parent_, "Log Display" );
    displaygrp_->setBorder(0);
    for ( int idx=0; idx<2; idx++ )
    {
	uiWellLogDisplay::Setup wldsu; wldsu.nrmarkerchars(3);
	uiWellLogDisplay* logdisp = new uiWellLogDisplay( displaygrp_, wldsu );
	logsdisp_ += logdisp;
	logdisp->setSceneBorder( 2 );
	logdisp->setPrefWidth( vwr_->prefHNrPics()/2 );
	logdisp->setPrefHeight( vwr_->prefVNrPics() );
	logdisp->disableScrollZoom();
	logdisp->getMouseEventHandler().movement.notify(
				mCB(this,uiTieView,mouseMoveCB) );
    }
    logsdisp_[0]->attach( leftOf, logsdisp_[1] );

    displaygrp_->attach( leftOf, vwr_ );
}


uiGroup* uiTieView::displayGroup()
{
    return displaygrp_;
}


void uiTieView::initFlatViewer()
{
    vwr_->setInitialSize( uiSize(520,540) );
    vwr_->setExtraBorders( uiSize(0,3), uiSize(0,23) ); // trial and error
    FlatView::Appearance& app = vwr_->appearance();
    app.setDarkBG( false );
    app.setGeoDefaults( true );
    app.annot_.showaux_ = true ;
    app.annot_.x1_.showannot_ = true;
    app.annot_.x1_.sampling_ = 100;
    app.annot_.x1_.showgridlines_ = false;
    app.annot_.x2_.showannot_ = true;
    app.annot_.x2_.sampling_ = 0.1;
    app.annot_.x2_.showgridlines_ = true;
    app.ddpars_.show( true, false );
    app.ddpars_.wva_.mapper_->setup().setNoClipping();
    app.annot_.x1_.name_ = uiStrings::sSeisObjName(true,true,false);
    app.annot_.x2_.name_ =  uiStrings::sTWT();
    app.annot_.title_ = tr("Synthetics<--------------------"
			"------------------------------->Seismics");
    vwr_->viewChanged.notify( mCB(this,uiTieView,zoomChg) );
    vwr_->getMouseEventHandler().movement.notify(
				mCB(this,uiTieView,setInfoMsg) );
    vwr_->getMouseEventHandler().movement.notify(
				mCB(this,uiTieView,mouseMoveCB) );
}


void uiTieView::setLogsParams()
{
    mGetWD(return)
    for ( int idx=0; idx<logsdisp_.size(); idx++ )
    {
	logsdisp_[idx]->logData(true).setLog( 0 );
	logsdisp_[idx]->logData(false).setLog( 0 );
	uiWellDahDisplay::Data data( wd );
	data.dispzinft_ = params_.iszinft_;
	data.zistime_ = params_.iszintime_;
	logsdisp_[idx]->setData( data );
    }
    const float zfac =1;
    Interval<float> zrg( zrange_.start*zfac, zrange_.stop*zfac );
    setLogsRanges( zrg );
}


void uiTieView::drawLog( const char* nm, bool first, int dispnr, bool reversed )
{
    uiWellLogDisplay::LogData& wldld = logsdisp_[dispnr]->logData( first );
    wldld.setLog( data_.logset_.getLogByName( nm ) );
    wldld.col_ =  Color::stdDrawColor( first ? 0 : 1 );
    wldld.disp_.setFillLeft( false );
    wldld.disp_.setFillRight( false );
    wldld.xrev_ = reversed;
}


#define mNrTrcs 14
void uiTieView::drawTraces()
{
    trcbuf_.erase();
    const int nrtrcs = nrtrcs_*2 + 4;
    for ( int idx=0; idx<nrtrcs; idx++ )
    {
	const int midtrc = nrtrcs/2-1;
	const bool issynth = idx < midtrc;
	SeisTrc* trc = new SeisTrc;
	trc->copyDataFrom( issynth ? data_.synthtrc_ : data_.seistrc_ );
	trc->info().sampling_ = data_.getTraceRange();
	trcbuf_.add( trc );
	bool udf = idx == 0 || idx == midtrc || idx == midtrc+1 || idx>nrtrcs-2;
	if ( udf )
	    setUdfTrc( *trc );
	else
	    { SeisTrcPropChg pc( *trc ); pc.normalize( true ); }
    }
    setDataPack();
}


void uiTieView::setUdfTrc( SeisTrc& trc ) const
{
    for ( int idx=0; idx<trc.size(); idx++)
	trc.set( idx, mUdf(float), 0 );
}


void uiTieView::setDataPack()
{
    vwr_->clearAllPacks();
    SeisTrcBufDataPack* dp = new SeisTrcBufDataPack( &trcbuf_, Seis::Vol,
				SeisTrcInfo::TrcNr, "Seismic" );
    dp->trcBufArr2D().setBufMine( false );
    StepInterval<double> xrange( 1, trcbuf_.size(), 1 );
    dp->posData().setRange( true, xrange );
    dp->setName( data_.sKeySeismic() );
    DPM(DataPackMgr::FlatID()).add( dp );
    vwr_->setPack( true, dp->id(), false );
    vwr_->setPack( false, dp->id(), false );
}


void uiTieView::setLogsRanges( Interval<float> rg )
{
    for (int idx=0; idx<logsdisp_.size(); idx++)
	logsdisp_[idx]->setZRange( rg );
}


void uiTieView::zoomChg( CallBacker* )
{
    const uiWorldRect& curwr = vwr_->curView();
    const float userfac = SI().showZ2UserFactor();
    Interval<float> zrg;

    if ( !params_.iszintime_  && SI().depthsInFeet() )
    {
	float zrgstart = data_.wd_->d2TModel().getDah(
				mCast(float, curwr.top()), data_.wd_->track() );
	float zrgstop = data_.wd_->d2TModel().getDah(
			    mCast(float, curwr.bottom()), data_.wd_->track() );
	if ( mIsUdf(zrgstop) )
	{
	    const int sz = data_.wd_->d2TModel().size();
	    Well::D2TModel::PointID pid =
					 data_.wd_->d2TModel().pointIDFor(sz-1);
	    zrgstop = data_.wd_->d2TModel().dah(pid);
	}
	zrgstart = (float) data_.wd_->track().getPos(zrgstart).z_;
	zrgstop = (float) data_.wd_->track().getPos(zrgstop).z_;
	zrg.start = zrgstart;
	zrg.stop = zrgstop;
    }
    else
    {
	zrg.start = (float) curwr.top()*userfac;
	zrg.stop = (float) curwr.bottom()*userfac;
    }
    setLogsRanges( zrg );
}


void uiTieView::drawMarker( FlatView::AuxData* auxdata,
				bool left, float zpos )
{
    Interval<float> xrg( (float) vwr_->boundingBox().left(),
				    (float) vwr_->boundingBox().right() );
    auxdata->poly_ += FlatView::Point( left ? xrg.start : xrg.width()/2, zpos );
    auxdata->poly_ += FlatView::Point( left ? xrg.width()/2 : xrg.stop, zpos );
}


void uiTieView::drawLogDispWellMarkers()
{
    mGetWD(return)
    for ( int idx=0; idx<logsdisp_.size(); idx++ )
    {
	logsdisp_[idx]->markerDisp() = data_.dispparams_.mrkdisp_;
	logsdisp_[idx]->reDrawAnnots();
    }
}

#define mRemoveItms( itms ) \
    for ( int idx=0; idx<itms.size(); idx++ ) \
	vwr_->rgbCanvas().scene().removeItem( itms[idx] ); \
    deepErase( itms );
#define mRemoveSet( auxs ) \
    for ( int idx=0; idx<auxs.size(); idx++ ) \
        vwr_->removeAuxData( auxs[idx] ); \
    deepErase( auxs );
void uiTieView::drawViewerWellMarkers()
{
    mRemoveItms( mrktxtnms_ )
    mGetWD(return)

    mRemoveSet( wellmarkerauxdatas_ );
    bool ismarkerdisp = params_.isvwrmarkerdisp_;
    if ( !ismarkerdisp ) return;

    const Well::D2TModel& d2tm = wd->d2TModel();
    if ( d2tm.isEmpty() )
	return;

    const Well::MarkerDispProps& mrkdisp = data_.dispparams_.mrkdisp_;
    const BufferStringSet selmrknms = mrkdisp.selMarkerNames();
    const Well::MarkerSet& markers = wd->markers();
    Well::MarkerSetIter miter( markers );
    while( miter.next() )
    {
	const Well::Marker& marker = miter.get();
	if ( marker.isUdf()  ) continue;

	if ( !selmrknms.isPresent( marker.name() ) )
	    continue;

	const float zpos = d2tm.getTime( marker.dah(), wd->track() );

	if ( !zrange_.includes( zpos, true ) )
	    continue;

	const Color col = mrkdisp.singleColor() ? mrkdisp.color()
						: marker.color();

	if ( col == Color::NoColor() || col == Color::White() )
	    continue;

	FlatView::AuxData* auxdata = vwr_->createAuxData( marker.name() );
	if ( !auxdata ) continue;

	wellmarkerauxdatas_ += auxdata;
	vwr_->addAuxData( auxdata );
	const int shapeint = mrkdisp.shapeType();
	const int drawsize = mrkdisp.size();
	OD::LineStyle ls = OD::LineStyle( OD::LineStyle::Dot, drawsize, col );
	if ( shapeint == 1 )
	    ls.type_ =  OD::LineStyle::Solid;
	if ( shapeint == 2 )
	    ls.type_ = OD::LineStyle::Dash;
	auxdata->linestyle_ = ls;

	BufferString mtxt( marker.name() );
	mtxt.insertAt( 0, " " );
	if ( !params_.dispmrkfullnames_ && mtxt.size()>4 )
	    mtxt[4] = '\0';
	auxdata->name_ = toUiString(mtxt);
	auxdata->namealignment_ = OD::Alignment(OD::Alignment::Left,
						OD::Alignment::Top);
	auxdata->namepos_ = 0;

	drawMarker( auxdata, true, zpos );
    }
}


void uiTieView::drawUserPicks()
{
    mRemoveSet( userpickauxdatas_ );
    const int nrauxs = mMAX( seispickset_.size(), synthpickset_.size() );

    for ( int idx=0; idx<nrauxs; idx++ )
    {
	FlatView::AuxData* auxdata = vwr_->createAuxData( 0 );
	userpickauxdatas_ += auxdata;
	vwr_->addAuxData( auxdata );
    }
    drawUserPicks( synthpickset_, true );
    drawUserPicks( seispickset_, false );
}


void uiTieView::drawUserPicks( const TypeSet<Marker>& pickset, bool issynth )
{
    for ( int idx=0; idx<pickset.size(); idx++ )
    {
	const Marker& pick = pickset[idx];
	OD::LineStyle ls = OD::LineStyle( OD::LineStyle::Solid, pick.size_,
								pick.color_ );
	userpickauxdatas_[idx]->linestyle_ = ls;
	drawMarker(userpickauxdatas_[idx], issynth, pick.zpos_ );
    }
}


void uiTieView::drawHorizons()
{
    mRemoveItms( hortxtnms_ )
    mRemoveSet( horauxdatas_ );
    bool ishordisp = params_.isvwrhordisp_;
    if ( !ishordisp ) return;
    const TypeSet<Marker>& horizons = data_.horizons_;
    for ( int idx=0; idx<horizons.size(); idx++ )
    {
	FlatView::AuxData* auxdata = vwr_->createAuxData( 0 );
	horauxdatas_ += auxdata;
	vwr_->addAuxData( auxdata );
	const Marker& hor = horizons[idx];
	float zval = hor.zpos_*(1.f/SI().zDomain().userFactor());

	BufferString mtxt( hor.name_ );
	if ( !params_.disphorfullnames_ && mtxt.size() > 3 )
	    mtxt[3] = '\0';
	auxdata->name_ = toUiString(mtxt);
	auxdata->namealignment_ = OD::Alignment(OD::Alignment::HCenter,
						OD::Alignment::Top);
	auxdata->namepos_ = 0;
	OD::LineStyle ls = OD::LineStyle( OD::LineStyle::Dot, 2, hor.color_ );
	auxdata->linestyle_ = ls;

	drawMarker( auxdata, false, zval );
    }
}


void uiTieView::setInfoMsg( CallBacker* cb )
{
    BufferString infomsg;
    mDynamicCastGet(MouseEventHandler*,mevh,cb)
    if ( !mevh )
    {
	mCBCapsuleUnpack(BufferString,inf,cb);
	infomsg = inf;
    }
    CBCapsule<BufferString> caps( infomsg, this );
    infoMsgChanged.trigger( &caps );
}


void uiTieView::mouseMoveCB( CallBacker* cb )
{
    mDynamicCastGet(MouseEventHandler*,mevh,cb)
    if ( !mevh ) return;

    const MouseEvent& ev = mevh->event();
    const int ypos = ev.y();
    uiRect rect = logDisps()[0]->getViewArea();
    linelog1_->setLine( rect.left(), ypos, rect.right(), ypos );

    rect = logDisps()[1]->getViewArea();
    linelog2_->setLine( rect.left(), ypos, rect.right(), ypos );

    rect = vwr_->rgbCanvas().getViewArea();
    lineseis_->setLine( rect.left(), ypos, rect.right(), ypos );
}


void uiTieView::enableCtrlNotifiers( bool yn )
{
    logsdisp_[0]->scene().getMouseEventHandler().movement.enable( yn );
    logsdisp_[1]->scene().getMouseEventHandler().movement.enable( yn );
}


uiCrossCorrView::uiCrossCorrView( uiParent* p, const Data& d )
	: uiGroup(p)
	, data_(d)
{
    disp_ = new uiFunctionDisplay( this, uiFunctionDisplay::Setup() );
    disp_->xAxis()->setCaption( tr("Lags (ms)") );
    disp_->yAxis(false)->setCaption( uiStrings::sCoefficient() );
    lbl_ = new uiLabel( this,uiString::empty() );
    lbl_->attach( centeredAbove, disp_ );
}


void uiCrossCorrView::set( const Data::CorrelData& cd )
{
    vals_.erase();
    for ( int idx=0; idx<cd.vals_.size(); idx++ )
	vals_ += cd.vals_[idx];
    lag_ = mCast( float, cd.lag_ );
    coeff_ = (float) cd.coeff_;
}


void uiCrossCorrView::draw()
{
    const int halfsz = vals_.size()/2;
    if ( !vals_.validIdx( halfsz ) )
	return;

    const float normalfactor = coeff_ / vals_[halfsz];
    TypeSet<float> xvals, yvals;
    for ( int idx=-halfsz; idx<halfsz; idx++)
    {
	float xaxistime = idx *
			 data_.getTraceRange().step*SI().zDomain().userFactor();
	if ( fabs( xaxistime ) > lag_ )
	    continue;
	xvals += xaxistime;
	float val = vals_[idx+halfsz];
	val *= normalfactor;
	yvals += fabs(val)>1 ? 0 : val;
    }
    disp_->setVals( xvals.arr(), yvals.arr(), xvals.size() );
    uiString corrbuf = tr("Cross-Correlation Coefficient: %1").arg(coeff_);
    lbl_->setPrefWidthInChar(50);
    lbl_->setText( corrbuf );
}

} // namespace WellTie
