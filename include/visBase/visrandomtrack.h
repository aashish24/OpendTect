#ifndef visrandomtracksection_h
#define visrandomtracksection_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: visrandomtrack.h,v 1.8 2003-02-25 07:18:48 nanne Exp $
________________________________________________________________________


-*/

#include "position.h"
#include "ranges.h"
#include "visobject.h"

class SoRandomTrackLineDragger;
template <class T> class Array2D;
class IOPar;


namespace visBase
{
class TriangleStripSet;
class VisColorTab;
class Material;

/*!\brief

*/

class RandomTrack : public VisualObjectImpl
{
public:
    static RandomTrack*		create()
				mCreateDataObj(RandomTrack);

    void			showDragger( bool yn );
    bool			isDraggerShown() const;

    int				nrKnots() const;
    void			addKnot(const Coord& );
    void			insertKnot( int pos, const Coord& );
    Coord			getKnotPos(int) const;
    Coord			getDraggerKnotPos(int) const;
    void			setKnotPos(int, const Coord& );
    void			removeKnot( int );

    void			setDepthInterval( const Interval<float>& );
    const Interval<float>	getDepthInterval() const;
    const Interval<float>	getDraggerDepthInterval() const;

    void			setXrange( const StepInterval<float>& );
    void			setYrange( const StepInterval<float>& );
    void			setZrange( const StepInterval<float>& );
    				/*!< sets limits for dragging */

    void			setDraggerSize( const Coord3& );
    Coord3			getDraggerSize() const;

    void			setClipRate( float );
    float			clipRate() const;

    void			setAutoScale( bool yn );
    bool			autoScale() const;

    void			setColorTab( VisColorTab& );
    VisColorTab&		getColorTab();

    void			setResolution(int);
    int				getResolution() const;

    void			setMaterial( Material* );
    Material*			getMaterial();


    void			setData( int section, const Array2D<float>& );
    				/*!< section ranges from 0 to nrKnots-2 */

    CNotifier<RandomTrack,int>	knotmovement;
    				/*!< Sends the index of the knot moving */

    static const char*		nrknotsstr;
    static const char*		knotprefix;
    static const char*		textureidstr;
    static const char*		draggersizestr;
    static const char*		depthintvstr;
    
    virtual void		fillPar( IOPar&, TypeSet<int>& ) const;
    virtual int			usePar( const IOPar& );

protected:
    				~RandomTrack();
    void			rebuild();
    void			createDragger();

    static void			motionCB(void*,SoRandomTrackLineDragger*);

    Interval<float>		depthrg;
    TypeSet<Coord>		knots;

    ObjectSet<TriangleStripSet>	sections;
    SoRandomTrackLineDragger*	dragger;
    SoSwitch*			draggerswitch;
};

};

#endif

