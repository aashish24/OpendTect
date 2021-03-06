/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Dec 2013
-*/


#include "dbkey.h"
#include "bufstringset.h"
#include "compoundkey.h"
#include "surveydisklocation.h"

static BufferString noNameOfFn( const DBKey& )	{ return BufferString(); }
static IOObj* noGetIOObjFn( const DBKey& )	{ return 0; }
static void noDelIOObjFn( IOObj* )		{}

typedef BufferString (*nameOfFn)(const DBKey&);
typedef IOObj* (*getIOObjFn)(const DBKey&);
typedef void (*delIOObjFn)(IOObj*);

static nameOfFn nameoffn_ = noNameOfFn;
static getIOObjFn getioobjfn_ = noGetIOObjFn;
static delIOObjFn delioobjfn_ = noDelIOObjFn;

mGlobal(Basic) void setDBMan_DBKey_Fns(nameOfFn,getIOObjFn,delIOObjFn);
void setDBMan_DBKey_Fns( nameOfFn nfn, getIOObjFn ifn, delIOObjFn dfn )
{
    nameoffn_ = nfn;
    getioobjfn_ = ifn;
    delioobjfn_ = dfn;
}

BufferString nameOf( const DBKey& dbky )
{
    return (*nameoffn_)( dbky );
}

IOObj* getIOObj( const DBKey& dbky )
{
    return (*getioobjfn_)( dbky );
}

void delIOObj( IOObj* ioobj )
{
    (*delioobjfn_)( ioobj );
}


bool isValidGroupedIDString( const char* str )
{
    if ( !str || !*str )
	return false;
    const bool isudf = *str == '-';
    if ( isudf )
	str++;

    bool digitseen = false;
    bool dotseen = false;
    while ( *str )
    {
	if ( iswdigit(*str) )
	    digitseen = true;
	else if ( *str == '|' )
	    return isudf ? digitseen : true;
	else if ( *str != '.' )
	    return false;
	else if ( dotseen )
	    return false;
	else
	    { dotseen = true; digitseen = false; }
	str++;
    }

    return digitseen;
}


void getGroupedIDNumbers( const char* str, od_int64& gnr, od_int64& onr,
			  BufferString* auxpart, BufferString* survpart )
{
    gnr = onr = -1;
    BufferString inpstr( str );
    if ( inpstr.isEmpty() )
	return;

    char* ptrbq = inpstr.find( '`' );
    if ( ptrbq )
    {
	*ptrbq = '\0';
	if ( survpart )
	    survpart->set( ptrbq + 1 );
    }

    char* ptrpipe = inpstr.find( '|' );
    if ( ptrpipe )
    {
	*ptrpipe = '\0';
	if ( auxpart )
	    auxpart->set( ptrpipe + 1 );
    }

    if ( !isValidGroupedIDString( inpstr.str() ) )
	return;

    CompoundKey ck( inpstr );
    const int len = ck.nrKeys();
    for ( int idx=0; idx<2; idx++ )
    {
	if ( idx < len )
	{
	    const BufferString subky( ck.key(idx) );
	    if ( idx == 0 )
		gnr = toInt64( subky.str() );
	    else
		onr = toInt64( subky.str() );
	}
    }
}


DBKey::~DBKey()
{
    delete auxkey_;
    delete survloc_;
}


DBKey& DBKey::operator =( const DBKey& oth )
{
    if ( this != &oth )
    {
	IDWithGroup<int,int>::operator =( oth );
	setAuxKey( oth.auxkey_ ? oth.auxkey_->str() : 0 );
	delete survloc_;
	if ( oth.survloc_ )
	    survloc_ = new SurveyDiskLocation( *oth.survloc_ );
	else
	    survloc_ = 0;
    }
    return *this;
}


bool DBKey::operator ==( const DBKey& oth ) const
{
    if ( !IDWithGroup<int,int>::operator ==(oth) )
	return false;

    const bool iscursuv = isInCurrentSurvey();
    const bool othiscursurv = oth.isInCurrentSurvey();
    if ( iscursuv && othiscursurv )
	{ /*OK*/ }
    else if ( iscursuv || othiscursurv )
	return false;
    else if ( *survloc_ != *oth.survloc_ )
	return false;

    const bool haveauxkey = auxkey_;
    const bool othhasauxkey = oth.auxkey_;
    if ( !haveauxkey && !othhasauxkey )
	return true;
    else if ( !haveauxkey || !othhasauxkey )
	return false;

    return *auxkey_ == *oth.auxkey_;
}


const SurveyDiskLocation& DBKey::surveyDiskLocation() const
{
    static const SurveyDiskLocation emptysdl_( 0, 0 );
    return survloc_ ? *survloc_ : emptysdl_;
}


const SurveyInfo& DBKey::surveyInfo() const
{
    return surveyDiskLocation().surveyInfo();
}


bool DBKey::isUsable() const
{
    IOObj* ioobj = getIOObj( *this );
    if ( !ioobj )
	return false;
    delIOObj( ioobj );
    return true;
}


bool DBKey::isInCurrentSurvey() const
{
    return !survloc_ || survloc_->isCurrentSurvey();
}


BufferString DBKey::toString() const
{
    BufferString ret;

    if ( groupnr_ < 0 )
	ret.set( -1 );
    else
    {
	ret.set( groupnr_ );
	if ( objnr_ > 0 )
	    ret.add( "." ).add( objnr_ );
    }

    if ( auxkey_ )
	ret.add( "|" ).add( *auxkey_ );

    if ( survloc_ )
	ret.add( "`" ).add( survloc_->fullPath() );

    return ret;
}


void DBKey::fromString( const char* str )
{
    od_int64 gnr, onr; BufferString aux, surv;
    getGroupedIDNumbers( str, gnr, onr, &aux, &surv );
    groupnr_ = (GroupNrType)gnr;
    objnr_ = (ObjNrType)onr;
    setAuxKey( aux );

    SurveyDiskLocation sdl;
    sdl.set( surv );
    delete survloc_;
    if ( sdl.isCurrentSurvey() )
	survloc_ = 0;
    else
	survloc_ = new SurveyDiskLocation( sdl );
}


BufferString DBKey::auxKey() const
{
    return auxkey_ ? *auxkey_ : BufferString::empty();
}


void DBKey::setAuxKey( const char* str )
{
    if ( !str || !*str )
	{ delete auxkey_; auxkey_ = 0; }
    else
    {
	if ( !auxkey_ )
	    auxkey_ = new BufferString( str );
	else
	    *auxkey_ = str;
    }
}


void DBKey::setSurveyDiskLocation( const SurveyDiskLocation& sdl )
{
    delete survloc_;
    if ( sdl.isCurrentSurvey() )
	survloc_ = 0;
    else
	survloc_ = new SurveyDiskLocation( sdl );
}


void DBKey::clearSurveyDiskLocation()
{
    delete survloc_;
    survloc_ = 0;
}


DBKey DBKey::getLocal() const
{
    DBKey ret(*this);
    ret.clearSurveyDiskLocation();
    return ret;
}


BufferString DBKey::surveyName() const
{
    return surveyDiskLocation().surveyName();
}


DBKey DBKey::getFromStr( const char* str )
{
    DBKey ret;
    ret.fromString( str );
    return ret;
}


DBKey DBKey::getFromI64( od_int64 i64 )
{
    GroupedID id = GroupedID::getInvalid();
    id.fromInt64( i64 );
    return DBKey( id.groupNr(), id.objNr() );
}


uiString DBKey::toUiString() const
{
    return ::toUiString( toString() );
}


bool DBKeySet::operator ==( const DBKeySet& oth ) const
{
    const size_type sz = size();
    if ( sz != oth.size() )
	return false;

    for ( idx_type idx=0; idx<sz; idx++ )
    {
	if ( *dbkys_.get(idx) != *oth.dbkys_.get(idx) )
	    return false;
    }
    return true;
}


DBKeySet::idx_type DBKeySet::indexOf( const DBKey& dbky ) const
{
    const size_type sz = size();
    for ( idx_type idx=0; idx<sz; idx++ )
    {
	if ( *dbkys_.get(idx) == dbky )
	    return idx;
    }
    return -1;
}


bool DBKeySet::addIfNew( const DBKey& dbky )
{
    if ( isPresent(dbky) )
	return false;

    add( dbky );
    return true;
}


void DBKeySet::append( const DBKeySet& oth, bool allowduplicates )
{
    if ( allowduplicates )
	deepAppend( dbkys_, oth.dbkys_ );
    else
    {
	for ( auto dbky : oth )
	    addIfNew( *dbky );
    }
}


void DBKeySet::insert( idx_type idx, const DBKey& dbky )
{
    dbkys_.insertAt( new DBKey(dbky), idx );
}


DBKeySet& DBKeySet::removeSingle( idx_type idx )
{
    delete dbkys_.removeSingle( idx );
    return *this;
}


DBKeySet& DBKeySet::removeRange( idx_type idx1, idx_type idx2 )
{
    if ( idx1 == idx2 )
	return removeSingle( idx1 );

    if ( idx1 > idx2 )
	std::swap( idx1, idx2 );
    const size_type sz = size();
    if ( idx1 >= sz )
	return *this;
    if ( idx2 >= sz-1 )
	idx2 = sz-1;

    for ( idx_type idx=idx1; idx<=idx2; idx++ )
	delete dbkys_.get( idx );

    dbkys_.removeRange( idx1, idx2 );
    return *this;
}


DBKeySet& DBKeySet::remove( const DBKey& dbky )
{
    const idx_type idx = indexOf( dbky );
    if ( idx >= 0 )
	removeSingle( idx );
    return *this;
}


DBKeySet& DBKeySet::removeUnusable()
{
    for ( int idx=size()-1; idx>=0; idx-- )
	if ( !get(idx).isUsable() )
	    removeSingle( idx );
    return *this;
}


void DBKeySet::addTo( BufferStringSet& bss ) const
{
    for ( int idx=0; idx<size(); idx++ )
	bss.add( get(idx).toString() );
}
