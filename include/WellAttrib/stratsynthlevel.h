#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki/Bruno
 Date:		July 2013
________________________________________________________________________

-*/

#include "wellattribmod.h"
#include "stratlevel.h"
#include "typeset.h"

namespace StratSynth
{


mExpClass(WellAttrib) Level
{
public:

    typedef Strat::Level::ID	ID;
    typedef TypeSet<float>	ZValueSet;

			Level( ID lvlid )
			    : id_(lvlid)	{}

    int			size() const		{ return zvals_.size(); }
    ID			id() const		{ return id_; }
    BufferString	name() const;
    Color		color() const;

    const ID		id_;
    ZValueSet		zvals_; //!< one for each model/synthetic

    static const Level&	undef();
    static Level&	dummy();

};


mExpClass(WellAttrib) LevelSet
{
public:

    typedef Level::ID	ID;

			LevelSet()			{}
			LevelSet( const LevelSet& oth )	{ *this = oth; }
			~LevelSet()			{ setEmpty(); }
    LevelSet&		operator =(const LevelSet&);

    bool		isEmpty() const		{ return lvls_.isEmpty(); }
    void		setEmpty()		{ deepErase( lvls_ ); }

    Level&		add(ID); //!< if ID already present returns that one

    int			size() const			{ return lvls_.size(); }
    int			indexOf(ID) const;
    int			indexOf(const char*) const;
    bool		isPresent( ID id ) const
			{ return indexOf(id) >= 0; }
    bool		isPresent( const char* nm ) const
			{ return indexOf(nm) >= 0; }

    Level&		getByIdx( int idx )		{ return *lvls_[idx]; }
    const Level&	getByIdx( int idx ) const	{ return *lvls_[idx]; }
    Level&		get(ID);
    const Level&	get(ID) const;
    Level&		getByName(const char*);
    const Level&	getByName(const char*) const;

    const ObjectSet<Level>& levels() const		{ return lvls_; }

protected:

    ObjectSet<Level>	lvls_;


};

} // namespace StratSynth
