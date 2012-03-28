/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Helene Huck
 Date:		March 2012
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelllogcalcinpdata.cc,v 1.2 2012-03-28 13:35:07 cvshelene Exp $";


#include "uiwelllogcalcinpdata.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimathexpression.h"
#include "uimsg.h"
#include "uirockphysform.h"
#include "uiwelllogcalc.h"

#include "welllogset.h"
#include "welllog.h"
#include "separstr.h"
#include "mathexpression.h"
#include "unitofmeasure.h"

static const char* specvararr[] = { "MD", "DZ", 0 };
static const BufferStringSet specvars( specvararr );

uiWellLogCalcInpData::uiWellLogCalcInpData( uiWellLogCalc* p, uiGroup* inpgrp,
					    int fieldnr )
    : uiFormInputSel(inpgrp,p->lognms_,fieldnr)
    , wls_(&p->wls_)
    , lognmsettodef_(false)
    , convertedlog_(0)
{
    inpfld_->box()->selectionChanged.notify( mCB(p,uiWellLogCalc,inpSel) );

    udfbox_ = new uiCheckBox( this, "Fill empty sections" );
    udfbox_->attach( rightOf, unfld_ ? unfld_ : inpfld_ );
}


uiWellLogCalcInpData::~uiWellLogCalcInpData()
{
    if ( convertedlog_ ) delete convertedlog_;
}


void uiWellLogCalcInpData::use( MathExpression* expr )
{
    const int nrvars = expr ? expr->nrUniqueVarNames() : 0;
    if ( idx_ >= nrvars )
	{ display( false ); return; }
    const BufferString varnm = expr->uniqueVarName( idx_ );
    if ( specvars.indexOf(varnm.buf()) >= 0 )
	{ display( false ); return; }

    varnm_ = varnm;
    display( true );
    BufferString inplbl = "For '"; inplbl += varnm; inplbl += "' use";
    inpfld_->label()->setText( inplbl.buf() );
    if ( !lognmsettodef_ )
    {
	const int nearidx = posinpnms_.nearestMatch( varnm );
	if ( nearidx >= 0 )
	{
	    inpfld_->box()->setCurrentItem( nearidx );
	    lognmsettodef_ = true;
	}
    }
}


const Well::Log* uiWellLogCalcInpData::getLog()
{
    return wls_->getLog( inpfld_->box()->text() );
}

bool uiWellLogCalcInpData::getInp( uiWellLogCalc::InpData& inpdata )
{
    if ( isCst() )
    {
	inpdata.iscst_ = true;
	inpdata.cstval_ = getCstVal();
	return true;
    }

    inpdata.noudf_ = udfbox_->isChecked();
    inpdata.wl_ = getLog();
    const char* logunitnm = inpdata.wl_->unitMeasLabel();
    const UnitOfMeasure* logun = UoMR().get( logunitnm );
    const UnitOfMeasure* convertun = getUnit();
    if ( !logun || !convertun )
	return inpdata.wl_;		//TODO: would we want to stop?

    if ( logun == convertun )
	return inpdata.wl_;

    if ( !inpdata.wl_ )
	return false;

    convertedlog_ = new Well::Log( *inpdata.wl_ );
    for ( int idx=0; idx<inpdata.wl_->size(); idx++ )
    {
	const float initialval = inpdata.wl_->value( idx );
	const float valinsi = logun->getSIValue( initialval );
	const float convertedval = convertun->getUserValueFromSI( valinsi );
	convertedlog_->valArr()[idx] = convertedval;
    }

    inpdata.wl_ = convertedlog_;
    return true;
}


