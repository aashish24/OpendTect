#ifndef sighndl_h
#define sighndl_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id: sighndl.h,v 1.6 2003-11-07 12:21:51 bert Exp $
________________________________________________________________________

-*/

#include <callback.h>

/*!\brief asynchronous event handling and notification. */


class SignalHandling : public CallBacker
{
public:

    enum EvType			{
				    ConnClose,
				    ChldStop,
				    ReInit,
				    Stop,
				    Cont,
				    Alarm,
				    Kill	// This process
				};

    static void			startNotify(EvType,const CallBack&);
    static void			stopNotify(EvType,const CallBack&);
    static void			stopProcess(int,bool friendly=true);
    static void			stopRemote( const char*,int, bool friendly=true,
					    const char* rshcomm=0 );

protected:

				SignalHandling();
    static SignalHandling	theinst_;

    CallBackList		conncbs;
    CallBackList		chldcbs;
    CallBackList		reinitcbs;
    CallBackList		stopcbs;
    CallBackList		contcbs;
    CallBackList		alarmcbs;
    CallBackList		killcbs;

    CallBackList&		getCBL(EvType);

#ifndef __msvc__

    static void			handle(int);

    void			doKill(int);
    void			doStop(int);
    void			doCont();
    void			handleConn();
    void			handleChld();
    void			handleAlarm();
    void			handleReInit();
#endif

};


#endif
