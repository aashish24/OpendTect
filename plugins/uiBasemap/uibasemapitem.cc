/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2013
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";


#include "uibasemapitem.h"

#include "uibasemap.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "uiioobjselgrp.h"
#include "uilistbox.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodmain.h"
#include "uistrings.h"
#include "uitreeview.h"

#include "basemapimpl.h"
#include "basemapzvalues.h"
#include "ctxtioobj.h"
#include "filepath.h"
#include "ioman.h"
#include "transl.h"



// uiBasemapTreeTop
uiBasemapTreeTop::uiBasemapTreeTop( uiTreeView* tv )
    : uiTreeTopItem(tv)
{}

uiBasemapTreeTop::~uiBasemapTreeTop()
{}


// uiBasemapGroup
const char* uiBasemapGroup::sKeyNrObjs()	{ return "Nr Objects"; }
const char* uiBasemapGroup::sKeyItem()		{ return "Item"; }

uiBasemapGroup::uiBasemapGroup( uiParent* p )
    : uiGroup(p)
    , namefld_(0)
    , defaultname_("<Name of Basemap Item>")
{
}


uiBasemapGroup::~uiBasemapGroup()
{}


void uiBasemapGroup::addNameField()
{
    namefld_ = new uiGenInput( this, "Name" );
    namefld_->setElemSzPol( uiObject::Wide );
    if ( lastObject() )
	namefld_->attach( alignedBelow, lastObject() );
}


void uiBasemapGroup::setItemName( const char* nm )
{
    if ( namefld_ )
	namefld_->setText( nm );
    else
	defaultname_ = nm;
}


const char* uiBasemapGroup::itemName() const
{ return namefld_ ? namefld_->text() : defaultname_.buf(); }


HelpKey uiBasemapGroup::getHelpKey() const
{ return HelpKey::emptyHelpKey(); }


bool uiBasemapGroup::fillPar( IOPar& par ) const
{
    par.set( sKey::Name(), itemName() );
    return true;
}


bool uiBasemapGroup::usePar( const IOPar& par )
{
    setItemName( par.find(sKey::Name()) );
    return true;
}


bool uiBasemapGroup::acceptOK()
{
    if ( !namefld_ ) return true;

    const FixedString nm = namefld_->text();
    if ( nm.isEmpty() )
    {
	uiMSG().error( "Please enter a name." );
	return false;
    }

    return true;
}



// uiBasemapIOObjGroup
uiBasemapIOObjGroup::uiBasemapIOObjGroup( uiParent* p, const IOObjContext& ctxt,
					  bool isadd)
    : uiBasemapGroup(p)
    , typefld_(0)
{
    ioobjfld_ = new uiIOObjSelGrp( this, ctxt,
				   uiIOObjSelGrp::Setup(OD::ChooseAtLeastOne) );
    ioobjfld_->itemChosen.notify( mCB(this,uiBasemapIOObjGroup,selChg) );
    ioobjfld_->selectionChanged.notify( mCB(this,uiBasemapIOObjGroup,selChg) );

    if ( isadd )
    {
	typefld_ = new uiGenInput( this, "Add items",
	    BoolInpSpec(true,"as group","individually") );
	typefld_->valuechanged.notify( mCB(this,uiBasemapIOObjGroup,typeChg) );
	typefld_->attach( alignedBelow, ioobjfld_ );
    }
}


uiBasemapIOObjGroup::~uiBasemapIOObjGroup()
{
}


uiObject* uiBasemapIOObjGroup::lastObject()
{ return typefld_ ? typefld_->attachObj() : ioobjfld_->attachObj(); }


void uiBasemapIOObjGroup::selChg( CallBacker* )
{
    ioobjfld_->getChosen( mids_ );
    const int nrsel = ioobjfld_->nrChosen();
    if ( nrsel==1 )
	setItemName( IOM().nameOf(ioobjfld_->chosenID()) );
    else
    {
	BufferStringSet strs; ioobjfld_->getChosen( strs );
	const BufferString catstr = strs.cat( "," );
	setItemName( catstr.buf() );
    }
}


void uiBasemapIOObjGroup::typeChg( CallBacker* )
{
    if ( !namefld_ ) return;
    namefld_->setSensitive( typefld_ ? typefld_->getBoolValue() : true );
}


bool uiBasemapIOObjGroup::acceptOK()
{
    const bool res = uiBasemapGroup::acceptOK();
    return res;
}


int uiBasemapIOObjGroup::nrItems() const
{
    const int nrsel = mids_.size();
    const bool addasgroup = !typefld_ || typefld_->getBoolValue() || nrsel==1;
    return addasgroup ? 1 : nrsel;
}


int uiBasemapIOObjGroup::nrObjsPerItem() const
{
    const int nrsel = mids_.size();
    const bool addasgroup = !typefld_ || typefld_->getBoolValue() || nrsel==1;
    return addasgroup ? nrsel : 1;
}


bool uiBasemapIOObjGroup::fillItemPar( int idx, IOPar& par ) const
{
    const int nrobjsperitem = nrObjsPerItem();
    par.set( sKeyNrObjs(), nrobjsperitem );

    if ( nrItems()==1 )
	par.set( sKey::Name(), itemName() );
    else
	par.set( sKey::Name(), IOM().nameOf(mids_[idx]) );

    for ( int objidx=0; objidx<nrobjsperitem; objidx++ )
	par.set( IOPar::compKey(sKey::ID(),objidx), mids_[idx+objidx] );

    return true;
}


bool uiBasemapIOObjGroup::fillPar( IOPar& par ) const
{
    bool res = uiBasemapGroup::fillPar( par );

    const int nritems = nrItems();
    par.set( sKey::NrItems(), nritems );
    for ( int idx=0; idx<nritems; idx++ )
    {
	IOPar ipar;
	fillItemPar( idx, ipar );
	const BufferString key = IOPar::compKey( sKeyItem(), idx );
	par.mergeComp( ipar, key );
    }

    BufferString tmpfnm = FilePath::getTempName( "par" );
    par.write( tmpfnm, 0 );
    return res;
}


bool uiBasemapIOObjGroup::usePar( const IOPar& par )
{
    int nrobjs = 0;
    par.get( sKeyNrObjs(), nrobjs );

    TypeSet<MultiID> mids( nrobjs, MultiID::udf() );
    for ( int idx=0; idx<nrobjs; idx++ )
	par.get( IOPar::compKey(sKey::ID(),idx), mids[idx] );

    ioobjfld_->setChosen( mids );

    bool res = uiBasemapGroup::usePar( par );
    return res;
}


// uiBasemapGroupDlg
class uiBasemapGroupDlg : public uiDialog
{
public:
uiBasemapGroupDlg( uiParent* p, uiBasemapItem& itm, bool isadd )
    : uiDialog(p,Setup("Select Basemap parameters",mNoDlgTitle,mNoHelpKey))
    , grp_(0)
{
    grp_ = itm.createGroup( this, isadd );
    setHelpKey( grp_->getHelpKey() );
}


bool fillPar( IOPar& par ) const
{ return grp_->fillPar( par ); }

bool usePar( const IOPar& par )
{ return grp_->usePar( par ); }


bool acceptOK( CallBacker* )
{ return grp_ ? grp_->acceptOK() : true; }

    uiBasemapGroup* grp_;
};


// uiBasemapItem
mImplFactory( uiBasemapItem, uiBasemapItem::factory )

uiBasemapItem::uiBasemapItem()
{
    mDefineStaticLocalObject( Threads::Atomic<int>, itmid, (100) );
    id_ = itmid++;
}


// uiBasemapTreeItem
uiBasemapTreeItem::uiBasemapTreeItem( const char* nm )
    : uiTreeItem(nm)
    , pars_(*new IOPar)
{
    mDefineStaticLocalObject( Threads::Atomic<int>, treeitmid, (1000) );
    id_ = treeitmid++;
}


uiBasemapTreeItem::~uiBasemapTreeItem()
{
    for ( int idx=0; idx<basemapobjs_.size(); idx++ )
	BMM().getBasemap().removeObject( basemapobjs_[idx] );

    deepErase( basemapobjs_ );
    checkStatusChange()->remove( mCB(this,uiBasemapTreeItem,checkCB) );
    delete &pars_;
}


bool uiBasemapTreeItem::init()
{
    checkStatusChange()->notify( mCB(this,uiBasemapTreeItem,checkCB) );

    const uiBasemapItem* itm = BMM().getBasemapItem( familyid_ );
    const char* iconnm = itm ? itm->iconName() : 0;
    uitreeviewitem_->setIcon( 0, iconnm );
    return true;
}


void uiBasemapTreeItem::addBasemapObject( BaseMapObject& bmo )
{
    const uiBasemapItem* itm = BMM().getBasemapItem( familyid_ );
    if ( !itm ) return;

    bmo.setDepth( Basemap::ZValues().get(itm->factoryKeyword()) );
    basemapobjs_ += &bmo;
    BMM().getBasemap().addObject( &bmo );
}


BaseMapObject* uiBasemapTreeItem::removeBasemapObject( BaseMapObject& bmo )
{
    basemapobjs_ -= &bmo;
    BMM().getBasemap().removeObject( &bmo );
    return &bmo;
}


void uiBasemapTreeItem::checkCB( CallBacker* )
{
    const bool show = isChecked();
    for ( int idx=0; idx<basemapobjs_.size(); idx++ )
	BMM().getBasemap().show( *basemapobjs_[idx], show );
}


bool uiBasemapTreeItem::hasParChanged( const IOPar& oldp, const IOPar& newp,
				       const char* key )
{
    return oldp[key] != newp[key];
}


bool uiBasemapTreeItem::hasSubParChanged( const IOPar& oldp, const IOPar& newp,
					  const char* compkey )
{
    PtrMan<IOPar> oldsubpar = oldp.subselect( compkey );
    PtrMan<IOPar> newsubpar = newp.subselect( compkey );
    if ( !oldsubpar || !newsubpar ) return true;

    return !newsubpar->isEqual( *oldsubpar.ptr(), false );
}


int uiBasemapTreeItem::uiTreeViewItemType() const
{ return uiTreeViewItem::CheckBox; }


// TODO: Implement MenuHandler system
bool uiBasemapTreeItem::showSubMenu()
{ return true; }


bool uiBasemapTreeItem::handleSubMenu( int mnuid )
{
    bool handled = true;
    if ( mnuid == sEditID() )
	edit();
    else if ( mnuid == sRemoveID() )
	remove();
    else
	handled = false;

    return handled;
}


void uiBasemapTreeItem::edit()
{
    BMM().edit( getFamilyID(), ID() );
}


void uiBasemapTreeItem::remove()
{
    parent_->removeChild( this );
}


bool uiBasemapTreeItem::usePar( const IOPar& par )
{
    pars_ = par;
    const uiBasemapItem* itm = BMM().getBasemapItem( familyid_ );
    if ( itm ) pars_.set( sKey::Type(), itm->factoryKeyword() );

    BufferString nm;
    if ( par.get(sKey::Name(),nm) )
    {
	setName( nm );
	updateColumnText( 0 );
    }

    return true;
}



//uiBasemapManager
uiBasemapManager& BMM()
{
    mDefineStaticLocalObject( PtrMan<uiBasemapManager>, bmm,
			      (new uiBasemapManager) );
    return *bmm;
}


uiBasemapManager::uiBasemapManager()
    : basemap_(0)
    , basemapcursor_(0)
    , treetop_(0)
{
    init();
}


uiBasemapManager::~uiBasemapManager()
{}


void uiBasemapManager::init()
{
    const BufferStringSet& nms = uiBasemapItem::factory().getNames();
    for ( int idx=0; idx<nms.size(); idx++ )
    {
	uiBasemapItem* bmitm = uiBasemapItem::factory().create( nms.get(idx) );
	basemapitems_ += bmitm;
	Basemap::ZValues().set( bmitm->factoryKeyword(),
				bmitm->defaultZValue() );
    }
}


void uiBasemapManager::setBasemap( uiBaseMap& bm )
{ basemap_ = &bm; }


uiBaseMap& uiBasemapManager::getBasemap()
{ return *basemap_; }


void uiBasemapManager::setTreeTop( uiTreeTopItem& tt )
{ treetop_ = &tt; }


void uiBasemapManager::addfromPar( const IOPar& treepar )
{
    int nrtreeitems = 0;
    treepar.get( sKey::NrItems(), nrtreeitems );

    for ( int key=0; key<nrtreeitems; key++ )
    {
	PtrMan<IOPar> itmpars = treepar.subselect( key );
	if ( !itmpars ) continue;

	BufferString itmnm;
	itmpars->get( sKey::Name(), itmnm );

	BufferString factkw;
	itmpars->get( sKey::Type(), factkw );

	uiBasemapItem* itm = getBasemapItem( factkw );
	uiBasemapTreeItem* treeitm = itm->createTreeItem( itmnm );

	treeitm->setFamilyID( itm->ID() );
	if ( !treeitm->usePar(*itmpars) )
	{
	    delete treeitm;
	    continue;
	}

	treeitems_.push( treeitm );
	treetop_->addChild( treeitm, true );
    }
    basemap_->resetChangeFlag();
}


void uiBasemapManager::add( int itemid )
{
    uiBasemapItem* itm = getBasemapItem( itemid );
    if ( !itm ) return;

    uiBasemapGroupDlg dlg( basemap_, *itm, true );
    if ( !dlg.go() )
	return;

    IOPar pars;
    dlg.fillPar( pars );

    int nritems = 1;
    pars.get( sKey::NrItems(), nritems );
    for ( int idx=0; idx<nritems; idx++ )
    {
	const BufferString key = IOPar::compKey(uiBasemapGroup::sKeyItem(),idx);
	PtrMan<IOPar> itmpars = pars.subselect( key );
	if ( !itmpars ) continue;

	BufferString itmnm;
	itmpars->get( sKey::Name(), itmnm );
	uiBasemapTreeItem* treeitm = itm->createTreeItem( itmnm );
	treeitm->setFamilyID( itm->ID() );
	if ( !treeitm->usePar(*itmpars) )
	{
	    delete treeitm;
	    continue;
	}

	treeitems_ += treeitm;
	treetop_->addChild( treeitm, true );
    }
}


void uiBasemapManager::edit( int itemid, int treeitemid )
{
    uiBasemapItem* itm = getBasemapItem( itemid );
    if ( !itm ) return;

    uiBasemapTreeItem* treeitm = getBasemapTreeItem( treeitemid );
    if ( !treeitm ) return;

    uiBasemapGroupDlg dlg( basemap_, *itm, false );
    dlg.usePar( treeitm->pars() );
    if ( !dlg.go() )
	return;

    IOPar pars;
    dlg.fillPar( pars );
    const BufferString key = IOPar::compKey(uiBasemapGroup::sKeyItem(),0);
    PtrMan<IOPar> itmpars = pars.subselect( key );
    if ( !itmpars ) return;

    treeitm->usePar( *itmpars );
}


void uiBasemapManager::removeSelectedItems()
{
    ObjectSet<uiBasemapTreeItem> selitms;
    for ( int idx=0; idx<treeitems_.size(); idx++ )
    {
	uiBasemapTreeItem* itm = treeitems_[idx];
	if ( itm->isSelected() )
	    selitms += itm;
    }

    if ( selitms.isEmpty() ) return;

    if ( !uiMSG().askRemove(
	     "All selected items will be removed from the basemap") )
	return;

    while ( !selitms.isEmpty() )
    {
	uiBasemapTreeItem* itm = selitms[0];
	treeitems_ -= itm;
	treetop_->removeChild( itm );
	selitms.removeSingle( 0 );
    }
}


void uiBasemapManager::removeAllItems()
{
    for ( int idx=0; idx<treeitems_.size(); idx++ )
	treetop_->removeChild( treeitems_[idx] );

    treeitems_.erase();
    basemap_->resetChangeFlag();
}


uiBasemapItem* uiBasemapManager::getBasemapItem( const char* factkw )
{
    const FixedString factkeyw = factkw;
    for ( int idx=0; idx<basemapitems_.size(); idx++ )
    {
	if ( factkeyw == basemapitems_[idx]->factoryKeyword() )
	    return basemapitems_[idx];
    }

    return 0;
}


uiBasemapItem* uiBasemapManager::getBasemapItem( int id )
{
    for ( int idx=0; idx<basemapitems_.size(); idx++ )
    {
	if ( basemapitems_[idx]->ID() == id )
	    return basemapitems_[idx];
    }

    return 0;
}


uiBasemapTreeItem* uiBasemapManager::getBasemapTreeItem( int id )
{
    for ( int idx=0; idx<treeitems_.size(); idx++ )
    {
	if ( treeitems_[idx]->ID() == id )
	    return treeitems_[idx];
    }

    return 0;
}


void uiBasemapManager::updateMouseCursor( const Coord3& coord )
{
    const bool defined = coord.isDefined();

    if ( defined && !basemapcursor_ )
    {
	basemapcursor_ = new BaseMapMarkers;
	basemapcursor_->setMarkerStyle(0,
		MarkerStyle2D(MarkerStyle2D::Target,5) );
	basemap_->addObject( basemapcursor_ );
    }

    if ( !basemapcursor_ )
	return;

    Threads::Locker lckr( basemapcursor_->lock_,
			  Threads::Locker::DontWaitForLock );

    if ( lckr.isLocked() )
    {
	if ( !defined )
	    basemapcursor_->positions().erase();
	else if ( basemapcursor_->positions().isEmpty() )
		basemapcursor_->positions() += coord;
	else
	    basemapcursor_->positions()[0] = coord;

	lckr.unlockNow();
	basemapcursor_->updateGeometry();
    }
}
