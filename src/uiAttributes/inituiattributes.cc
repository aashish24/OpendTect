/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki
 Date:          December 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: inituiattributes.cc,v 1.5 2010-03-22 10:13:06 cvsbert Exp $";

#include "inituiattributes.h"

#include "uicoherencyattrib.h"
#include "uiconvolveattrib.h"
#include "uideltaresampleattrib.h"
#include "uidipfilterattrib.h"
#include "uienergyattrib.h"
#include "uieventattrib.h"
#include "uifingerprintattrib.h"
#include "uifrequencyattrib.h"
#include "uifreqfilterattrib.h"
#include "uiinstantattrib.h"
#include "uimatchdeltaattrib.h"
#include "uimathattrib.h"
#include "uiprestackattrib.h"
#include "uipositionattrib.h"
#include "uireferenceattrib.h"
#include "uiscalingattrib.h"
#include "uishiftattrib.h"
#include "uisimilarityattrib.h"
#include "uispecdecompattrib.h"
#include "uivolstatsattrib.h"


void uiAttributes::initStdClasses()
{
    uiCoherencyAttrib::initClass();
    uiConvolveAttrib::initClass();
    uiDeltaResampleAttrib::initClass();
    uiDipFilterAttrib::initClass();
    uiEnergyAttrib::initClass();
    uiEventAttrib::initClass();
    uiFingerPrintAttrib::initClass();
    uiFrequencyAttrib::initClass();
    uiFreqFilterAttrib::initClass();
    uiInstantaneousAttrib::initClass();
    uiMatchDeltaAttrib::initClass();
    uiMathAttrib::initClass();
    uiPreStackAttrib::initClass();
    uiPositionAttrib::initClass();
    uiReferenceAttrib::initClass();
    uiScalingAttrib::initClass();
    uiShiftAttrib::initClass();
    uiSimilarityAttrib::initClass();
    uiSpecDecompAttrib::initClass();
    uiVolumeStatisticsAttrib::initClass();
}

