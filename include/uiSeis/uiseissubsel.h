#ifndef uiseis2dsubsel_h
#define uiseis2dsubsel_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          June 2004
 RCS:           $Id: uiseissubsel.h,v 1.4 2004-07-29 21:41:25 bert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "ranges.h"
class IOPar;
class uiGenInput;
class HorSampling;
class BufferStringSet;


class uiSeis2DSubSel : public uiGroup
{ 	
public:

			uiSeis2DSubSel(uiParent*,const BufferStringSet* lnms=0);

    void		setInput(const StepInterval<int>&);
    			//!< Trace number range
    void		setInput(const StepInterval<float>&);
    			//!< Z range
    void		setInput(const HorSampling&);
    			//!< crlrg converted to trace range
    void		setInput(const char* linename);
    void		setInput( const StepInterval<int>& tr,
	    			  const StepInterval<float>& zr )
			{ setInput( tr ); setInput( zr ); }

    virtual void	usePar(const IOPar&);
    virtual bool	fillPar(IOPar&) const;
    bool		getRange(StepInterval<int>&) const;
    bool		getZRange(StepInterval<float>&) const;

    bool		isAll() const;

    int			expectedNrSamples() const;
    int			expectedNrTraces() const;


protected:

    uiGenInput*		selfld;
    uiGenInput*		trcrgfld;
    uiGenInput*		zfld;
    uiGenInput*		lnmsfld;

    virtual void	selChg(CallBacker*);
    void		doFinalise(CallBacker*);

};


#endif
