#ifndef uiseisfileman_h
#define uiseisfileman_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uiseisfileman.h,v 1.14 2008-01-22 15:04:17 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiobjfileman.h"
class uiToolButton;


class uiSeisFileMan : public uiObjFileMan
{
public:
			uiSeisFileMan(uiParent*);
			~uiSeisFileMan();

protected:

    uiToolButton*	mrgdmpbut;
    uiToolButton*	cpym2dbut;
    uiToolButton*	browsebut;

    void		ownSelChg();

    void		mergeDump2DPush(CallBacker*);
    void		browsePush(CallBacker*);
    void		copyMan2DPush(CallBacker*);
    void		manPS3D(CallBacker*);
    void		manPS2D(CallBacker*);

    void		mkFileInfo();
    double		getFileSize(const char*,int&) const;
};


#endif
