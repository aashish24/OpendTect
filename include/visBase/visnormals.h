#ifndef visnormals_h
#define visnormals_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: visnormals.h,v 1.3 2003-11-07 12:21:54 bert Exp $
________________________________________________________________________


-*/


#include "vissceneobj.h"

class CallBacker;
class SoNormal;
class Vector3;

namespace Threads { class Mutex; };

namespace visBase
{

/*!\brief

*/

class Normals : public SceneObject
{
public:
    static Normals*	create()
			mCreateDataObj(Normals);

    void		setNormal( int, const Vector3& );
    int			addNormal( const Vector3& );
    void		removeNormal( int );

    SoNode*		getData();

protected:
    			~Normals();
    int			getFreeIdx();
    			/*!< Object should be locked when calling */

    SoNormal*		normals;

    TypeSet<int>	unusednormals;
    Threads::Mutex&	mutex;
    			
};

};

#endif

