#ifndef visannot_h
#define visannot_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: visannot.h,v 1.15 2003-11-07 12:21:54 bert Exp $
________________________________________________________________________


-*/


#include "visobject.h"
#include "position.h"
#include "sets.h"

class SoSwitch;
class SoCoordinate3;
class AxisInfo;

namespace visBase
{
class Text;
class SceneObjectGroup;

/*!\brief
    Annotation is a simple wireframe box with text on its
    axis.
*/

class Annotation : public VisualObjectImpl
{
public:
    static Annotation*		create()
				mCreateDataObj(Annotation);

    void			showText( bool yn );
    bool			isTextShown() const;

    void			showScale( bool yn );
    bool			isScaleShown() const;

    void			setCorner( int, float, float, float );
    Coord3			getCorner( int ) const;
    void			setText( int dim, const char * );

    void			fillPar( IOPar&, TypeSet<int>& ) const;
    int				usePar( const IOPar& );

protected:
    				~Annotation();
    void			updateTextPos(int dim);
    void			updateTextPos();
    Text*			getText( int dim, int textnr );

    SoCoordinate3*		coords;

    ObjectSet<SceneObjectGroup>	scales;
    SceneObjectGroup*		texts;
    
    SoSwitch*			textswitch;
    SoSwitch*			scaleswitch;

    static const char*		textprefixstr;
    static const char*		cornerprefixstr;
    static const char*		showtextstr;
    static const char*		showscalestr;
};

};

#endif
