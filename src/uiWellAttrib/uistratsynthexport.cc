/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki / Bert
 Date:          July 2013
_______________________________________________________________________

-*/

#include "uistratsynthexport.h"

#include "uigeninput.h"
#include "uiimpexp2dgeom.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uipicksetsel.h"
#include "uiseislinesel.h"
#include "uiseissel.h"
#include "uiselsimple.h"
#include "uiseparator.h"
#include "uitaskrunner.h"

#include "dbman.h"
#include "emhorizon2d.h"
#include "emmanager.h"
#include "ioobjctxt.h"
#include "od_helpids.h"
#include "picksetmanager.h"
#include "posinfo2dsurv.h"
#include "prestackgather.h"
#include "prestacksyntheticdata.h"
#include "randomlinegeom.h"
#include "randomlinetr.h"
#include "seisbufadapters.h"
#include "stratlevel.h"
#include "stratsynthdatamgr.h"
#include "stratsynthexp.h"
#include "stratsynthlevel.h"
#include "survgeom2d.h"
#include "survinfo.h"
#include "syntheticdataimpl.h"
#include "velocitycalc.h"
#include "zdomain.h"


#define mErrRet( msg, rettyp ) \
{ \
    uiMSG().error( msg ); \
    return rettyp;\
}


class uiStratSynthOutSel : public uiCheckedCompoundParSel
{ mODTextTranslationClass(uiStratSynthOutSel);
public:

uiStratSynthOutSel( uiParent* p, const uiString& seltxt,
		    const BufferStringSet& nms )
    : uiCheckedCompoundParSel( p, seltxt, false, uiStrings::sSelect() )
    , nms_(nms)
    , nm_(seltxt)
{
    butPush.notify( mCB(this,uiStratSynthOutSel,selItems) );
}

void selItems( CallBacker* )
{
    uiDialog::Setup su( uiStrings::phrSelect(nm_), mNoDlgTitle,
			mODHelpKey(mStartSynthOutSelHelpID) );
    uiDialog dlg( parent(), su );
    uiListBox* lb = new uiListBox( &dlg, toString(nm_) );
    lb->setMultiChoice( true );
    lb->addItems( nms_ );

    if ( itmsarelevels_ )
    {
	const auto& lvls = Strat::LVLS();
	MonitorLock ml( lvls );
	for ( int idx=0; idx<nms_.size(); idx++ )
	{
	    const Strat::Level::ID id = lvls.getIDByIdx( idx );
	    lb->setColorIcon( idx, Strat::LVLS().levelColor(id) );
	}
    }

    for ( int idx=0; idx<selidxs_.size(); idx++ )
	lb->setChosen( selidxs_[idx], true );

    if ( dlg.go() )
    {
	selidxs_.erase();
	selnms_.setEmpty();
	for ( int idx=0; idx<lb->size(); idx++ )
	{
	    if ( lb->isChosen(idx) )
	    {
		selidxs_ += idx;
		selnms_.add( lb->itemText(idx) );
	    }
	}
    }
}


virtual uiString getSummary() const
{
    uiString ret;
    const int sz = nms_.size();
    const int selsz = selidxs_.size();

    if ( sz < 1 )
	ret = tr("None available").embedFinalState();
    else if ( selsz == 0 )
	ret = tr("None selected").embedFinalState();
    else
    {
	if ( selsz > 1 )
	{
	    ret = toUiString("%1 %2").arg(selsz).arg(uiStrings::sSelected());
	    if ( sz == selsz )
		ret.appendPhrase( uiStrings::sAll().toLower().parenthesize(),
				    uiString::Space,uiString::OnSameLine );
	    ret.embedFinalState();
	}
	ret.appendPhrase(toUiString(nms_.get( selidxs_[0] )), uiString::NoSep);
	if ( selsz > 1 )
	    ret.appendPlainText( ", ..." );
	else if ( sz == selsz )
	    ret.appendPhrase( uiStrings::sAll().toLower().parenthesize(),
				    uiString::Space,uiString::OnSameLine );
    }

    return ret;
}

    const uiString	    nm_;
    const BufferStringSet   nms_;
    BufferStringSet	    selnms_;
    TypeSet<int>	    selidxs_;
    bool		    itmsarelevels_	= false;

    uiListBox*		    listfld_;

};



uiStratSynthExport::uiStratSynthExport( uiParent* p,
					const StratSynth::DataMgr& ssdm )
    : uiDialog(p,uiDialog::Setup(tr("Save synthetic seismics and horizons"),
				 mNoDlgTitle,
				 mODHelpKey(mStratSynthExportHelpID) ) )
    , ssdm_(ssdm)
    , randlinesel_(0)
{
    crnewfld_ = new uiGenInput( this, tr("2D Line"),
			     BoolInpSpec(true,uiStrings::phrCreate(
			     uiStrings::sNew()), tr("Use existing")) );
    crnewfld_->valuechanged.notify( mCB(this,uiStratSynthExport,crNewChg) );


    newlinenmfld_ = new uiGenInput( this, tr("New Line Name"),
				    StringInpSpec() );
    newlinenmfld_->attach( alignedBelow, crnewfld_ );
    existlinenmsel_ = new uiSeis2DLineNameSel( this, true );
    existlinenmsel_->fillWithAll();
    existlinenmsel_->attach( alignedBelow, crnewfld_ );

    uiSeparator* sep = new uiSeparator( this, "Hsep 1" );
    sep->attach( stretchedBelow, existlinenmsel_ );

    geomgrp_ = new uiGroup( this, "Geometry group" );
    fillGeomGroup();
    geomgrp_->attach( alignedBelow, existlinenmsel_ );
    geomgrp_->attach( ensureBelow, sep );

    sep = new uiSeparator( this, "Hsep 2" );
    sep->attach( stretchedBelow, geomgrp_ );

    uiGroup* selgrp = new uiGroup( this, "Export sel group" );
    selgrp->attach( ensureBelow, sep );

    BufferStringSet nms; addNames( postsds_, nms );
    poststcksel_ = new uiStratSynthOutSel( selgrp, tr("Post-stack line data")
									,nms );
    nms.erase(); addNames( ssdm_.levels().levels(), nms );
    horsel_ = new uiStratSynthOutSel( selgrp, uiStrings::s2DHorizon(mPlural)
							    .toLower(), nms );
    horsel_->attach( alignedBelow, poststcksel_ );
    horsel_->itmsarelevels_ = true;

    nms.erase(); addNames( presds_, nms );
    prestcksel_ = new uiStratSynthOutSel( selgrp, tr("PreStack Data"), nms );
    prestcksel_->attach( alignedBelow, horsel_ );

    selgrp->setHAlignObj( poststcksel_ );
    selgrp->attach( alignedBelow, geomgrp_ );

    uiLabel* lbl = new uiLabel( this, tr("Output object names will "
					 "be generated.\nYou can specify "
					 "an optional prefix and postfix "
					 "for each:") );
    lbl->attach( ensureBelow, selgrp );
    lbl->setAlignment( OD::Alignment::Left );
    prefxfld_ = new uiGenInput( this, uiStrings::sPrefix() );
    prefxfld_->attach( alignedBelow, selgrp );
    prefxfld_->attach( ensureBelow, lbl );
    postfxfld_ = new uiGenInput( this, uiStrings::sPostfix() );
    postfxfld_->attach( rightOf, prefxfld_ );

    postFinalise().notify( mCB(this,uiStratSynthExport,crNewChg) );
}


uiStratSynthExport::~uiStratSynthExport()
{
    deepUnRef( postsds_ );
    deepUnRef( presds_ );
}


void uiStratSynthExport::fillGeomGroup()
{
    StringListInpSpec inpspec;
    inpspec.addString(tr("Straight line"));
    inpspec.addString(uiStrings::sPolygon());
    const bool haverl = SI().has3D();
    if ( haverl )
	inpspec.addString( uiStrings::sRandomLine() );
    geomsel_ = new uiGenInput( geomgrp_, tr("Geometry for line"), inpspec );
    geomsel_->valuechanged.notify( mCB(this,uiStratSynthExport,geomSel) );
    geomgrp_->setHAlignObj( geomsel_ );

    BinID startbid( SI().inlRange(true).snappedCenter(),
		    SI().crlRange(true).start );
    Coord startcoord = SI().transform( startbid );
    BinID stopbid( SI().inlRange(true).snappedCenter(),
		   SI().crlRange(true).stop );
    Coord stopcoord = SI().transform( stopbid );
    coord0fld_ = new uiGenInput( geomgrp_, tr("Coordinates: from"),
					DoubleInpSpec(), DoubleInpSpec() );
    coord0fld_->attach( alignedBelow, geomsel_ );
    coord0fld_->setValue( startcoord.x_, 0 );
    coord0fld_->setValue( startcoord.y_, 1 );
    coord1fld_ = new uiGenInput( geomgrp_, tr("to"),
					DoubleInpSpec(), DoubleInpSpec() );
    coord1fld_->attach( alignedBelow, coord0fld_ );
    coord1fld_->setValue( stopcoord.x_, 0 );
    coord1fld_->setValue( stopcoord.y_, 1 );

    picksetsel_ = new uiPickSetIOObjSel( geomgrp_, true,
					 uiPickSetIOObjSel::PolygonOnly );
    picksetsel_->attach( alignedBelow, geomsel_ );
    if ( haverl )
    {
	randlinesel_ = new uiIOObjSel( geomgrp_, mIOObjContext(RandomLineSet) );
	randlinesel_->attach( alignedBelow, geomsel_ );
    }
}


void uiStratSynthExport::getExpObjs()
{
    if ( ssdm_.nrSynthetics() < 1 )
	return;

    deepUnRef( postsds_ ); deepUnRef( presds_ );
    for ( int idx=0; idx<ssdm_.nrSynthetics(); idx++ )
    {
	ConstRefMan<SyntheticData> sd = ssdm_.getSyntheticByIdx( idx );
	(sd->isPS() ? presds_ : postsds_) += sd;
    }

    deepRef( postsds_ );
    deepRef( presds_ );
}


void uiStratSynthExport::crNewChg( CallBacker* )
{
    const bool iscreate = crnewfld_->getBoolValue();
    newlinenmfld_->display( iscreate );
    existlinenmsel_->display( !iscreate );
    geomgrp_->display( iscreate );
    if ( iscreate )
	geomSel( 0 );
}


void uiStratSynthExport::geomSel( CallBacker* )
{
    const int selgeom = crnewfld_->getBoolValue() ? geomsel_->getIntValue(): -1;
    coord0fld_->display( selgeom == 0 );
    coord1fld_->display( selgeom == 0 );
    picksetsel_->display( selgeom == 1 );
    if ( randlinesel_ )
	randlinesel_->display( selgeom == 2 );
}


void uiStratSynthExport::create2DGeometry( const TypeSet<Coord>& ptlist,
					   PosInfo::Line2DData& geom )
{
    geom.setEmpty();
    int synthmodelsz = mUdf(int);
    if ( postsds_.isEmpty() )
    {
	if ( !presds_.isEmpty() )
	{
	    mDynamicCastGet(const PreStack::PreStackSyntheticData*,
			    presd,presds_[0]);
	    synthmodelsz = presd->preStackPack().getGathers().size();
	}
    }
    else
    {
	mDynamicCastGet(const PostStackSyntheticData*,postsd,postsds_[0]);
	synthmodelsz = postsd->postStackPack().trcBuf().size();
    }

    int trcnr = 0;
    for ( int idx=0; idx<ptlist.size()-1; idx++ )
    {
	Coord startpos = ptlist[idx];
	Coord stoppos = ptlist[idx+1];
	const float dist = startpos.distTo<float>( stoppos );
	const double unitdist = mMAX( SI().inlStep() * SI().inlDistance(),
				      SI().crlStep() * SI().crlDistance() );
	const int nrsegs = mNINT32( dist / unitdist );
	const double unitx = ( stoppos.x_ - startpos.x_ ) / nrsegs;
	const double unity = ( stoppos.y_ - startpos.y_ ) / nrsegs;
	for ( int nidx=0; nidx<nrsegs; nidx++ )
	{
	    const double curx = startpos.x_ + nidx * unitx;
	    const double cury = startpos.y_ + nidx * unity;
	    Coord curpos( curx, cury );
	    trcnr++;
	    PosInfo::Line2DPos pos( trcnr );
	    pos.coord_ = curpos;
	    geom.add( pos );
	    if ( synthmodelsz <= trcnr )
		return;
	}

	trcnr++;
	PosInfo::Line2DPos stop2dpos( trcnr );
	stop2dpos.coord_ = stoppos;
	geom.add( stop2dpos );
	if ( synthmodelsz <= trcnr )
	    return;
    }
}


uiStratSynthExport::GeomSel uiStratSynthExport::selType() const
{
    return crnewfld_->getBoolValue()
	? (uiStratSynthExport::GeomSel)geomsel_->getIntValue()
	: uiStratSynthExport::Existing;
}


Pos::GeomID uiStratSynthExport::getGeometry( PosInfo::Line2DData& linegeom )
{
    uiStratSynthExport::GeomSel selgeom = selType();
    TypeSet<Coord> ptlist;
    switch ( selgeom )
    {
	case Existing:
	{
	    const Survey::Geometry* geom =
		Survey::GM().getGeometry( linegeom.lineName() );
	    mDynamicCastGet(const Survey::Geometry2D*,geom2d,geom);
	    if ( !geom2d )
		mErrRet(uiStrings::phrCannotFind(
			    tr("the geometry of specified line")), mUdfGeomID )
	    linegeom = geom2d->data();
	    return geom->getID();
	}
	case StraightLine:
	{
	    ptlist += Coord(coord0fld_->getDValue(0), coord0fld_->getDValue(1));
	    ptlist += Coord(coord1fld_->getDValue(0), coord1fld_->getDValue(1));
	    break;
	}
	case Polygon:
	{
	    ConstRefMan<Pick::Set> ps = picksetsel_->getPickSet();
	    if ( !ps )
		return mUdfGeomID;
	    Pick::SetIter psiter( *ps );
	    while ( psiter.next() )
		ptlist += psiter.get().pos().getXY();
	    break;
	}
	case RandomLine:
	{
	    const IOObj* randlineobj = randlinesel_->ioobj();
	    if ( !randlineobj )
		mErrRet( tr("No random line selected"), mUdfGeomID )
	    Geometry::RandomLineSet lset;
	    uiString errmsg;
	    if ( !RandomLineSetTranslator::retrieve(lset,randlineobj,errmsg) )
		mErrRet( errmsg, mUdfGeomID )
	    const ObjectSet<Geometry::RandomLine>& lines = lset.lines();
	    BufferStringSet linenames;
	    for ( int idx=0; idx<lines.size(); idx++ )
		linenames.add( lines[idx]->name() );
	    int selitem = 0;
	    if ( linenames.isEmpty() )
		mErrRet( tr("Random line appears to be empty"), mUdfGeomID )
	    else if ( linenames.size()>1 )
	    {
		uiSelectFromList seldlg( this,
			uiSelectFromList::Setup(tr("Random lines"),linenames) );
		selitem = seldlg.selection();
	    }

	    const Geometry::RandomLine& rdmline = *lset.lines()[ selitem ];
	    for ( int nidx=0; nidx<rdmline.nrNodes(); nidx++ )
		ptlist += SI().transform( rdmline.nodePosition(nidx) );
	    break;
	}
    }

    Survey::Geometry::ID newgeomid =
		Geom2DImpHandler::getGeomID( linegeom.lineName() );
    if ( !mIsUdfGeomID(newgeomid) )
	create2DGeometry( ptlist, linegeom );

    return newgeomid;
}


void uiStratSynthExport::addPrePostFix( BufferString& oldnm ) const
{
    BufferString newnm;

    BufferString pfx( prefxfld_->text() );
    if ( !pfx.isEmpty() )
	newnm.add( pfx ).add( "_" );

    newnm += oldnm.buf();
    pfx = postfxfld_->text();
    if ( !pfx.isEmpty() )
	newnm.add( "_" ).add( pfx );

    oldnm = newnm;
}


bool uiStratSynthExport::createHor2Ds()
{
    EM::ObjectManager& mgr = EM::Hor2DMan();
    const bool createnew = crnewfld_->getBoolValue();
    if ( createnew && (presds_.isEmpty() && postsds_.isEmpty()) )
	mErrRet(tr("Cannot create horizon without a geometry. Select any "
		   "synthetic data to create a new geometry or use existing "
		   "2D line"), false);
    const char* linenm = createnew ? newlinenmfld_->text()
				   : existlinenmsel_->getInput();
    const Pos::GeomID geomid = Survey::GM().getGeomID( linenm );
    if ( geomid == Survey::GeometryManager::cUndefGeomID() )
	return false;

    const Survey::Geometry2D* geom2d = Survey::GM().getGeometry(geomid)->as2D();
    StepInterval<Pos::TraceID> trcnrrg = geom2d->data().trcNrRange();
    ssdm_.updateLevelInfo();
    for ( int horidx=0; horidx<sellvls_.size(); horidx++ )
    {
	const Strat::Level stratlvl
		= Strat::LVLS().getByName( sellvls_.get(horidx) );
	if ( stratlvl.id().isInvalid() )
	    continue;

	BufferString hornm( stratlvl.name() );
	addPrePostFix( hornm );
	EM::Object* emobj = mgr.createObject(EM::Horizon2D::typeStr(),hornm);
	mDynamicCastGet(EM::Horizon2D*,horizon2d,emobj);
	if ( !horizon2d )
	    continue;

	horizon2d->geometry().addLine( geomid );
	StratSynth::DataMgr::ZValueSet zvals;
	ssdm_.getLevelDepths( stratlvl.id(), zvals );
	for ( int trcidx=0; trcidx<zvals.size(); trcidx++ )
	{
	    const int trcnr = trcnrrg.atIndex( trcidx );
	    horizon2d->setZPos( geomid, trcnr, zvals[trcidx], false );
	}

	PtrMan<Executor> saver = horizon2d->saver();
	uiTaskRunner taskrunner( this );
	if ( !TaskRunner::execute(&taskrunner,*saver) )
	    mErrRet( saver->message(), false );
    }

    return false;
}

void uiStratSynthExport::getSelections()
{
    TypeSet<int> selids;

    if ( poststcksel_->isChecked() )
    {
	selids = poststcksel_->selidxs_;
	for ( int idx=postsds_.size()-1; idx>=0; idx-- )
	{
	    if ( !selids.isPresent(idx) )
		postsds_.removeSingle( idx )->unRef();
	}
    }
    else
	deepUnRef( postsds_ );

    if ( prestcksel_->isChecked() )
    {
	selids = prestcksel_->selidxs_;
	for ( int idx=presds_.size()-1; idx>=0; idx-- )
	{
	    if ( !selids.isPresent(idx) )
		presds_.removeSingle( idx )->unRef();
	}
    }
    else
	deepUnRef( presds_ );

    if ( horsel_->isChecked() )
	sellvls_ = horsel_->selnms_;
    else
	sellvls_.erase();
}


bool uiStratSynthExport::acceptOK()
{
    getSelections();

    if ( presds_.isEmpty() && postsds_.isEmpty() && sellvls_.isEmpty() )
    {
	getExpObjs();
	mErrRet( tr("Nothing selected for export"), false );
    }

    const bool useexisting = selType()==uiStratSynthExport::Existing;
    if ( !useexisting && postsds_.isEmpty() )
    {
	getExpObjs();
	mErrRet(tr("No post stack selected. Since a new geometry will be "
		   "created you need to select atleast one post stack data to "
		   "create a 2D line geometry."), false);
    }

    BufferString linenm =
	crnewfld_->getBoolValue() ? newlinenmfld_->text()
				  : existlinenmsel_->getInput();
    if ( linenm.isEmpty() )
    {
	getExpObjs();
	mErrRet( tr("No line name specified"), false );
    }

    PtrMan<PosInfo::Line2DData> linegeom = new PosInfo::Line2DData( linenm );
    Pos::GeomID newgeomid = getGeometry( *linegeom );
    if ( mIsUdfGeomID(newgeomid) )
    {
	getExpObjs();
	return false;
    }

    int synthmodelsz = linegeom->positions().size();
    if ( !postsds_.isEmpty() )
    {
	mDynamicCastGet(const PostStackSyntheticData*,postsd,postsds_[0]);
	synthmodelsz = postsd->postStackPack().trcBuf().size();
    }
    else if ( !presds_.isEmpty() )
    {
	mDynamicCastGet(const PreStack::PreStackSyntheticData*,presd,
			presds_[0]);
	synthmodelsz = presd->preStackPack().getGathers().size();
    }

    if ( linegeom->positions().size() < synthmodelsz )
	uiMSG().warning(tr("The geometry of the line could not accomodate \n"
			   "all the traces from the synthetics. Some of the \n"
			   "end traces will be clipped"));
    SeparString prepostfix;
    prepostfix.add( prefxfld_->text() );
    prepostfix.add( postfxfld_->text() );
    ObjectSet<const SyntheticData> sds( postsds_ );
    sds.append( presds_ );
    deepRef( sds );
    if ( !sds.isEmpty() )
    {
	StratSynthExporter synthexp( sds, newgeomid, linegeom, prepostfix );
	uiTaskRunner taskrunner( this );
	const bool res = TaskRunner::execute( &taskrunner, synthexp );
	if ( !res )
	    return false;
    }

    createHor2Ds();
    if ( !SI().has2D() )
	uiMSG().warning(tr("You need to change survey type to 'Both 2D and 3D'"
			   " in survey setup to display the 2D line"));
    deepUnRef( sds );
    return true;
}
