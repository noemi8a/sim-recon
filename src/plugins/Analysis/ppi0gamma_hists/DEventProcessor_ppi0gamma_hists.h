// $Id$
//
//    File: DEventProcessor_ppi0gamma_hists.h
// Created: Fri May 15 14:19:50 EDT 2015
// Creator: jrsteven (on Linux ifarm1401 2.6.32-431.el6.x86_64 x86_64)
//

#ifndef _DEventProcessor_ppi0gamma_hists_
#define _DEventProcessor_ppi0gamma_hists_

#include <JANA/JEventProcessor.h>
#include <JANA/JApplication.h>

#include <ANALYSIS/DEventWriterROOT.h>
#include <HDDM/DEventWriterREST.h>
#include <ANALYSIS/DHistogramActions.h>

#include "DFactoryGenerator_ppi0gamma_hists.h"

using namespace jana;
using namespace std;

class DEventProcessor_ppi0gamma_hists : public jana::JEventProcessor
{
	public:
		const char* className(void){return "DEventProcessor_ppi0gamma_hists";}

	private:
		jerror_t init(void);						///< Called once at program start.
		jerror_t brun(jana::JEventLoop* locEventLoop, int locRunNumber);	///< Called every time a new run number is detected.
		jerror_t evnt(jana::JEventLoop* locEventLoop, int locEventNumber);	///< Called every event.
		jerror_t erun(void);						///< Called every time run number changes, provided brun has been called.
		jerror_t fini(void);						///< Called after last event of last event source has been processed.

		//For non-custom reaction-independent histograms, it is recommended that you simultaneously run the monitoring_hists plugin instead of defining them here
};

#endif // _DEventProcessor_ppi0gamma_hists_

