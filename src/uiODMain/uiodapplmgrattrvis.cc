/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiodapplmgrattrvis.cc,v 1.14 2010-07-30 07:07:11 cvsnageswara Exp $";

#include "uiodapplmgraux.h"
#include "uiodapplmgr.h"

#include "attribdescset.h"
#include "coltabmapper.h"
#include "coltabsequence.h"
#include "filepath.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "seis2dline.h"
#include "survinfo.h"
#include "visseis2ddisplay.h"
#include "vissurvobj.h"

#include "uiattribpartserv.h"
#include "uicolortable.h"
#include "uiemattribpartserv.h"
#include "uinlapartserv.h"
#include "uiviscoltabed.h"
#include "uivispartserv.h"
#include "uiwellattribpartserv.h"


void uiODApplMgrAttrVisHandler::survChg( bool before )
{
}


bool uiODApplMgrAttrVisHandler::editNLA( bool is2d )
{
    if ( !am_.nlaserv_ ) return false;

    am_.nlaserv_->set2DEvent( is2d );
    const bool res = am_.nlaserv_->go();
    if ( !res ) am_.attrserv_->setNLAName( am_.nlaserv_->modelName() );
    return res;
}


void uiODApplMgrAttrVisHandler::createHorOutput( int tp, bool is2d )
{
    am_.emattrserv_->setDescSet( am_.attrserv_->curDescSet(is2d) );
    MultiID nlaid; const NLAModel* nlamdl = 0;
    if ( am_.nlaserv_ )
    {
	am_.nlaserv_->set2DEvent( is2d );
	nlaid = am_.nlaserv_->modelId();
	nlamdl = &am_.nlaserv_->getModel();
    }
    am_.emattrserv_->setNLA( nlamdl, nlaid );

    uiEMAttribPartServer::HorOutType type =
	  tp==0 ? uiEMAttribPartServer::OnHor :
	( tp==1 ? uiEMAttribPartServer::AroundHor : 
		  uiEMAttribPartServer::BetweenHors );
    am_.emattrserv_->createHorizonOutput( type );
}


void uiODApplMgrAttrVisHandler::createVol( bool is2d )
{
    MultiID nlaid;
    if ( am_.nlaserv_ )
    {
	am_.nlaserv_->set2DEvent( is2d );
	nlaid = am_.nlaserv_->modelId();
    }
    am_.attrserv_->outputVol( nlaid, is2d );
}


void uiODApplMgrAttrVisHandler::doXPlot()
{
    const Attrib::DescSet* ads = am_.attrserv_->getUserPrefDescSet();
    if ( !ads ) return;

    am_.wellattrserv_->setAttribSet( *ads );
    am_.wellattrserv_->doXPlot();
}


void uiODApplMgrAttrVisHandler::crossPlot()
{
    const Attrib::DescSet* ads = am_.attrserv_->getUserPrefDescSet();
    if ( !ads ) return;

    am_.attrserv_->set2DEvent( ads->is2D() );
    am_.attrserv_->showXPlot(0);
}


void uiODApplMgrAttrVisHandler::setZStretch()
{
    am_.visserv_->setZStretch();
}


bool uiODApplMgrAttrVisHandler::selectAttrib( int id, int attrib )
{
    if ( am_.appl_.isRestoringSession() ) return false;

    if ( id < 0 ) return false;
    const Attrib::SelSpec* as = am_.visserv_->getSelSpec( id, attrib );
    if ( !as ) return false;

    const char* zdomkey =
	am_.visserv_->getZDomainString( am_.visserv_->getSceneID(id) );
    const char* zdomid =
	am_.visserv_->getZDomainID( am_.visserv_->getSceneID(id) );
    BufferString survzdom = SI().getZDomainString();
    const bool samezdom = survzdom == zdomkey;
    Attrib::SelSpec myas( *as );
    const bool selok = am_.attrserv_->selectAttrib( myas, samezdom ? 0 :zdomkey,
	    					    zdomid, myas.is2D() );
    if ( selok )
	am_.visserv_->setSelSpec( id, attrib, myas );
    return selok;
}


void uiODApplMgrAttrVisHandler::setHistogram( int visid, int attrib )
{
    am_.appl_.colTabEd().setHistogram(
	    	am_.visserv_->getHistogram(visid,attrib) );
}


void uiODApplMgrAttrVisHandler::createAndSetMapDataPack( int visid, int attrib,
					   const DataPointSet& data, int colnr )
{
    DataPack::ID cacheid = am_.visserv_->getDataPackID( visid, attrib );
    if ( cacheid == -1 )
	am_.useDefColTab( visid, attrib );
    const int dpid = am_.createMapDataPack( data, colnr );
    am_.visserv_->setDataPackID( visid, attrib, dpid );
    am_.visserv_->setRandomPosData( visid, attrib, &data );
}


void uiODApplMgrAttrVisHandler::pageUpDownPressed( bool pageup )
{
    const int visid = am_.visserv_->getEventObjId();
    const int attrib = am_.visserv_->getSelAttribNr();
    if ( attrib<0 || attrib>=am_.visserv_->getNrAttribs(visid) )
	return;

    int texture = am_.visserv_->selectedTexture( visid, attrib );
    if ( texture<am_.visserv_->nrTextures(visid,attrib)-1 && !pageup )
	texture++;
    else if ( texture && pageup )
	texture--;

    am_.visserv_->selectTexture( visid, attrib, texture );
    updateColorTable( visid, attrib );
}


void uiODApplMgrAttrVisHandler::updateColorTable( int visid, int attrib  )
{
    if ( attrib<0 || attrib>=am_.visserv_->getNrAttribs(visid) )
    {
	am_.appl_.colTabEd().setColTab( 0, false, 0, false );
	return;
    }

    mDynamicCastGet( visSurvey::SurveyObject*, so,
	am_.visserv_->getObject( visid ) );
    if ( so )
	am_.appl_.colTabEd().setColTab( so, attrib, mUdf(int) );
    else
    {
 	am_.appl_.colTabEd().setColTab(
	    am_.visserv_->getColTabSequence( visid, attrib ),
	    true, am_.visserv_->getColTabMapperSetup(visid,attrib),
	    am_.visserv_->canHandleColTabSeqTrans(visid,attrib) );
    }

    setHistogram( visid, attrib );
}


void uiODApplMgrAttrVisHandler::colMapperChg()
{
    mDynamicCastGet(const visBase::DataObject*,dataobj,
		    am_.appl_.colTabEd().getSurvObj())
    const int visid = dataobj ? dataobj->id() : am_.visserv_->getSelObjectId();
    int attrib = dataobj
	? am_.appl_.colTabEd().getChannel() : am_.visserv_->getSelAttribNr();
    if ( attrib == -1 ) attrib = 0;

    am_.visserv_->setColTabMapperSetup( visid, attrib,
	    am_.appl_.colTabEd().getColTabMapperSetup() );
    setHistogram( visid, attrib );

    //Autoscale may have changed ranges, so update.
    mDynamicCastGet( visSurvey::SurveyObject*, so,
	am_.visserv_->getObject( visid ) );
    if ( so )
	am_.appl_.colTabEd().setColTab( so, attrib, mUdf(int) );
    else
    {
 	am_.appl_.colTabEd().setColTab(
	    am_.visserv_->getColTabSequence( visid, attrib ),
	    true, am_.visserv_->getColTabMapperSetup(visid,attrib),
	    am_.visserv_->canHandleColTabSeqTrans(visid,attrib) );
    }
}


void uiODApplMgrAttrVisHandler::colSeqChg()
{
    mDynamicCastGet(const visBase::DataObject*,dataobj,
		    am_.appl_.colTabEd().getSurvObj())
    const int visid = dataobj ? dataobj->id() : am_.visserv_->getSelObjectId();
    int attrib = dataobj
	? am_.appl_.colTabEd().getChannel()
	: am_.visserv_->getSelAttribNr();

    if ( attrib == -1 ) attrib = 0;
    setHistogram( visid, attrib );

    am_.visserv_->setColTabSequence( visid, attrib,
	    am_.appl_.colTabEd().getColTabSequence() );
}


NotifierAccess* uiODApplMgrAttrVisHandler::colorTableSeqChange()
{
    return &am_.appl_.colTabEd().seqChange();
}


// TODO: Remove in v3.5
static bool useOldDefColTab( const IOPar& par, ColTab::MapperSetup& ms,
			     ColTab::Sequence& seq )
{
    BufferString seqnm;
    bool autoscale = false;
    float symmidval = mUdf(float);
    float cliprate = mUdf(float);
    Interval<float> coltabrange;
    if ( !par.get("ColorSeq Name",seqnm) || seqnm.isEmpty() )
	return false;
    seq = ColTab::Sequence( seqnm.buf() );
    par.getYN( "Auto scale", autoscale );
    ms.type_ = autoscale ? ColTab::MapperSetup::Auto
			 : ColTab::MapperSetup::Fixed;
    if ( autoscale )
    {
	if ( par.get("Cliprate",cliprate) )
	    ms.cliprate_ = cliprate;
	if ( par.get("Symmetry Midvalue",symmidval) )
	    ms.symmidval_ = symmidval;
    }
    else if ( par.get("Scale Factor",coltabrange) )
    {
	ms.start_ = coltabrange.start;
	ms.width_ = coltabrange.width();
    }

    return true;
}


#define mGet2DDataFile { \
	mDynamicCastGet( visSurvey::Seis2DDisplay*, s2d, \
			 am_.visserv_->getObject( visid ) ); \
	if ( !s2d ) \
	    return; \
	const char* linenm = s2d->getLineName(); \
	LineKey lk( linenm, as->userRef() ); \
	Seis2DLineSet seis2dlnset( *ioobj ); \
	const int lineidx = seis2dlnset.indexOf( lk ); \
	if ( lineidx < 0 ) \
	    return; \
	const IOPar par2d = seis2dlnset.getInfo( lineidx ); \
	BufferString fnm; \
	par2d.get( "File name", fnm ); \
	fp.setFileName( fnm ); \
    }

 
void uiODApplMgrAttrVisHandler::useDefColTab( int visid, int attrib )
{
    if ( am_.appl_.isRestoringSession() ) return;

    const Attrib::SelSpec* as = am_.visserv_->getSelSpec( visid, attrib );
    if ( !as || as->id().asInt()<0 ) return;

    ColTab::MapperSetup mapper;
    ColTab::Sequence seq( 0 );
    PtrMan<IOObj> ioobj = am_.attrserv_->getIOObj( *as );
    if ( !ioobj )
	return;

    FilePath fp( ioobj->fullUserExpr(true) );
    if ( as->is2D() )
	mGet2DDataFile

    fp.setExtension( "par" );
    IOPar iop;
    if ( iop.read( fp.fullPath(), sKey::Pars) && !iop.isEmpty() )
    {
	if ( !useOldDefColTab(iop,mapper,seq) )
	{
	    const char* ctname = iop.find( sKey::Name );
	    seq = ColTab::Sequence( ctname );
	    mapper.usePar( iop );
	}
    }

    am_.visserv_->setColTabMapperSetup( visid, attrib, mapper );
    am_.visserv_->setColTabSequence( visid, attrib, seq );
    am_.appl_.colTabEd().colTab()->setMapperSetup( &mapper );
    am_.appl_.colTabEd().colTab()->setSequence( &seq, true );
    updateColorTable( visid, attrib );
}


void uiODApplMgrAttrVisHandler::saveDefColTab( int visid, int attrib )
{
    const Attrib::SelSpec* as = am_.visserv_->getSelSpec(visid,attrib);
    PtrMan<IOObj> ioobj = am_.attrserv_->getIOObj( *as );
    if ( !ioobj ) return;

    FilePath fp( ioobj->fullUserExpr(true) );
    IOPar iop;
    if ( as->is2D() )
	mGet2DDataFile

    fp.setExtension( "par" );

    const ColTab::Sequence& ctseq = *am_.visserv_->getColTabSequence(
	    visid, attrib );
    const ColTab::MapperSetup& mapper = *am_.visserv_->getColTabMapperSetup(
	    visid, attrib );
    iop.set( sKey::Name, ctseq.name() );
    mapper.fillPar( iop );
    iop.write( fp.fullPath(), sKey::Pars );
}
