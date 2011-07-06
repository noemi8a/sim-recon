// $Id$
//
//    File: JEventProcessor_rawevent.h
// Created: Fri Jun 24 12:05:19 EDT 2011
// Creator: wolin (on Linux stan.jlab.org 2.6.18-194.11.1.el5 x86_64)
//

#ifndef _JEventProcessor_rawevent_
#define _JEventProcessor_rawevent_


#include <vector>
#include <map>


#include <JANA/JApplication.h>
#include <JANA/JEventProcessor.h>
#include <JANA/JEventLoop.h>


#include <evioFileChannel.hxx>
#include <evioUtil.hxx>


#include "FCAL/DFCALHit.h"
#include "BCAL/DBCALHit.h"
#include "TOF/DTOFRawHit.h"
#include "CDC/DCDCHit.h"
#include "FDC/DFDCHit.h"
#include "START_COUNTER/DSCHit.h"
#include "TAGGER/DTagger.h"


#include<boost/tuple/tuple.hpp>


using namespace std;
using namespace jana;
using namespace evio;
using namespace boost;


typedef tuple<int,int,int> cscVal;



//----------------------------------------------------------------------------


class JEventProcessor_rawevent : public jana::JEventProcessor {

	public:
		JEventProcessor_rawevent();
		~JEventProcessor_rawevent();
		const char* className(void){return "JEventProcessor_rawevent";}

	private:
		jerror_t init(void);
		jerror_t brun(jana::JEventLoop *eventLoop, int runnumber);
		jerror_t evnt(jana::JEventLoop *eventLoop, int eventnumber);
		jerror_t erun(void);
		jerror_t fini(void);


                // these routines read and fill the translation tables
                void readTranslationTable(void);
                static void startElement(void *userData, const char *xmlname, const char **atts);


                // these routines access the translation tables
                cscVal DTOFRawHitTranslationADC(const DTOFRawHit* hit);
                cscVal DTOFRawHitTranslationTDC(const DTOFRawHit* hit);

                cscVal DBCALHitTranslationADC(const DBCALHit* hit);
                cscVal DBCALHitTranslationTDC(const DBCALHit* hit);

                cscVal DFCALHitTranslationADC(const DFCALHit* hit);

                cscVal DFDCAnodeHitTranslation(const DFDCHit* hit);
                cscVal DFDCCathodeHitTranslation(const DFDCHit* hit);

                cscVal DCDCHitTranslationADC(const DCDCHit* hit);

                cscVal DSCHitTranslationADC(const DSCHit* hit);
                cscVal DSCHitTranslationTDC(const DSCHit* hit);

                cscVal DTaggerTranslationADC(const DTagger* hit);
                cscVal DTaggerTranslationTDC(const DTagger* hit);


                // maps convert from detector spec to (crate,slot,channel)
                // key is detector-dependent encoded string (e.g. "ring:straw" for CDC)
                static map<string,cscVal>  cscMap;
};

#endif // _JEventProcessor_rawevent_


//----------------------------------------------------------------------------