#ifndef vismarker_h
#define vismarker_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		July 2002
 RCS:		$Id: vismarker.h,v 1.9 2003-11-07 12:21:54 bert Exp $
________________________________________________________________________


-*/

#include "visobject.h"
#include "position.h"

class SoTranslation;
class SoGroup;
class SoShape;
class SoMarkerScale;

namespace visBase
{
class Transformation;

/*!\brief

Marker is a basic pickmarker with a constant size on screen. 
Size and shape are settable.

*/

class Marker : public VisualObjectImpl
{
public:
    static Marker*	create()
			mCreateDataObj(Marker);

    enum Type		{ Cube, Cone, Cylinder, Sphere, Cross, Star };
    void		setType(Type);
    Type		getType() const		{ return markertype; }
 
    void		setCenterPos(const Coord3&);
    Coord3		centerPos(bool displayspace=false) const;
   
    void		setSize(const float);
    float		getSize() const;

    void		setScale(const Coord3&);

    void		setTransformation( Transformation* );
    Transformation*	getTransformation();
    
    int			usePar(const IOPar&);
    void		fillPar(IOPar&,TypeSet<int>&) const;

protected:
			~Marker();
    Transformation*	transformation;

    SoMarkerScale*	markerscale;
    SoShape*		shape;

    Type		markertype;

    static const char*  centerposstr;
};

};


#endif
