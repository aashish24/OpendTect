#ifndef visshape_h
#define visshape_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
________________________________________________________________________

-*/

#include "visbasemod.h"
#include "visobject.h"
#include "indexedshape.h"
#include "draw.h"

namespace osg { class Geometry; class Geode; class Switch; class PrimitiveSet; }

namespace visBase
{

class NodeState;
class ForegroundLifter;
class VisColorTab;
class Material;
class Coordinates;
class Normals;
class TextureChannels;
class TextureCoords;


#undef mDeclSetGetItem
#define mDeclSetGetItem( ownclass, clssname, variable ) \
protected: \
    clssname*		   gt##clssname() const; \
public: \
    inline clssname*	   get##clssname()	 { return gt##clssname(); } \
    inline const clssname* get##clssname() const { return gt##clssname(); } \
    void		   set##clssname(clssname*)


mExpClass(visBase) Shape : public VisualObject
{
public:

    mDeclSetGetItem( Shape,	Material, material_ );

    void			setMaterialBinding( int );
    static int			cOverallMaterialBinding()	{ return 0; }
    static int			cPerVertexMaterialBinding()	{ return 2; }

    int				getMaterialBinding() const;

    void			renderOneSide(int side);
				/*!< 0 = visible from both sides.
				     1 = visible from positive side
				    -1 = visible from negative side. */

    int				usePar(const IOPar&);
    void			fillPar(IOPar&) const;

    				// Latency, will be removed at next commit
    void                        setTwoSidedLight(bool) { renderOneSide(0); } 

protected:
				Shape();
    virtual			~Shape();
    
    Material*			material_;

    static const char*		sKeyOnOff();
    static const char*		sKeyTexture();
    static const char*		sKeyMaterial();
};


class ShapeNodeCallbackHandler;

mExpClass(visBase) VertexShape : public Shape
{
    friend class ShapeNodeCallbackHandler;

public:
    static VertexShape*	create()
			mCreateDataObj(VertexShape);
    
    void		setPrimitiveType(Geometry::PrimitiveSet::PrimitiveType);
			//!<Should be called before adding statesets

    mDeclSetGetItem( VertexShape, Normals, normals_ );
    mDeclSetGetItem( VertexShape, TextureCoords, texturecoords_ );

    virtual  void	  setCoordinates(Coordinates* coords);
    virtual  Coordinates* getCoordinates() { return coords_; }
    virtual  void	  setLineStyle(const LineStyle&) {};

    void		removeSwitch();
			/*!<Will turn the object permanently on.
			 \note Must be done before giving away the
			 SoNode with getInventorNode() to take
			 effect. */

    virtual void	setDisplayTransformation( const mVisTrans* );
    			/*!<\note The transformation is forwarded to the
			     the coordinates, if you change coordinates, 
			     you will have to setTransformation again.  */
    const mVisTrans*	getDisplayTransformation() const;
    			/*!<\note Direcly relayed to the coordinates */
    
    void		dirtyCoordinates();

    void		addPrimitiveSet(Geometry::PrimitiveSet*);
    void		removePrimitiveSet(const Geometry::PrimitiveSet*);
    void		removeAllPrimitiveSets();
    int			nrPrimitiveSets() const;
    virtual void	touchPrimitiveSet(int)			{}
    Geometry::PrimitiveSet*	getPrimitiveSet(int);
    void		setMaterial( Material* mt );
    void		materialChangeCB( CallBacker*  );
    void		useOsgAutoNormalComputation(bool);

    enum		BindType{ BIND_OFF = 0,BIND_OVERALL, 
				       BIND_PER_PRIMITIVE_SET, 
				       BIND_PER_PRIMITIVE, BIND_PER_VERTEX};
    void		setColorBindType(BindType);
    int			getNormalBindType();
    void		setNormalBindType(BindType);
    void		updatePartialGeometry(Interval<int>);
    void		useVertexBufferRender(bool);
			/*!<\true, osg use vertex buffer to render and ignore
			    displaylist false, osg use display list to render.*/

    void		setTextureChannels(TextureChannels*);
    
protected:
    			VertexShape( Geometry::PrimitiveSet::PrimitiveType,
				     bool creategeode );
    			~VertexShape();
    
    void		setupOsgNode();
    
    virtual void	addPrimitiveSetToScene(osg::PrimitiveSet*);
    virtual void	removePrimitiveSetFromScene(const osg::PrimitiveSet*);

    Normals*		normals_;
    Coordinates*	coords_;
    TextureCoords*	texturecoords_;

    osg::Node*		node_;
    
    osg::Geode*		geode_;
    osg::Geometry*	osggeom_;

    bool		useosgsmoothnormal_;

    BindType		colorbindtype_;
    BindType		normalbindtype_;

    RefMan<TextureChannels>	channels_;
    ShapeNodeCallbackHandler*	osgcallbackhandler_;
    bool			needstextureupdate_;
    
    Geometry::PrimitiveSet::PrimitiveType	primitivetype_;

    Threads::Lock 				lock_;
						/*!<lock protects primitiveset
						and osg color array*/
    ObjectSet<Geometry::PrimitiveSet>		primitivesets_;

};

#undef mDeclSetGetItem
    
    
mExpClass(visBase) IndexedShape : public VertexShape
{
public:
    
    int		nrCoordIndex() const;
    void	setCoordIndex(int pos,int idx);
    void	setCoordIndices(const int* idxs, int sz);
    		/*!<\note idxs are not copied, and caller must ensure
			  they remain in memory. */
    void	setCoordIndices(const int* idxs, int sz, int start);
    		/*!<\note idxs are copied */

    void	removeCoordIndexAfter(int);
    int		getCoordIndex(int) const;

    int		nrTextureCoordIndex() const;
    void	setTextureCoordIndex(int pos,int idx);
    void	setTextureCoordIndices(const int* idxs,int sz);
    		/*!<\note idxs are not copied, and caller must ensure
			  they remain in memory. */
    void	setTextureCoordIndices(const int* idxs, int sz, int start);
    		/*!<\note idxs are copied */
    void	removeTextureCoordIndexAfter(int);
    int		getTextureCoordIndex(int) const;

    int		nrNormalIndex() const;
    void	setNormalIndex(int pos,int idx);
    void	setNormalIndices(const int* idxs,int sz);
    		/*!<\note idxs are not copied, and caller must ensure
			  they remain in memory. */
    void	setNormalIndices(const int* idxs, int sz, int start);
    		/*!<\note idxs are copied */
    void	removeNormalIndexAfter(int);
    int		getNormalIndex(int) const;

    int		nrMaterialIndex() const;
    void	setMaterialIndex(int pos,int idx);
    void	setMaterialIndices(const int* idxs,int sz);
    		/*!<\note idxs are not copied, and caller must ensure
			  they remain in memory. */
    void	setMaterialIndices(const int* idxs, int sz, int start);
    		/*!<\note idxs are copied */
    void	removeMaterialIndexAfter(int);
    int		getMaterialIndex(int) const;

    int		getClosestCoordIndex(const EventInfo&) const;
    void	replaceShape(SoNode*);

protected:
		IndexedShape( Geometry::PrimitiveSet::PrimitiveType );
};
    
    
class PrimitiveSetCreator : public Geometry::PrimitiveSetCreator
{
    Geometry::PrimitiveSet* doCreate( bool, bool );
};
    


}

#endif

