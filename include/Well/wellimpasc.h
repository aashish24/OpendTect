#ifndef wellimpasc_h
#define wellimpasc_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id: wellimpasc.h,v 1.7 2003-11-07 12:21:52 bert Exp $
________________________________________________________________________


-*/

#include "ranges.h"
#include "strmdata.h"
#include "bufstringset.h"
#include <iosfwd>

class MeasureUnit;


namespace Well
{
class Data;

class AscImporter
{
public:

			AscImporter( Data& d ) : wd(d)		{}
			~AscImporter();

    const char*		getTrack(const char*,bool first_is_surface,
	    			 bool depthinfeet=false);
    const char*		getD2T(const char*,bool istvd,bool depthinfeet=false);

    class LasFileInfo
    {
    public:
			LasFileInfo()
			    : zrg(mUndefValue,mUndefValue)
			    , undefval(-999.25)	{}
			~LasFileInfo()		{ deepErase(lognms); }

	BufferStringSet	lognms;
	Interval<float>	zrg;
	float		undefval;

	BufferString	wellnm; //!< only info; not used by getLogs
    };

    const char*		getLogInfo(const char* lasfnm,LasFileInfo&) const;
    const char*		getLogInfo(istream& lasstrm,LasFileInfo&) const;
    const char*		getLogs(const char* lasfnm,const LasFileInfo&,
	    			bool istvd=true);
    const char*		getLogs(istream& lasstrm,const LasFileInfo&,
	    			bool istvd=true);

protected:

    Data&		wd;

    mutable ObjectSet<MeasureUnit>	convs;

    void		parseHeader(char*,char*&,char*&,char*&) const;

};

}; // namespace Well

#endif
