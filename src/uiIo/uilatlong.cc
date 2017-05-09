/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uilatlong2coord.h"
#include "uilatlonginp.h"
#include "latlong.h"
#include "survinfo.h"
#include "uifileinput.h"
#include "uilabel.h"
#include "uitoolbutton.h"
#include "uibuttongroup.h"
#include "uilineedit.h"
#include "uispinbox.h"
#include "uimsg.h"
#include "od_iostream.h"
#include "od_helpids.h"


using namespace Coords;

mImplFactory1Param( uiPositionSystem, uiParent*,
		    uiPositionSystem::factory );


uiPositionSystem::uiPositionSystem( uiParent* p )
    : uiGroup(p,"PositionSystem")
    , si_(0)
{}

#define mErrRet(msg) { uiMSG().error( msg ); return false; }

class uiLatLongDMSInp : public uiGroup
{ mODTextTranslationClass(uiLatLongDMSInp);
public:
			uiLatLongDMSInp(uiParent*,bool lat);

    double		value() const;
    void		set(double);

protected:

    bool	islat_;

    uiSpinBox*	degfld_;
    uiSpinBox*	minfld_;
    uiLineEdit*	secfld_;
    uiCheckBox*	swfld_;

};


uiLatLongDMSInp::uiLatLongDMSInp( uiParent* p, bool lat )
    : uiGroup(p,BufferString(lat?"lat":"long"," DMS group"))
    , islat_(lat)
{
    const char* nm = islat_ ? "Latitude" : "Longitude";
    degfld_ = new uiSpinBox( this, 0, BufferString("DMS ",nm," deg") );
    if ( islat_ )
	degfld_->setInterval( 0, 90, 1 );
    else
	degfld_->setInterval( 0, 180, 1 );
    degfld_->setValue( 0 );
    minfld_ = new uiSpinBox( this, 0, BufferString("DMS ",nm," min") );
    minfld_->setInterval( 0, 59, 1 );
    minfld_->setValue( 0 );
    minfld_->attach( rightOf, degfld_ );
    secfld_ = new uiLineEdit( this, FloatInpSpec(),
			      BufferString("DMS ",nm," sec") );
    secfld_->setHSzPol( uiObject::Small );
    secfld_->attach( rightOf, minfld_ );
    secfld_->setValue( 0 );

    swfld_ = new uiCheckBox( this, islat_ ? uiStrings::sSouth(true) : 
					    uiStrings::sWest(true) );
    swfld_->attach( rightOf, secfld_ );
    swfld_->setHSzPol( uiObject::Small );
}


double uiLatLongDMSInp::value() const
{
    int sign = swfld_->isChecked() ? -1 : 1;
    const int d = sign * degfld_->getValue();
    const int m = sign * minfld_->getValue();
    const float s = sign * secfld_->getfValue();
    LatLong ll; ll.setDMS( islat_, d, m, s );
    return islat_ ? ll.lat_ : ll.lng_;
}


void uiLatLongDMSInp::set( double val )
{
    LatLong ll;
    (islat_ ? ll.lat_ : ll.lng_) = val;
    int d, m; float s;
    ll.getDMS( islat_, d, m, s );
    const bool issw = val < 0;
    swfld_->setChecked( issw );
    const int sign = issw ? -1 : 1;
    degfld_->setValue( sign * d );
    minfld_->setValue( sign * m );
    secfld_->setValue( sign * s );
}


uiLatLongInp::uiLatLongInp( uiParent* p )
    : uiGroup(p,"Lat/Long inp group")
{
    const CallBack tscb( mCB(this,uiLatLongInp,typSel) );
    uiButtonGroup* bgrp = new uiButtonGroup( this, "Dec/DMS sel grp",
					     OD::Vertical );
    bgrp->setExclusive( true );
    isdecbut_ = new uiRadioButton( bgrp, uiStrings::sDecimal() );
    isdecbut_->setChecked( true );
    isdecbut_->activated.notify( tscb );
    uiRadioButton* isdmsbut = new uiRadioButton( bgrp, tr("DMS") );
    isdmsbut->activated.notify( tscb );

    uiGroup* lblgrp = new uiGroup( this, "Lat/Long Label grp" );
    uiLabel* latlbl = new uiLabel( lblgrp, uiStrings::sLat() );
    uiLabel* lnglbl = new uiLabel( lblgrp, uiStrings::sLongitude() );
    lnglbl->attach( alignedBelow, latlbl );
    lblgrp->setHAlignObj( latlbl );

    uiGroup* inpgrp = new uiGroup( this, "Lat/Long inp grp" );
    latdecfld_ = new uiLineEdit( inpgrp, DoubleInpSpec(0), "Dec Latitude" );
    lngdecfld_ = new uiLineEdit( inpgrp, DoubleInpSpec(0), "Dec Longitude" );
    lngdecfld_->attach( alignedBelow, latdecfld_ );
    latdmsfld_ = new uiLatLongDMSInp( inpgrp, true );
    lngdmsfld_ = new uiLatLongDMSInp( inpgrp, false );
    lngdmsfld_->attach( alignedBelow, latdmsfld_ );
    inpgrp->setHAlignObj( latdecfld_ );

    lblgrp->attach( rightOf, bgrp );
    inpgrp->attach( rightOf, lblgrp );
    setHAlignObj( inpgrp );
    postFinalise().notify( tscb );
}


void uiLatLongInp::typSel( CallBacker* )
{
    const bool isdec = isdecbut_->isChecked();
    LatLong oth; get( oth, !isdec );
    set( oth, isdec ? 1 : -1 );
    latdecfld_->display( isdec );
    lngdecfld_->display( isdec );
    latdmsfld_->display( !isdec );
    lngdmsfld_->display( !isdec );
}


void uiLatLongInp::get( LatLong& ll ) const
{
    return get( ll, isdecbut_->isChecked() );
}


void uiLatLongInp::get( LatLong& ll, bool isdec ) const
{
    if ( isdec )
    {
	ll.lat_ = latdecfld_->getdValue();
	ll.lng_ = lngdecfld_->getdValue();
    }
    else
    {
	ll.lat_ = latdmsfld_->value();
	ll.lng_ = lngdmsfld_->value();
    }
}


void uiLatLongInp::set( const LatLong& ll )
{
    set( ll, 0 );
}


void uiLatLongInp::set( const LatLong& ll, int opt )
{
    if ( opt >= 0 )
    {
	latdecfld_->setValue( ll.lat_ );
	lngdecfld_->setValue( ll.lng_ );
    }
    if ( opt <= 0 )
    {
	latdmsfld_->set( ll.lat_ );
	lngdmsfld_->set( ll.lng_ );
    }
}


uiLatLong2CoordDlg::uiLatLong2CoordDlg( uiParent* p, const LatLong2Coord& l,
					const SurveyInfo* si )
    : uiDialog(p,uiDialog::Setup(tr("Lat/Long vs Coordinates"),
	     tr("Estimation of geographical coordinates from/to "
	     "the rectangular survey coordinates"),
				 mODHelpKey(mLatLong2CoordDlgHelpID) ))
    , ll2c_(*new LatLong2Coord(l))
    , si_(si?si:&SI())
{
    coordfld_ = new uiGenInput( this, tr("Coordinate in or near survey"),
				DoubleInpSpec(), DoubleInpSpec() );
    uiToolButton* tb = new uiToolButton( this, "xy2ll",
			    tr("Transform file from/to lat long"),
			    mCB(this,uiLatLong2CoordDlg,transfFile) );
    tb->attach( rightTo, coordfld_ );
    tb->attach( rightBorder );

    latlngfld_ = new uiLatLongInp( this );
    latlngfld_->attach( alignedBelow, coordfld_ );
    new uiLabel( this, tr("Corresponds to"), latlngfld_ );

    if ( ll2c_.isOK() )
    {
	coordfld_->setValue( ll2c_.refCoord() );
	latlngfld_->set( ll2c_.refLatLong() );
    }
}


uiLatLong2CoordDlg::~uiLatLong2CoordDlg()
{
    delete &ll2c_;
}


class uiLatLong2CoordFileTransDlg : public uiDialog
{ mODTextTranslationClass(uiLatLong2CoordFileTransDlg)
public:

uiLatLong2CoordFileTransDlg( uiParent* p, ConstRefMan<PositionSystem> coordsys )
    : uiDialog(p,Setup(tr("Transform file"),
		       tr("Transform a file, Lat Long <=> X Y"),
		       mODHelpKey(mLatLong2CoordFileTransDlgHelpID)))
    , coordsys_(coordsys)
{
    uiFileInput::Setup fisu( uiFileDialog::Txt );
    fisu.forread( true ).exameditable( true );
    inpfld_ = new uiFileInput( this, uiStrings::phrInput(uiStrings::sFile()), 
									fisu );

    tollfld_ = new uiGenInput( this, tr("Transform"), BoolInpSpec( true,
			    tr("X Y to Lat Long"), tr("Lat Long to X Y") ) );
    tollfld_->attach( alignedBelow, inpfld_ );

    fisu.forread( false ).withexamine( false );
    outfld_ = new uiFileInput( this, uiStrings::phrOutput(uiStrings::sFile()),
									fisu );
    outfld_->attach( alignedBelow, tollfld_ );
}

bool acceptOK( CallBacker* )
{
    const BufferString inpfnm = inpfld_->fileName();
    if ( inpfnm.isEmpty() ) mErrRet(tr("Please enter the input filename"))
    if ( !File::exists(inpfnm) ) mErrRet(tr("Input file does not exist"))
    const BufferString outfnm = outfld_->fileName();
    if ( outfnm.isEmpty() ) mErrRet(tr("Please enter the input filename"))

    od_istream inpstrm( inpfnm );
    if ( !inpstrm.isOK() ) mErrRet(tr("Empty input file"))
    od_ostream outstrm( outfnm );
    if ( !outstrm.isOK() ) mErrRet(tr("Cannot open output file"))

    const bool toll = tollfld_->getBoolValue();

    double d1, d2;
    Coord coord; LatLong ll;
    while ( inpstrm.isOK() )
    {
	mSetUdf(d1); mSetUdf(d2);
	inpstrm >> d1 >> d2;
	if ( mIsUdf(d1) || mIsUdf(d2) )
	    continue;

	if ( toll )
	{
	    coord.x = d1; coord.y = d2;
	    if ( !SI().isReasonable(coord) )
		continue;
	    ll = coordsys_->toGeographicWGS84( coord );
	    outstrm << ll.lat_ << od_tab << ll.lng_;
	}
	else
	{
	    ll.lat_ = d1; ll.lng_ = d2;
	    coord = coordsys_->fromGeographicWGS84( ll );
	    if ( !SI().isReasonable(coord) )
		continue;
	    outstrm << coord.x << od_tab << coord.y;
	}
	if ( !outstrm.isOK() )
	    break;
	outstrm << od_endl;
    }

    return true;
}

    uiFileInput*	inpfld_;
    uiGenInput*		tollfld_;
    uiFileInput*	outfld_;
    ConstRefMan<PositionSystem>  coordsys_;

};


void uiLatLong2CoordDlg::transfFile( CallBacker* )
{
    return;
}


bool uiLatLong2CoordDlg::getLL2C()
{
    LatLong ll; latlngfld_->get( ll );
    const Coord crd = coordfld_->getCoord();
    if ( mIsUdf(ll.lat_) || mIsUdf(ll.lng_) || mIsUdf(crd.x) || mIsUdf(crd.y) )
	mErrRet(tr("Please fill all fields"))
    if (ll.lat_ > 90 || ll.lat_ < -90)
	mErrRet(tr("Latitude must be between -90 and 90"))
    if (ll.lng_ > 180 || ll.lng_ < -180)
	mErrRet(tr("Longitude must be between -180 and 180"))
    if ( !si_->isReasonable(crd) )
    {
	if ( !uiMSG().askContinue(
	    tr("The coordinate seems to be far away from the survey."
	       "\nContinue?")))
	    return false;
    }

    ll2c_.set( ll, crd );
    if ( !ll2c_.isOK() )
    {
	uiMSG().error(tr("Sorry, your Lat/Long definition has a problem"));
	return false;
    }

    return true;
}


bool uiLatLong2CoordDlg::acceptOK( CallBacker* )
{
    if ( !getLL2C() )
	return false;

    si_->getLatlong2Coord() = ll2c_;
    if ( !si_->write() )
    {
	uiMSG().error(tr("Could not write the definitions "
			 "to your '.survey' file"
			 "\nThe definition will work this "
			 "OpendTect session only"));
	return false;
    }

    if ( si_->getDirName() == SI().getDirName() &&
	 si_->getDataDirName() == SI().getDataDirName() )
	SI().getLatlong2Coord() = ll2c_;

    return true;
}


bool uiLatLong2CoordDlg::ensureLatLongDefined( uiParent* p, SurveyInfo* si )
{
    if ( !si ) si = const_cast<SurveyInfo*>( &SI() );
    if ( si->latlong2Coord().isOK() ) return true;

    uiLatLong2CoordDlg dlg( p, si->latlong2Coord(), si );
    return dlg.go();
}



uiPositionSystemSel::uiPositionSystemSel( uiParent* p, bool onlyorthogonal,
					  const PositionSystem* fillfrom )
    : uiGroup( p, "Coordinate system" )
{
    uiStringSet names;
    PositionSystem::getSystemNames( onlyorthogonal, names, coordsystempars_ );

    coordsystemsuis_.allowNull();

    for ( int idx=0; idx<coordsystempars_.size(); idx++ )
    {
	BufferString key;
	if ( !coordsystempars_[idx]->get( PositionSystem::sKeyFactoryName(),
					  key ) )
	{
	    coordsystempars_.removeSingle( idx );
	    names.removeSingle( idx );
	    idx--;
	    continue;
	}

	uiPositionSystem* systemui =
		uiPositionSystem::factory().create( key, this );

	coordsystemsuis_ += systemui;

	if ( !systemui )
	    continue;

	if ( fillfrom && key==systemui->factoryKeyword() )
	    systemui->initFields( fillfrom );

	systemui->display( false );
    }

    if ( names.size() > 1 )
    {
	coordsystemsel_ = new uiGenInput( this, tr("Coordinate system"),
				      StringListInpSpec(names) );
	mAttachCB( coordsystemsel_->valuechanged,
	       uiPositionSystemSel::systemChangedCB);
    }
    else
    {
	coordsystemsel_ = 0;
    }

    if ( coordsystemsel_ )
    {
	const BufferString selname = fillfrom ? fillfrom->factoryKeyword() : "";
	for ( int idx=0; idx<coordsystemsuis_.size(); idx++ )
	{
	    if ( coordsystemsuis_[idx] )
		coordsystemsuis_[idx]->attach( alignedBelow, coordsystemsel_ );

	    if ( selname==coordsystemsuis_[idx]->factoryKeyword() )
	    {
		coordsystemsel_->setValue(idx);
	    }
	}
    }

    systemChangedCB( 0 );
}


uiPositionSystemSel::~uiPositionSystemSel()
{

}

void uiPositionSystemSel::systemChangedCB(CallBacker *)
{
    const int selidx = coordsystemsel_ ? coordsystemsel_->getIntValue() : 0;

    for ( int idx=0; idx<coordsystemsuis_.size(); idx++ )
    {
	if ( coordsystemsuis_[idx] )
	    coordsystemsuis_[idx]->display(idx==selidx);
    }
}


bool uiPositionSystemSel::acceptOK()
{
    outputsystem_ = 0;

    const int selidx = coordsystemsel_ ? coordsystemsel_->getIntValue() : 0;

    if ( coordsystemsuis_[selidx] )
    {
	if ( !coordsystemsuis_[selidx]->acceptOK() )
	    return false;

	outputsystem_ = coordsystemsuis_[selidx]->outputSystem();
    }
    else
    {
	BufferString key;
	coordsystempars_[selidx]->get( sKey::Name(), key );
	outputsystem_ = PositionSystem::factory().create( key );
	if ( !outputsystem_->usePar(*coordsystempars_[selidx]) )
	{
	    outputsystem_ = 0;
	}
    }

    return outputsystem_;
}

uiCoordSystemDlg::uiCoordSystemDlg( uiParent* p,
				    const PositionSystem* coordsys )
    : uiDialog(p,uiDialog::Setup(tr("Coordinate Reference System"),mNoDlgTitle,
				 mODHelpKey(mLatLong2CoordDlgHelpID) ))
{
    coordsysselfld_ = new Coords::uiPositionSystemSel( this, true, coordsys );
    uiToolButton* tb = new uiToolButton( this, "xy2ll",
			    tr("Transform file from/to lat long"),
			    mCB(this,uiCoordSystemDlg,transfFile) );
    tb->attach( rightTo, coordsysselfld_ );
    tb->attach( rightBorder );
}


uiCoordSystemDlg::~uiCoordSystemDlg()
{
}


void uiCoordSystemDlg::transfFile( CallBacker* )
{
    if ( !getCoordSystem() || !getCoordSystem()->geographicTransformOK() )
	return;

    uiLatLong2CoordFileTransDlg dlg( this, getCoordSystem().ptr() );
    dlg.go();
}


RefMan<PositionSystem> uiCoordSystemDlg::getCoordSystem()
{
    if ( !coordsysselfld_->outputSystem() )
	coordsysselfld_->acceptOK();

    return coordsysselfld_->outputSystem();
}


bool uiCoordSystemDlg::acceptOK( CallBacker* )
{
    if ( !getCoordSystem() )
	return false;

    return true;
}


bool uiCoordSystemDlg::ensureLatLongDefined( uiParent* p, SurveyInfo* si )
{
    if ( !si ) si = const_cast<SurveyInfo*>( &SI() );
    if ( si->getCoordSystem() && si->getCoordSystem()->geographicTransformOK() )
	return true;

    uiCoordSystemDlg dlg( p, si->getCoordSystem() );
    if ( !dlg.go() || !dlg.getCoordSystem()
	    || !dlg.getCoordSystem()->geographicTransformOK() )
	return false;

    si->setCoordSystem( dlg.getCoordSystem() );
    return true;
}


uiUnlocatedXYSystem::uiUnlocatedXYSystem( uiParent* p )
    : uiPositionSystem(p)
{
    xyinftfld_ = new uiCheckBox( this, tr("Coordinates are in feet") );
    xyinftfld_->setChecked( false );
}


bool uiUnlocatedXYSystem::initFields( const Coords::PositionSystem* sys )
{
    mDynamicCastGet( const Coords::UnlocatedXY*, from,	sys );
    if ( !from )
	return false;

    xyinftfld_->setChecked( from->isFeet() );
    return true;
}


bool uiUnlocatedXYSystem::acceptOK()
{
    RefMan<UnlocatedXY> res = new UnlocatedXY;
    res->setIsFeet( xyinftfld_->isChecked() );
    outputsystem_ = res;
    return true;
}


uiAnchorBasedXYSystem::uiAnchorBasedXYSystem( uiParent* p )
    : uiPositionSystem(p)
{
    helpkey_ = mODHelpKey(mLatLong2CoordDlgHelpID);

    coordfld_ = new uiGenInput( this, tr("Coordinate in or near survey"),
				DoubleInpSpec(), DoubleInpSpec() );
    latlngfld_ = new uiLatLongInp( this );
    latlngfld_->attach( alignedBelow, coordfld_ );
    new uiLabel( this, tr("Corresponds to"), latlngfld_ );


    xyinftfld_ = new uiCheckBox( this, tr("Coordinates are in feet") );
    xyinftfld_->attach( rightOf, coordfld_ );
    xyinftfld_->setChecked( false );
    setHAlignObj( coordfld_ );
}


bool uiAnchorBasedXYSystem::initFields( const Coords::PositionSystem* sys )
{
    mDynamicCastGet( const Coords::AnchorBasedXY*, from, sys );
    if ( !from || !from->geographicTransformOK() )
	return false;

    coordfld_->setValue( from->refCoord() );
    latlngfld_->set( from->refLatLong() );

    xyinftfld_->setChecked( from->isFeet() );
    return true;
}


bool uiAnchorBasedXYSystem::acceptOK()
{
    LatLong ll;
    latlngfld_->get( ll );
    const Coord crd = coordfld_->getCoord();
    if ( mIsUdf(ll.lat_) || mIsUdf(ll.lng_) ||
	 mIsUdf(crd.x) || mIsUdf(crd.y) )
	mErrRet(tr("Please fill all fields"))
    if (ll.lat_ > 90 || ll.lat_ < -90)
	mErrRet(tr("Latitude must be between -90 and 90"))
    if (ll.lng_ > 180 || ll.lng_ < -180)
	mErrRet(tr("Longitude must be between -180 and 180"))
    if ( !si_->isReasonable(crd) )
    {
	if ( !uiMSG().askContinue(
	    tr("The coordinate seems to be far away from the survey."
	       "\nContinue?")))
	    return false;
    }

    RefMan<AnchorBasedXY> res = new AnchorBasedXY( ll, crd );
    if ( !res->geographicTransformOK() )
    {
	uiMSG().error(tr("Sorry, your Lat/Long definition has a problem"));
	return false;
    }

    res->setIsFeet( xyinftfld_->isChecked() );

    outputsystem_ = res;

    return true;
}
