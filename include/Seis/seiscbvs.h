#ifndef seiscbvs_h
#define seiscbvs_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		April 2001
 RCS:		$Id: seiscbvs.h,v 1.17 2003-05-13 15:27:56 bert Exp $
________________________________________________________________________

CBVS-based seimic translator.

-*/

#include <seistrctr.h>
#include <tracedata.h>
#include <cbvsinfo.h>
class CBVSReadMgr;
class CBVSWriteMgr;
class VBrickSpec;
class SeisTrcBuf;


class CBVSSeisTrcTranslator : public SeisTrcTranslator
{			isTranslator(CBVS,SeisTrc)
public:

			CBVSSeisTrcTranslator(const char* nm=0);
			~CBVSSeisTrcTranslator();

    bool		readInfo(SeisTrcInfo&);
    bool		read(SeisTrc&);
    bool		skip(int nrtrcs=1);

    bool		supportsGoTo() const		{ return true; }
    bool		goTo(const BinID&);

    const UserIDSet*	parSpec(Conn::State) const
			{ return &datatypeparspec; }
    virtual void	usePar(const IOPar*);

    const CBVSReadMgr*	readMgr() const			{ return rdmgr; }
    BinID2Coord		getTransform() const;

    virtual bool	implRemove(const IOObj*) const;
    virtual bool	implRename(const IOObj*,const char*) const;
    virtual bool	implSetReadOnly(const IOObj*,bool) const;
    virtual const char*	defExtension() const		{ return "cbvs"; }

    bool		minimalHdrs() const		{ return minimalhdrs; }
    void		setMinimalHdrs( bool yn=true )	{ minimalhdrs = yn; }

protected:

    bool		forread;
    bool		headerdone;
    bool		donext;
    int			nrdone;

    // Following variables are inited by commitSelections_
    TraceDataInterpreter** storinterps;
    int			actualsz;
    StepInterval<int>	samps;
    Interval<int>	cbvssamps;
    bool*		comps;
    bool*		userawdata;
    bool*		samedatachar;
    unsigned char**	blockbufs;
    unsigned char**	stptrs;
    unsigned char**	tdptrs;
    int			preseldatatype;
    VBrickSpec&		brickspec;

    CBVSReadMgr*	rdmgr;
    CBVSWriteMgr*	wrmgr;
    PosAuxInfo		auxinf;
    bool		minimalhdrs;

    virtual void	cleanUp();
    virtual bool	initRead_();
    virtual bool	initWrite_(const SeisTrc&);
    virtual bool	commitSelections_();
    virtual bool	writeTrc_(const SeisTrc&);
    bool		startWrite();
    bool		toNext();
    bool		getFileName(BufferString&);

private:

    static UserIDSet	datatypeparspec;

    void		calcSamps();
    void		destroyVars();

};


#endif
