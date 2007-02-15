#ifndef visanchor_h
#define visanchor_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kris Tingdahl
 Date:		Feb 2007
 RCS:		$Id: visanchor.h,v 1.1 2007-02-15 23:38:28 cvskris Exp $
________________________________________________________________________

-*/

#include "visdatagroup.h"

class SbString;
class SoWWWAnchor;

namespace visBase
{


class Anchor : public DataObjectGroup
{
public:
    static Anchor*	create()
			mCreateDataObj(Anchor);
    
    void		enable( bool yn );

    Notifier<Anchor>	click;

protected:
    SoGroup*		createGroup();
    SoWWWAnchor*	getAnchor();

    static void		clickCB(const SbString&,void*,SoWWWAnchor*);
    static void		highlightCB(const SbString&,void*,SoWWWAnchor*);
    bool		ishighlighted_;
};
}; //namespace

#endif
