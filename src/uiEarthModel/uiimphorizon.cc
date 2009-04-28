/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiimphorizon.cc,v 1.118 2009-04-28 10:54:18 cvssatyaki Exp $";

#include "uiimphorizon.h"
#include "uiarray2dinterpol.h"
#include "array2dinterpol.h"
#include "uipossubsel.h"

#include "uicombobox.h"
#include "uilistbox.h"
#include "uibutton.h"
#include "uicolor.h"
#include "uitaskrunner.h"
#include "uifileinput.h"
#include "uigeninputdlg.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uiscaler.h"
#include "uiseparator.h"
#include "uistratlvlsel.h"
#include "uitblimpexpdatasel.h"

#include "arrayndimpl.h"
#include "binidvalset.h"
#include "ctxtioobj.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "emsurfaceauxdata.h"
#include "horizonscanner.h"
#include "ioobj.h"
#include "pickset.h"
#include "randcolor.h"
#include "strmdata.h"
#include "strmprov.h"
#include "surfaceinfo.h"
#include "survinfo.h"
#include "tabledef.h"
#include "filegen.h"
#include "emhorizon3d.h"

#include <math.h>

static const char* sZVals = "Z values";



uiImportHorizon::uiImportHorizon( uiParent* p, bool isgeom )
    : uiDialog(p,uiDialog::Setup("Import Horizon",
					   "Specify parameters",
					   "104.0.0"))
    , ctio_(*mMkCtxtIOObj(EMHorizon3D))
    , isgeom_(isgeom)
    , filludffld_(0)
    , arr2dinterpfld_(0)
    , colbut_(0)
    , stratlvlfld_(0)
    , displayfld_(0)
    , fd_(*EM::Horizon3DAscIO::getDesc())
    , scanner_(0)
{
    inpfld_ = new uiFileInput( this, "Input ASCII File", uiFileInput::Setup()
					    .withexamine(true)
					    .forread(true) );
    inpfld_->setDefaultSelectionDir( 
			    IOObjContext::getDataDirName(IOObjContext::Surf) );
    inpfld_->setSelectMode( uiFileDialog::ExistingFiles );
    inpfld_->valuechanged.notify( mCB(this,uiImportHorizon,formatSel) );

    ctio_.ctxt.forread = !isgeom_;
    ctio_.ctxt.maychdir = false;

    attrlistfld_ = new uiLabeledListBox( this, "Select Attribute(s) to import",
	   				 true );
    attrlistfld_->box()->setLines( 4, true );
    attrlistfld_->attach( alignedBelow, inpfld_ );
    attrlistfld_->box()->selectionChanged.notify(
	    			mCB(this,uiImportHorizon,formatSel) );

    addbut_ = new uiPushButton( this, "Add new",
	    			mCB(this,uiImportHorizon,addAttrib), false );
    addbut_->attach( rightTo, attrlistfld_ );

    uiSeparator* sep = new uiSeparator( this, "H sep" );
    sep->attach( stretchedBelow, attrlistfld_ );

    dataselfld_ = new uiTableImpDataSel( this, fd_, "104.0.8" );
    dataselfld_->attach( alignedBelow, attrlistfld_ );
    dataselfld_->attach( ensureBelow, sep );
    dataselfld_->descChanged.notify( mCB(this,uiImportHorizon,descChg) );

    scanbut_ = new uiPushButton( this, "Scan &Input File",
	    			 mCB(this,uiImportHorizon,scanPush), true );
    scanbut_->attach( alignedBelow, dataselfld_);

    sep = new uiSeparator( this, "H sep" );
    sep->attach( stretchedBelow, scanbut_ );

    subselfld_ = new uiPosSubSel( this, uiPosSubSel::Setup(false,false) );
    subselfld_->attach( alignedBelow, attrlistfld_ );
    subselfld_->attach( ensureBelow, sep );
    subselfld_->setSensitive( false );

    outputfld_ = new uiIOObjSel( this, ctio_ );
    outputfld_->setLabelText( isgeom_ ? "Output Horizon" : "Add to Horizon" );

    if ( isgeom_ )
    {
	filludffld_ = new uiGenInput( this, "Fill undefined parts",
				      BoolInpSpec(true) );
	filludffld_->valuechanged.notify(mCB(this,uiImportHorizon,fillUdfSel));
	filludffld_->setValue(false);
	filludffld_->setSensitive( false );
	filludffld_->attach( alignedBelow, subselfld_ );

	arr2dinterpfld_ = new uiArray2DInterpolSel( this, true, true );
	arr2dinterpfld_->attach( alignedBelow, filludffld_ );
	arr2dinterpfld_->setDistanceUnit( SI().xyInFeet() ? "[ft]" : "[m]" );
	
	outputfld_->attach( alignedBelow, arr2dinterpfld_ );

	stratlvlfld_ = new uiStratLevelSel( this );
	stratlvlfld_->attach( alignedBelow, outputfld_ );
	stratlvlfld_->selChange.notify( mCB(this,uiImportHorizon,stratLvlChg) );

	colbut_ = new uiColorInput( this,
		  		   uiColorInput::Setup(getRandStdDrawColor())
	       			   .lbltxt("Base color") );
	colbut_->attach( alignedBelow, stratlvlfld_ );

	displayfld_ = new uiCheckBox( this, "Display after import" );
	displayfld_->attach( alignedBelow, colbut_ );
	
	fillUdfSel(0);
    }
    else
	outputfld_->attach( alignedBelow, subselfld_ );

    finaliseDone.notify( mCB(this,uiImportHorizon,formatSel) );
}


uiImportHorizon::~uiImportHorizon()
{
    delete ctio_.ioobj; delete &ctio_;
}


void uiImportHorizon::descChg( CallBacker* cb )
{
    if ( scanner_ ) delete scanner_;
    scanner_ = 0;
}


void uiImportHorizon::formatSel( CallBacker* cb )
{
    BufferStringSet attrnms;
    attrlistfld_->box()->getSelectedItems( attrnms );
    if ( isgeom_ ) attrnms.insertAt( new BufferString(sZVals), 0 );
    const int nrattrib = attrnms.size();
    EM::Horizon3DAscIO::updateDesc( fd_, attrnms );
    dataselfld_->updateSummary();
    dataselfld_->setSensitive( nrattrib );
    scanbut_->setSensitive( *inpfld_->fileName() && nrattrib );
    inpfld_->fileName();
    if ( !scanner_ ) 
    {
	subselfld_->setSensitive( false );
	if ( filludffld_ )
	    filludffld_->setSensitive( false );
    }
}


void uiImportHorizon::addAttrib( CallBacker* cb )
{
    uiGenInputDlg dlg( this, "Add Attribute", "Name", new StringInpSpec() );
    if ( !dlg.go() ) return;

    const char* attrnm = dlg.text();
    attrlistfld_->box()->addItem( attrnm );
    const int idx = attrlistfld_->box()->size() - 1;
    attrlistfld_->box()->setSelected( idx, true );
}


void uiImportHorizon::scanPush( CallBacker* )
{
    if ( !isgeom_ && !attrlistfld_->box()->nrSelected() )
    { uiMSG().error("Please select at least one attribute"); return; }
    if ( !dataselfld_->commit() ) return;
    if ( !scanner_ && !doScan() ) return;

    if ( isgeom_ ) 
    {
	filludffld_->setSensitive( scanner_->gapsFound(true) ||
	    		   	   scanner_->gapsFound(false) );
	fillUdfSel(0);
    }

    subselfld_->setSensitive( true );

    scanner_->launchBrowser();
}


bool uiImportHorizon::doScan()
{
    BufferStringSet filenms;
    if ( !getFileNames(filenms) ) return false;

    scanner_ = new HorizonScanner( filenms, fd_, isgeom_ );
    uiTaskRunner taskrunner( this );
    taskrunner.execute( *scanner_ );

    CubeSampling cs( true );
    cs.hrg.set( scanner_->inlRg(), scanner_->crlRg() );
    subselfld_->setInput( cs );
    return true;
}


void uiImportHorizon::fillUdfSel( CallBacker* )
{
    if ( arr2dinterpfld_ )
	arr2dinterpfld_->display( filludffld_->getBoolValue() );
}


bool uiImportHorizon::doDisplay() const
{
    return displayfld_ && displayfld_->isChecked();
}


MultiID uiImportHorizon::getSelID() const
{
    MultiID mid = ctio_.ioobj ? ctio_.ioobj->key() : -1;
    return mid;
}


void uiImportHorizon::stratLvlChg( CallBacker* )
{
    if ( !stratlvlfld_ ) return;
    const Color col( stratlvlfld_->getColor() );
    if ( col != Color::NoColor() )
	colbut_->setColor( col );
}
    
#define mErrRet(s) { uiMSG().error(s); return 0; }
#define mErrRetUnRef(s) { horizon->unRef(); mErrRet(s) }
#define mSave() \
    if ( !exec ) \
    { \
	delete exec; \
	horizon->unRef(); \
	return false; \
    } \
    uiTaskRunner taskrunner( this ); \
    rv = taskrunner.execute( *exec ); \
    delete exec; 

bool uiImportHorizon::doImport()
{
    BufferStringSet attrnms;
    attrlistfld_->box()->getSelectedItems( attrnms );
    if ( isgeom_ ) attrnms.insertAt( new BufferString(sZVals), 0 );
    if ( !attrnms.size() ) mErrRet( "No Attributes Selected" );

    EM::Horizon3D* horizon = isgeom_ ? createHor() : loadHor();
    if ( !horizon ) return false;

    if ( !scanner_ && !doScan() ) return false;

    ObjectSet<BinIDValueSet> sections = scanner_->getSections();

    if ( sections.isEmpty() )
    {
	horizon->unRef();
	mErrRet( "Nothing to import" );
    }

    const bool dofill = filludffld_ && filludffld_->getBoolValue();
    if ( dofill )
    {
	if ( !arr2dinterpfld_->acceptOK() )
	    return false;

	fillUdfs( sections );
    }

    HorSampling hs = subselfld_->envelope().hrg;
    ExecutorGroup importer( "Importing horizon" );
    importer.setNrDoneText( "Nr positions done" );
    int startidx = 0;
    if ( isgeom_ )
    {
	importer.add( horizon->importer(sections,hs) );
	attrnms.remove( 0 );
	startidx = 1;
    }

    if ( attrnms.size() )
	importer.add( horizon->auxDataImporter(sections,attrnms,startidx,hs) );

    uiTaskRunner taskrunner( this );
    const bool success = taskrunner.execute( importer );
    deepErase( sections );
    if ( !success )
	mErrRetUnRef("Cannot import horizon")

    bool rv;
    if ( isgeom_ )
    {
	Executor* exec = horizon->saver();
	mSave();
    }
    else
    {
	mDynamicCastGet(ExecutorGroup*,exec,horizon->auxdata.auxDataSaver(-1))
	mSave();
    }

    if ( !doDisplay() )
	horizon->unRef();
    else
	horizon->unRefNoDelete();

    return rv;
}


bool uiImportHorizon::acceptOK( CallBacker* )
{
    if ( !checkInpFlds() ) return false;

    return doImport();
}


bool uiImportHorizon::getFileNames( BufferStringSet& filenames ) const
{
    if ( !*inpfld_->fileName() )
	mErrRet( "Please select input file(s)" )
    
    inpfld_->getFileNames( filenames );
    for ( int idx=0; idx<filenames.size(); idx++ )
    {
	const char* fnm = filenames[idx]->buf();
	if ( !File_exists(fnm) )
	{
	    BufferString errmsg( "Cannot find input file:\n" );
	    errmsg += fnm;
	    deepErase( filenames );
	    mErrRet( errmsg );
	}
    }

    return true;
}


bool uiImportHorizon::checkInpFlds()
{
    BufferStringSet filenames;
    if ( !getFileNames(filenames) ) return false;

    if ( !dataselfld_->commit() )
	mErrRet( "Please define data format" );

    const char* outpnm = outputfld_->getInput();
    if ( !outpnm || !*outpnm )
	mErrRet( "Please select output horizon" )

    if ( !outputfld_->commitInput() )
	return false;

    if ( isgeom_ && ctio_.ioobj->implExists(false)
	   	 && !uiMSG().askOverwrite("Output horizon exists. Overwrite?") )
       return false;

    return true;
}


bool uiImportHorizon::fillUdfs( ObjectSet<BinIDValueSet>& sections )
{
    HorSampling hs = subselfld_->envelope().hrg;
    PtrMan<Array2DInterpol> interpolator = arr2dinterpfld_->getResult();
    if ( !interpolator )
	return false;

    const float inldist = SI().inlDistance();
    const float crldist = SI().crlDistance();
    interpolator->setRowStep( inldist*hs.step.inl );
    interpolator->setColStep( crldist*hs.step.crl);
    uiTaskRunner taskrunner( this );
    Array2DImpl<float> arr( hs.nrInl(), hs.nrCrl() );
    if ( !arr.isOK() )
	return false;

    for ( int idx=0; idx<sections.size(); idx++ )
    {
	arr.setAll( mUdf(float) );
	BinIDValueSet& data = *sections[idx];
	BinID bid;
	for ( int inl=0; inl<hs.nrInl(); inl++ )
	{
	    bid.inl = hs.start.inl + inl*hs.step.inl;
	    for ( int crl=0; crl<hs.nrCrl(); crl++ )
	    {
		bid.crl = hs.start.crl + crl*hs.step.crl;
		BinIDValueSet::Pos pos = data.findFirst( bid );
		if ( pos.j >= 0 )
		{
		    const float* vals = data.getVals( pos );
		    if ( vals )
			arr.set( inl, crl, vals[0] );
		}
	    }
	}

	if ( !interpolator->setArray( arr ) )
	    return false;

	if ( !taskrunner.execute(*interpolator) )
	    return false;

	for ( int inl=0; inl<hs.nrInl(); inl++ )
	{
	    bid.inl = hs.start.inl + inl*hs.step.inl;
	    for ( int crl=0; crl<hs.nrCrl(); crl++ )
	    {
		bid.crl = hs.start.crl + crl*hs.step.crl;
		BinIDValueSet::Pos pos = data.findFirst( bid );
		if ( pos.j >= 0 ) continue;

		TypeSet<float> vals( data.nrVals(), mUdf(float) );
		vals[0] = arr.get( inl, crl );
		data.add( bid, vals.arr() );
	    }
	}
    }

    return true;
}


EM::Horizon3D* uiImportHorizon::createHor() const
{
    const char* horizonnm = outputfld_->getInput();
    EM::EMManager& em = EM::EMM();
    const MultiID mid = getSelID();
    EM::ObjectID objid = em.getObjectID( mid );
    if ( objid < 0 )
	objid = em.createObject( EM::Horizon3D::typeStr(), horizonnm );

    mDynamicCastGet(EM::Horizon3D*,horizon,em.getObject(objid));
    if ( !horizon )
	mErrRet( "Cannot create horizon" );

    horizon->change.disable();
    horizon->setMultiID( mid );
    horizon->setStratLevelID( stratlvlfld_->getID() );
    horizon->setPreferredColor( colbut_->color() );
    horizon->ref();
    return horizon;
}


EM::Horizon3D* uiImportHorizon::loadHor()
{
    EM::EMManager& em = EM::EMM();
    EM::EMObject* emobj = em.createTempObject( EM::Horizon3D::typeStr() );
    emobj->setMultiID( ctio_.ioobj->key() );
    Executor* loader = emobj->loader();
    if ( !loader ) mErrRet( "Cannot load horizon");

    uiTaskRunner taskrunner( this );
    if ( !taskrunner.execute(*loader) )
	return 0;

    mDynamicCastGet(EM::Horizon3D*,horizon,emobj)
    if ( !horizon ) mErrRet( "Error loading horizon");

    horizon->ref();
    delete loader;
    return horizon;
}



