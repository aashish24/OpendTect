#ifndef i_qmenu_H
#define i_qmenu_H

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          26/04/2000
 RCS:           $Id: i_qmenu.h,v 1.4 2003-11-07 12:21:53 bert Exp $
________________________________________________________________________

-*/

#include <qobject.h>
#include <uimenu.h>

//! Helper class for uiMenuItem to relay Qt's 'activated' messages to uiMenuItem.
/*!
    Internal object, to hide Qt's signal/slot mechanism.
    Can only be constructed by uiMenuItem, which should connect 'activated' 
    slot to the corresponging QMenuItem when calling 'insertItem' on a 
    QMenuData object.
*/
class i_MenuMessenger : public QObject 
{

    Q_OBJECT
    friend class                uiMenuItem;

protected:
				i_MenuMessenger( uiMenuItem* receiver )
                                : _receiver( receiver )
				{}

private:

    uiMenuItem*			_receiver;

private slots:

    void activated() 		{ _receiver->activated.trigger( *_receiver ); }

};

#endif
