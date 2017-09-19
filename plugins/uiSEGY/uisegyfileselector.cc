/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nageswara
 Date:		August 2017
________________________________________________________________________

-*/

#include "uisegyfileselector.h"

#include "dirlist.h"
#include "file.h"
#include "filepath.h"

#include "uitextedit.h"
#include "uitoolbutton.h"
#include "uisegyimptype.h"
#include "uisegyreadstarter.h"

uiSEGYFileSelector::uiSEGYFileSelector(uiParent* p, const char* fnm,
				   const char* vntname,
				   const SEGY::ImpType& imptype,
				   const ObjectSet<uiSEGYVintageInfo>& vntinfos)
    : uiDialog(p, uiDialog::Setup(tr("Select SEGY file(s)"),
	       mNoDlgTitle,mNoHelpKey) )
    , filenmsfld_(0)
    , imptype_( imptype )
    , fp_(*new File::Path(fnm))
    , vntinfos_(vntinfos)
{
    uiString titlestr;
    setOkCancelText( tr("Next >>"), tr("<< Back") );
    titlestr = tr("Please select the file(s) belonging to vintage '%1' " )
	      .arg( vntname );
    setTitleText( titlestr );
    BufferStringSet nms;
    getSelectableFiles( fp_.pathOnly(), nms );
    filenmsfld_ = new uiListBox( this, "Select files", OD::ChooseAtLeastOne );
    filenmsfld_->addItems( nms );
    filenmsfld_->setCurrentItem( fp_.fileName() );
    filenmsfld_->setChosen( filenmsfld_->currentItem() );
    uiGroup* toolgrp = new uiGroup( this, "Tools group" );
    toolgrp->attach( centeredRightOf, filenmsfld_ );
    const CallBack scancb( mCB(this,uiSEGYFileSelector,quickScanCB) );
    uiToolButton* scanbut = new uiToolButton( toolgrp, "examine",
					      tr("Quick scan"), scancb );
    scanbut->setToolTip( tr("See the results of Quick Scan") );
    txtfld_ = new uiTextEdit( this, "Information", true );
    txtfld_->setPrefHeightInChar( 8 );
    txtfld_->setPrefWidthInChar( 80 );
    txtfld_->attach( alignedBelow, filenmsfld_ );
    filenmsfld_->selectionChanged.notify(
					mCB(this, uiSEGYFileSelector,selChgCB));
    selChgCB( 0 );
}


void uiSEGYFileSelector::getSelectableFiles(const BufferString& dirpath,
					    BufferStringSet& filenms )
{
    filenms.setEmpty();
    const BufferString msk( "*.", fp_.extension() );
    const DirList filelist( fp_.pathOnly(), File::FilesInDir, msk );
    for ( int idx=0; idx<filelist.size(); idx++ )
    {
	const BufferString nm( filelist.get(idx) );
	bool found = false;
	for ( int vidx=0; vidx<vntinfos_.size(); vidx++ )
	{
	    const uiSEGYVintageInfo* vntinfo = vntinfos_[vidx];
	    const BufferStringSet& selnms = vntinfo->filenms_;
	    if ( dirpath==vntinfo->fp_.pathOnly() )
	    {
		if ( selnms.isPresent(nm) )
		{
		    found = true;
		    break;
		}
	    }
	}

	if ( !found )
	    filenms.add( nm );
    }

}


uiSEGYFileSelector::~uiSEGYFileSelector()
{
    delete &fp_;
}


void uiSEGYFileSelector::selChgCB( CallBacker* )
{
    BufferString info;
    File::Path fp( fp_.fullPath(), filenmsfld_->getText() );
    info.add( "Location: " ).add( fp.fullPath() ).addNewLine();
    od_int64 szkb = File::getKbSize( fp.fullPath() );
    info.add( "Size: " ).add( szkb ).add( " KB" );
    info.addNewLine().add( "Last modified: ")
	.add( File::timeLastModified(fp.fullPath()) );
    txtfld_->setText( info );
}


void uiSEGYFileSelector::quickScanCB( CallBacker* )
{
    uiSEGYReadStarter::Setup su( false, &imptype_ );
    File::Path fp( fp_.pathOnly(), filenmsfld_->getText() );
    const BufferString fnm ( fp.fullPath());
    su.filenm( fnm ).fixedfnm(true).vintagecheckmode(true);
    uiSEGYReadStarter* readstrdlg = new uiSEGYReadStarter( this, su);
    if ( !readstrdlg->go() )
	return;
}


void uiSEGYFileSelector::getSelNames( BufferStringSet& nms )
{
    filenmsfld_->getChosen( nms );
}


bool uiSEGYFileSelector::acceptOK()
{
    if ( !filenmsfld_->nrChosen() )
	return false;

    return true;
}


bool uiSEGYFileSelector::rejectOK()
{
    uiSEGYReadStarter::Setup su( false, &imptype_ );
    BufferString fnm( fp_.fullPath() );
    su.filenm(fnm).fixedfnm(false).vintagecheckmode(true);
    uiSEGYReadStarter* readstrdlg = new uiSEGYReadStarter( this, su);
    return readstrdlg->go();
}