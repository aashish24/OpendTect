/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uistereodlg.cc,v 1.4 2003-11-07 12:22:01 bert Exp $
________________________________________________________________________

-*/

#include "uistereodlg.h"
#include "uisoviewer.h"
#include "uislider.h"


uiStereoDlg::uiStereoDlg( uiParent* p, ObjectSet<uiSoViewer>& vwrs_ )
	: uiDialog(p, uiDialog::Setup("Stereo viewing",
		    		      "Set stereo offset","50.0.2")
		      .canceltext(""))
	, vwrs(vwrs_)
{
    sliderfld = new uiSlider( this, "Stereo offset", true );
    sliderfld->valueChanged.notify( mCB(this,uiStereoDlg,sliderMove) );

    finaliseStart.notify( mCB(this,uiStereoDlg,doFinalise) );
}


void uiStereoDlg::doFinalise( CallBacker* )
{
    float offset = vwrs[0]->getStereoOffset();

    sliderfld->setMinValue( 10 );
    sliderfld->setMaxValue( 1000 );
    sliderfld->setTickMarks( false );
    sliderfld->setValue( offset );
}


bool uiStereoDlg::acceptOK( CallBacker* )
{
    sliderfld->processInput();
    float slval = sliderfld->getValue();
    for ( int idx=0; idx<vwrs.size(); idx++ )
	vwrs[idx]->setStereoOffset( slval );

    return true;
}


void uiStereoDlg::sliderMove( CallBacker* )
{
    float slval = sliderfld->getValue();
    for ( int idx=0; idx<vwrs.size(); idx++ )
        vwrs[idx]->setStereoOffset( slval );
}
