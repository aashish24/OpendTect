#ifndef SoCameraInfoElement_h
#define SoCameraInfoElement_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: SoCameraInfoElement.h,v 1.2 2003-11-07 12:21:54 bert Exp $
________________________________________________________________________


-*/

#include <Inventor/elements/SoInt32Element.h>

/*!\brief

*/

class SoCameraInfoElement : public SoInt32Element
{
    SO_ELEMENT_HEADER(SoCameraInfoElement);

public:
    static void		initClass();
    void		init(SoState*);
    static void		set( SoState*, SoNode*, int32_t );
    static int		get( SoState* );
    static int		getDefault();

private:
    			~SoCameraInfoElement();
};

#endif

