#ifndef ziparchiveinfo_h
#define ziparchiveinfo_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Salil Agarwal
 Date:		27 August 2012
 RCS:		$Id: ziparchiveinfo.h,v 1.3 2012-09-06 03:34:22 cvssalil Exp $
________________________________________________________________________

-*/

#include "objectset.h"
#include "basicmod.h"

class BufferStringSet;
class ZipHandler;

mClass(Basic) ZipArchiveInfo
{
public:

    class FileInfo
    {
    public:
				FileInfo( BufferString& fnm, 
					       unsigned int compsize, 
					       unsigned int uncompsize,
					       unsigned int offset )
					:fnm_(fnm)
					, compsize_(compsize) 
					, uncompsize_(uncompsize)
					, localheaderoffset_(offset)	{}
	BufferString		fnm_;
	unsigned int		compsize_, uncompsize_, localheaderoffset_;
    };

				ZipArchiveInfo( BufferString& fnm );
				~ZipArchiveInfo();

    void			getAllFnms( BufferStringSet& );
    unsigned int		getFCompSize( BufferString& fnm );
    unsigned int		getFCompSize( int );
    unsigned int		getFUnCompSize( BufferString& fnm );
    unsigned int		getFUnCompSize( int );
    unsigned int		getLocalHeaderOffset( BufferString& fnm );
    unsigned int		getLocalHeaderOffset( int );

protected:

    bool			readZipArchive( BufferString& fnm );
    ObjectSet<FileInfo>		files_;
    ZipHandler&			ziphd_;
};



#endif
