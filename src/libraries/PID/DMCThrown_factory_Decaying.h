// $Id$
//
//    File: DMCThrown_factory_Decaying.h
// Created: Tue Aug  9 14:29:24 EST 2011
// Creator: pmatt (on Linux ifarml6 2.6.18-128.el5 x86_64)
//

#ifndef _DMCThrown_factory_Decaying_
#define _DMCThrown_factory_Decaying_

#include <iostream>
#include <iomanip>

#include <JANA/JFactory.h>

#include <TRACKING/DMCThrown.h>

using namespace std;
using namespace jana;

class DMCThrown_factory_Decaying : public jana::JFactory<DMCThrown>
{
	public:
		DMCThrown_factory_Decaying(){use_factory = 1;}; //prevents JANA from searching the input file for these objects
		~DMCThrown_factory_Decaying(){};
		const char* Tag(void){return "Decaying";}

	private:

		jerror_t init(void);						///< Called once at program start.
		jerror_t brun(jana::JEventLoop *locEventLoop, int runnumber);	///< Called everytime a new run number is detected.
		jerror_t evnt(jana::JEventLoop *locEventLoop, int eventnumber);	///< Called every event.
		jerror_t erun(void);						///< Called everytime run number changes, provided brun has been called.
		jerror_t fini(void);						///< Called after last event of last event source has been processed.
};

#endif // _DMCThrown_factory_Decaying_
