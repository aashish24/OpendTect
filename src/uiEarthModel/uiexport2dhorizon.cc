/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman Singh
 Date:          June 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiexport2dhorizon.h"

#include "ctxtioobj.h"
#include "emhorizon2d.h"
#include "emmanager.h"
#include "emsurfaceiodata.h"
#include "emsurfacetr.h"
#include "emioobjinfo.h"
#include "executor.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "ioobj.h"
#include "keystrs.h"
#include "ptrman.h"
#include "od_ostream.h"
#include "surfaceinfo.h"
#include "survgeom2d.h"
#include "survinfo.h"

#include "uichecklist.h"
#include "uicombobox.h"
#include "uifileinput.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uistrings.h"
#include "uitaskrunner.h"
#include "od_helpids.h"

#include <stdio.h>


static const char* hdrtyps[] = { "No", "Single line", "Multi line", 0 };


uiExport2DHorizon::uiExport2DHorizon( uiParent* p,
				      const ObjectSet<SurfaceInfo>& hinfos )
    : uiDialog(p,uiDialog::Setup( uiStrings::phrExport( tr("2D Horizon") ),
	       mNoDlgTitle, mODHelpKey(mExportHorizonHelpID) ))
    , hinfos_(hinfos)
{
    setOkText( uiStrings::sExport() );

    uiLabeledComboBox* lcbox = new uiLabeledComboBox( this,
			 uiStrings::phrSelect( uiStrings::sHorizon() ),
			 "Select 2D Horizon" );
    horselfld_ = lcbox->box();
    horselfld_->setHSzPol( uiObject::MedVar );
    horselfld_->selectionChanged.notify( mCB(this,uiExport2DHorizon,horChg) );
    for ( int idx=0; idx<hinfos_.size(); idx++ )
	horselfld_->addItem( mToUiStringTodo(hinfos_[idx]->name) );

    uiListBox::Setup su( OD::ChooseZeroOrMore, tr("Select lines") );
    linenmfld_ = new uiListBox( this, su );
    linenmfld_->attach( alignedBelow, lcbox );

    headerfld_ = new uiGenInput( this, tr("Header"),
				 StringListInpSpec(hdrtyps) );
    headerfld_->attach( alignedBelow, linenmfld_ );

    udffld_ = new uiGenInput( this, tr("Write undefined parts? Undef value"),
			      StringInpSpec(sKey::FloatUdf()) );
    udffld_->setChecked( true );
    udffld_->setWithCheck( true );
    udffld_->attach( alignedBelow, headerfld_ );

    optsfld_ = new uiCheckList( this, uiCheckList::Unrel, OD::Horizontal);
    optsfld_->addItem( tr("Write line name") )
	      .addItem( uiStrings::phrZIn( SI().zIsTime() ? uiStrings::sMsec() :
							  uiStrings::sFeet()) );
    optsfld_->attach( alignedBelow, udffld_ );
    optsfld_->setChecked( 0, true )
	     .setChecked( 1, !SI().zIsTime() && SI().depthsInFeet() );

    outfld_ = new uiFileInput( this, 
		  uiStrings::phrOutput(uiStrings::phrASCII(uiStrings::sFile())),
		  uiFileInput::Setup().forread(false) );
    outfld_->attach( alignedBelow, optsfld_ );

    horChg( 0 );
}


uiExport2DHorizon::~uiExport2DHorizon()
{
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiExport2DHorizon::doExport()
{
    BufferStringSet linenms;
    linenmfld_->getChosen( linenms );
    if ( !linenms.size() )
	mErrRet(tr("Please select at least one line to proceed"))

    const int horidx = horselfld_->currentItem();
    if ( horidx < 0 || horidx > hinfos_.size() )
	mErrRet(tr("Invalid Horizon"))

    MultiID horid = hinfos_[horidx]->multiid;
    EM::EMManager& em = EM::EMM();
    EM::EMObject* obj = em.getObject( em.getObjectID(horid) );
    if ( !obj )
    {
	PtrMan<Executor> exec = em.objectLoader( horid );
	if ( !exec || !exec->execute() )
	    mErrRet(uiStrings::sCantReadHor())

	obj = em.getObject( em.getObjectID(horid) );
	if ( !obj ) return false;

	obj->ref();
    }

    mDynamicCastGet(EM::Horizon2D*,hor,obj);
    if ( !hor )
	mErrRet(uiStrings::sCantReadHor())

    EM::SectionID sid = hor->sectionID( 0 );
    const Geometry::Horizon2DLine* geom = hor->geometry().sectionGeometry(sid);
    if ( !geom ) mErrRet(tr("Error Reading Horizon"))

    const bool wrudfs = udffld_->isChecked();
    BufferString undefstr;
    if ( wrudfs )
    {
	undefstr =  udffld_->text();
	if ( undefstr.isEmpty() )
	    undefstr = "-";
    }

    const float zfac = !optsfld_->isChecked(1) ? 1
		     : (SI().zIsTime() ? 1000 : mToFeetFactorF);
    const bool wrlnms = optsfld_->isChecked( 0 );
    BufferString line( 180, false );

    od_ostream strm( outfld_->fileName() );
    if ( !strm.isOK() )
	mErrRet(uiStrings::sCantOpenOutpFile())

    writeHeader( strm );
    for ( int idx=0; idx< linenms.size(); idx++ )
    {
	BufferString linename = linenms.get( idx );
	const int lineidx = hor->geometry().lineIndex( linename );
	StepInterval<int> trcrg = geom->colRange( lineidx );
	mDynamicCastGet(const Survey::Geometry2D*,survgeom2d,
		Survey::GM().getGeometry(hor->geometry().geomID(lineidx)))
	PosInfo::Line2DData l2ddata = survgeom2d->data();
	for ( int trcnr=trcrg.start; trcnr<=trcrg.stop; trcnr+=trcrg.step )
	{
	    Coord3 pos = geom->getKnot( RowCol(lineidx,trcnr) );

	    PosInfo::Line2DPos l2dpos;
	    l2ddata.getPos(trcnr, l2dpos );
	    if ( mIsUdf(pos.x) || mIsUdf(pos.y) )
		continue;
	    const bool zudf = mIsUdf(pos.z);
	    if ( zudf && !wrudfs )
		continue;

	    const bool hasspace = linename.contains( ' ' ) ||
				  linename.contains( '\t' );
	    BufferString controlstr = hasspace ? "\"%15s\"" : "%15s";

	    if ( zudf )
	    {
		if ( !wrlnms )
		    line.set( pos.x ).add( "\t" ).add( pos.y )
				     .add("\t" ).add( l2dpos.spnr_ )
				     .add( "\t" ).add( undefstr );
		else
		{
		    controlstr += "%16.2lf%16.2lf%8d%8d%16s";
		    sprintf( line.getCStr(), controlstr.buf(), linename.buf(),
			     pos.x, pos.y, l2dpos.spnr_, trcnr, undefstr.buf());
		}
	    }
	    else
	    {
		pos.z *= zfac;
		if ( wrlnms )
		{
		    controlstr += "%16.2lf%16.2lf%8d%8d%16.4lf";
		    sprintf( line.getCStr(), controlstr.buf(),
			    linename.buf(), pos.x, pos.y, l2dpos.spnr_,
			    trcnr, pos.z );
		}
		else
		{
		    line.set( pos.x ).add( "\t" ).add( pos.y ).add("\t" )
				     .add( l2dpos.spnr_ ).add( "\t" )
				     .add( pos.z );
		}
	    }

	    strm << line << od_newline;
	    if ( !strm.isOK() )
	    {
		uiString msg = tr( "Error writing to the output file." );
		strm.addErrMsgTo( msg );
		mErrRet(msg)
	    }
	}
    }

    return true;
}


void uiExport2DHorizon::writeHeader( od_ostream& strm )
{
    if ( headerfld_->getIntValue() == 0 )
	return;

    const bool wrtlnm = optsfld_->isChecked( 0 );
    BufferString zstr( "Z", optsfld_->isChecked(1) ? "(ms)" : "(s)" );
    BufferString headerstr;
    if ( headerfld_->getIntValue() == 1 )
    {
	wrtlnm ? headerstr = "\"Line name\"\t\"X\"\t\"Y\"\t\"TrcNr\"\t"
	       : headerstr = " \"X\"\t\"Y\"\t";

	headerstr.add( "\"" ).add( "ShotPointNr" ).add( "\"\t" )
		 .add( "\"" ).add( zstr ).add( "\"" );
    }
    else
    {
	int id = 1;
	BufferString str( wrtlnm ? " LineName" : "" );
	if ( !str.isEmpty() )
	{
	    headerstr.add( id ).add( ":" )
                     .add( str ).add( "\n" ).add( "# " );
	    id++;
	}

	headerstr.add( id ).add( ": " ).add( "X\n" );
	headerstr.add( "# " ).add( ++id ).add( ": " ).add( "Y\n" );
	if ( wrtlnm )
	    headerstr.add( "# " ).add( ++id )
                     .add( ": " ).add( "TraceNr\n" );

	headerstr.add( "# " ).add( ++id ).add( ": " ).add( zstr );
    }

    strm << "# " << headerstr << od_newline;
    strm << "#-------------------" << od_endl;
}


bool uiExport2DHorizon::acceptOK( CallBacker* )
{
    const BufferString outfnm( outfld_->fileName() );
    if ( outfnm.isEmpty() )
	mErrRet( uiStrings::sSelOutpFile() );

    if ( File::exists(outfnm) &&
	!uiMSG().askOverwrite(uiStrings::sOutputFileExistsOverwrite()) )
	return false;

    const bool res = doExport();
    if (!res)
    {
	uiMSG().error( uiStrings::phrCannotWrite( uiStrings::sHorizon() ) );
	return false;
    }

    uiString msg = tr("2D Horizon successfully exported."
		      "\n\nDo you want to export more horizons?");
    return !uiMSG().askGoOn(msg, uiStrings::sYes(), tr("No, close window"));
}


void uiExport2DHorizon::horChg( CallBacker* cb )
{
    BufferStringSet sellines;
    linenmfld_->getChosen( sellines );
    linenmfld_->setEmpty();
    const int horidx = horselfld_->currentItem();
    if ( horidx < 0 || horidx > hinfos_.size() )
	return;

    MultiID horid = hinfos_[horidx]->multiid;

    PtrMan<IOObj> ioobj = IOM().get( horid );
    if ( !ioobj ) return;

    EM::SurfaceIOData emdata; EM::IOObjInfo oi( *ioobj );
    uiString errmsg;
    if ( !oi.getSurfaceData(emdata,errmsg) )
	return;

    linenmfld_->addItems( emdata.linenames );
    linenmfld_->setChosen( sellines );
    if ( linenmfld_->nrChosen() == 0 )
	linenmfld_->chooseAll();
}
