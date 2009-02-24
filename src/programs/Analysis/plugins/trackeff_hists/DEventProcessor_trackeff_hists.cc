// $Id: DEventProcessor_trackeff_hists.cc 2774 2007-07-19 15:59:02Z davidl $
//
//    File: DEventProcessor_trackeff_hists.cc
// Created: Sun Apr 24 06:45:21 EDT 2005
// Creator: davidl (on Darwin Harriet.local 7.8.0 powerpc)
//

#include <iostream>
#include <cmath>
using namespace std;

#include <TThread.h>

#include "DEventProcessor_trackeff_hists.h"

#include <JANA/JApplication.h>
#include <JANA/JEventLoop.h>

#include <DANA/DApplication.h>
#include <TRACKING/DMCThrown.h>
#include <TRACKING/DTrackCandidate.h>
#include <TRACKING/DTrack.h>
#include <FDC/DFDCGeometry.h>
#include <HDGEOMETRY/DGeometry.h>
#include <DVector2.h>
#include <particleType.h>


// Routine used to create our DEventProcessor
extern "C"{
void InitPlugin(JApplication *app){
	InitJANAPlugin(app);
	app->AddProcessor(new DEventProcessor_trackeff_hists());
}
} // "C"


//------------------
// DEventProcessor_trackeff_hists
//------------------
DEventProcessor_trackeff_hists::DEventProcessor_trackeff_hists()
{
	trk_ptr = &trk;
	cdchit_ptr = &cdchit;
	fdchit_ptr = &fdchit;
	
	pthread_mutex_init(&mutex, NULL);
	pthread_mutex_init(&rt_mutex, NULL);
}

//------------------
// ~DEventProcessor_trackeff_hists
//------------------
DEventProcessor_trackeff_hists::~DEventProcessor_trackeff_hists()
{
	delete ref;
}

//------------------
// init
//------------------
jerror_t DEventProcessor_trackeff_hists::init(void)
{
	// Create TRACKING directory
	TDirectory *dir = new TDirectoryFile("TRACKING","TRACKING");
	dir->cd();

	// Create Trees
	trkeff = new TTree("trkeff","Tracking Efficiency");
	trkeff->Branch("F","track",&trk_ptr);

	cdchits = new TTree("cdchits","CDC hits");
	cdchits->Branch("C","dchit",&cdchit_ptr);

	fdchits = new TTree("fdchits","FDC hits");
	fdchits->Branch("D","dchit",&fdchit_ptr);

	dir->cd("../");
	
	MAX_HIT_DIST_CDC = 1.0; // cm
	MAX_HIT_DIST_FDC = 5.0; // cm

	return NOERROR;
}

//------------------
// brun
//------------------
jerror_t DEventProcessor_trackeff_hists::brun(JEventLoop *loop, int runnumber)
{
	DApplication* dapp = dynamic_cast<DApplication*>(loop->GetJApplication());
	bfield = dapp->GetBfield(); // temporary until new geometry scheme is worked out
	ref = new DReferenceTrajectory(bfield);

	const DGeometry *dgeom  = dapp->GetDGeometry(runnumber);
	dgeom->GetFDCWires(fdcwires);
	
	
	return NOERROR;
}

//------------------
// erun
//------------------
jerror_t DEventProcessor_trackeff_hists::erun(void)
{

	return NOERROR;
}

//------------------
// fini
//------------------
jerror_t DEventProcessor_trackeff_hists::fini(void)
{
	return NOERROR;
}

//------------------
// evnt
//------------------
jerror_t DEventProcessor_trackeff_hists::evnt(JEventLoop *loop, int eventnumber)
{
	CDChitv cdctrackhits;
	FDChitv fdctrackhits;
	vector<const DTrackCandidate*> trackcandidates;
	vector<const DTrack*> tracks;
	vector<const DMCThrown*> mcthrowns;
	
	loop->Get(cdctrackhits);
	loop->Get(fdctrackhits);
	loop->Get(trackcandidates);
	//loop->Get(tracks);
	loop->Get(mcthrowns);
	
	// Lock mutex
	pthread_mutex_lock(&mutex);

	// Fill maps associating CDC and FDC hits with truth points
	FindCDCTrackNumbers(loop);
	FindFDCTrackNumbers(loop);
		
	// Get hit list for all candidates
	vector<CDChitv> cdc_candidate_hits;
	vector<FDChitv> fdc_candidate_hits;
	for(unsigned int i=0; i<trackcandidates.size(); i++){
		CDChitv cdc_outhits;
		GetCDCHits(trackcandidates[i], cdctrackhits, cdc_outhits);
		cdc_candidate_hits.push_back(cdc_outhits);

		FDChitv fdc_outhits;
		GetFDCHits(trackcandidates[i], fdctrackhits, fdc_outhits);
		fdc_candidate_hits.push_back(fdc_outhits);
	}

	// Get hit list for all fit tracks
	vector<CDChitv> cdc_fit_hits;
	vector<FDChitv> fdc_fit_hits;
	for(unsigned int i=0; i<tracks.size(); i++){
		CDChitv cdc_outhits;
		GetCDCHits(tracks[i], cdctrackhits, cdc_outhits);
		cdc_fit_hits.push_back(cdc_outhits);

		FDChitv fdc_outhits;
		GetFDCHits(tracks[i], fdctrackhits, fdc_outhits);
		//GetFDCHitsFromTruth(i+1, fdc_outhits);
		fdc_fit_hits.push_back(fdc_outhits);
	}

	// Get hit list for all throwns
	for(unsigned int i=0; i<mcthrowns.size(); i++){
		const DMCThrown *mcthrown = mcthrowns[i];
		
		// if this isn't a charged track, then skip it
		if(fabs(mcthrowns[i]->charge())==0.0)continue;

		CDChitv cdc_thrownhits;
		FDChitv fdc_thrownhits;
		//GetCDCHits(mcthrowns[i], cdctrackhits, cdc_thrownhits);
		//GetFDCHits(mcthrowns[i], fdctrackhits, fdc_thrownhits);
		GetCDCHitsFromTruth(i+1, cdc_thrownhits);
		GetFDCHitsFromTruth(i+1, fdc_thrownhits);

		trk.status_can = 0;
		trk.status_fit = 0;
		if(cdc_thrownhits.size()<5 && fdc_thrownhits.size()<5){
			trk.status_can--;
			trk.status_fit--;
		}
		
		trk.pthrown = mcthrown->momentum();
		trk.z_thrown = mcthrown->position().Z();
		trk.ncdc_hits_thrown = cdc_thrownhits.size();
		trk.nfdc_hits_thrown = fdc_thrownhits.size();
		trk.ncdc_hits = cdctrackhits.size();
		trk.nfdc_hits = GetNFDCWireHits(fdctrackhits);
		
		//========= CANDIDATE ========
		// Look for a candidate track that matches this thrown one
		CDChitv cdc_matched_hits;
		FDChitv fdc_matched_hits;
		unsigned int icdc_can = FindMatch(cdc_thrownhits, cdc_candidate_hits, cdc_matched_hits);
		unsigned int ifdc_can = FindMatch(fdc_thrownhits, fdc_candidate_hits, fdc_matched_hits);
		
		// Initialize values for when no matching candidate is found
		trk.pcan.SetXYZ(0.0, 0.0, 0.0);
		trk.z_can = -1000.0;
		trk.ncdc_hits_can = 0;
		trk.nfdc_hits_can = 0;
		trk.ncdc_hits_thrown_and_can = 0;
		trk.nfdc_hits_thrown_and_can = 0;
		trk.cdc_chisq_can = 1.0E6;
		trk.fdc_chisq_can = 1.0E6;
		
		// CDC
		const DTrackCandidate *cdc_can = NULL;
		if(icdc_can>=0 && icdc_can<trackcandidates.size()){
			cdc_can = trackcandidates[icdc_can];
			
			trk.ncdc_hits_can = cdc_candidate_hits[icdc_can].size();
			trk.ncdc_hits_thrown_and_can = cdc_matched_hits.size();
			trk.cdc_chisq_can = 0.0;
		}
		
		// FDC
		const DTrackCandidate *fdc_can = NULL;
		if(ifdc_can>=0 && ifdc_can<trackcandidates.size()){
			fdc_can = trackcandidates[ifdc_can];
			
			trk.nfdc_hits_can = fdc_candidate_hits[ifdc_can].size();
			trk.nfdc_hits_thrown_and_can = fdc_matched_hits.size();
			trk.fdc_chisq_can = 0.0;
		}
		
		// Figure out which candidate (if any) I should match this with
		const DTrackCandidate* can = NULL;
		if(cdc_can!=NULL && fdc_can!=NULL){
			can = cdc_matched_hits.size()>fdc_matched_hits.size() ? cdc_can:fdc_can;
		}else{
			can = cdc_can!=NULL ? cdc_can:fdc_can;
		}
		
		if(can!=NULL){
			trk.pcan = can->momentum();
			trk.z_can = can->position().Z();
		}else{
			trk.pcan.SetXYZ(0,0,0);
			trk.z_can = -1000.0;
		}

		// Fill tree
		trkeff->Fill();
	}
	

	// Unlock mutex
	pthread_mutex_unlock(&mutex);
	
	return NOERROR;
}

//------------------
// GetCDCHits
//------------------
void DEventProcessor_trackeff_hists::GetCDCHits(const DKinematicData *p, CDChitv &inhits, CDChitv &outhits)
{
	// In case we run with multiple threads
	pthread_mutex_lock(&rt_mutex);
	
	// Re-swim the reference trajectory using the parameters in p
	ref->Swim(p->position(), p->momentum(), p->charge());
	
	// Loop over hits in the "inhits" vector and find the DOCA of the R.T.
	// for each.
	outhits.clear();
	for(unsigned int i=0; i<inhits.size(); i++){
		double doca = ref->DistToRT(inhits[i]->wire);
		if(doca < MAX_HIT_DIST_CDC)outhits.push_back(inhits[i]);
	}
	
	pthread_mutex_unlock(&rt_mutex);
}

//------------------
// GetFDCHits
//------------------
void DEventProcessor_trackeff_hists::GetFDCHits(const DKinematicData *p, FDChitv &inhits, FDChitv &outhits)
{
	// In case we run with multiple threads
	pthread_mutex_lock(&rt_mutex);
	
	// Re-swim the reference trajectory using the parameters in p
	ref->Swim(p->position(), p->momentum(), p->charge());
	
	// Loop over hits in the "inhits" vector and find the DOCA of the R.T.
	// for each.
	outhits.clear();
	for(unsigned int i=0; i<inhits.size(); i++){
		const DFDCHit* hit = inhits[i];
		if(hit->type != 0)continue; // filter out cathode hits
		const DFDCWire *wire = fdcwires[hit->gLayer-1][hit->element-1];
		if(!wire)continue;
		double doca = ref->DistToRT(wire);
		if(doca < MAX_HIT_DIST_FDC)outhits.push_back(hit);
	}
	
	pthread_mutex_unlock(&rt_mutex);
}

//------------------
// GetCDCHitsFromTruth
//------------------
void DEventProcessor_trackeff_hists::GetCDCHitsFromTruth(int trackno, CDChitv &outhits)
{
	// Loop over all entries in the cdclink map and find the ones
	// corresponding to the given track number
	outhits.clear();
	map<const DCDCTrackHit*, const DMCTrackHit*>::iterator iter;
	for(iter=cdclink.begin(); iter!=cdclink.end(); iter++){
		if((iter->second)->track==trackno)outhits.push_back(iter->first);
	}
}

//------------------
// GetFDCHitsFromTruth
//------------------
void DEventProcessor_trackeff_hists::GetFDCHitsFromTruth(int trackno, FDChitv &outhits)
{
	// Loop over all entries in the fdclink map and find the ones
	// corresponding to the given track number
	outhits.clear();
	map<const DFDCHit*, const DMCTrackHit*>::iterator iter;
	for(iter=fdclink.begin(); iter!=fdclink.end(); iter++){
		if((iter->second)->track==trackno)outhits.push_back(iter->first);
	}
}

//------------------
// GetNFDCWireHits
//------------------
unsigned int DEventProcessor_trackeff_hists::GetNFDCWireHits(FDChitv &inhits)
{
	unsigned int N=0;
	for(unsigned int i=0; i<inhits.size(); i++){
		if(inhits[i]->type==0)N++;
	}
	return N;
}

//------------------
// FindMatch
//------------------
unsigned int DEventProcessor_trackeff_hists::FindMatch(CDChitv &thrownhits, vector<CDChitv> &candidate_hits, CDChitv &matched_hits)
{
	// Loop through all of the candidate hits and look for the one that best
	// matches this thrown's hits. The algorithm simply looks for the candidate
	// that has the most hits in common with the thrown.
	//
	// If the best match shares less than half its hits with the thrown,
	// then it is considered not to match and -1 is returned.

	unsigned int ibest=(unsigned int)-1;
	CDChitv my_matched;
	matched_hits.clear();
	for(unsigned int i=0; i<candidate_hits.size(); i++){
		CDChitv &canhits = candidate_hits[i];
		
		// Loop over thrown hits
		my_matched.clear();
		for(unsigned int j=0; j<thrownhits.size(); j++){
			// Loop over candidate hits
			for(unsigned int k=0; k<canhits.size(); k++){
				if(canhits[k] == thrownhits[j])my_matched.push_back(thrownhits[j]);
			}
		}
		
		// Check if this candidate is a better match
		if(my_matched.size() > matched_hits.size()){
			matched_hits = my_matched;
			ibest = i;
		}
	}
	
	// Is the best good enough?
	if(matched_hits.size() >= (thrownhits.size()/2))return ibest;
	
	return (unsigned int)-1;
}

//------------------
// FindMatch
//------------------
unsigned int DEventProcessor_trackeff_hists::FindMatch(FDChitv &thrownhits, vector<FDChitv> &candidate_hits, FDChitv &matched_hits)
{
	// Loop through all of the candidate hits and look for the one that best
	// matches this thrown's hits. The algorithm simply looks for the candidate
	// that has the most hits in common with the thrown.
	//
	// If the best match shares less than half its hits with the thrown,
	// then it is considered not to match and -1 is returned.

	unsigned int ibest=(unsigned int)-1;
	FDChitv my_matched;
	matched_hits.clear();
	for(unsigned int i=0; i<candidate_hits.size(); i++){
		FDChitv &canhits = candidate_hits[i];
		
		// Loop over thrown hits
		my_matched.clear();
		for(unsigned int j=0; j<thrownhits.size(); j++){
			// Loop over candidate hits
			for(unsigned int k=0; k<canhits.size(); k++){
				if(canhits[k] == thrownhits[j])my_matched.push_back(thrownhits[j]);
			}
		}
		
		// Check if this candidate is a better match
		if(my_matched.size() > matched_hits.size()){
			matched_hits = my_matched;
			ibest = i;
		}
	}
	
	// Is the best good enough?
//_DBG_<<"matched_hits.size()="<<matched_hits.size()<<"  thrownhits.size()="<<thrownhits.size()<<endl;
	if(matched_hits.size() >= (thrownhits.size()/2))return ibest;
	
	return (unsigned int)-1;
}

//------------------
// FindFDCTrackNumbers
//------------------
void DEventProcessor_trackeff_hists::FindFDCTrackNumbers(JEventLoop *loop)
{
	vector<const DFDCHit*> fdchits;
	vector<const DMCTrackHit*> mchits;
	loop->Get(fdchits);
	loop->Get(mchits);

	fdclink.clear();

	// Loop over all FDC wire hits
	for(unsigned int i=0; i<fdchits.size(); i++){
		const DFDCHit *fdchit = fdchits[i];
		if(fdchit->type!=0)continue; // only look for wires
		const DFDCWire *wire 
		  = fdcwires[fdchit->gLayer-1][fdchit->element-1];
		if(!wire)continue;
		
		// Loop over FDC truth points
		for(unsigned int j=0; j<mchits.size(); j++){
			const DMCTrackHit *mchit = mchits[j];
			if(mchit->system != SYS_FDC)continue;
			
			// Check if this hit is "on" this wire
			if(fabs(mchit->z - wire->origin.Z())>0.01)continue;
			DVector2 A(wire->origin.X(), wire->origin.Y());
			DVector2 Adir = A/A.Mod();
			DVector2 udir(wire->udir.X(), wire->udir.Y());
			DVector2 f(mchit->r*cos(mchit->phi), mchit->r*sin(mchit->phi));
			double delta = udir*(f-A);
			double k = udir*Adir;
			double beta = Adir*(f - A - delta*udir)/(1.0 - k*Adir*udir);
			
			// Truth point is somewhere in the cell. We give some tolerance
			// for the exact cell size because the track can go through at
			// an angle such that the truth point is outside the cell. 
			// Not also that the same truth point can be associated with 2
			// FDC hits.
			bool match = (fabs(beta)-1.116/2.0)<0.01;
			if(!match)continue;
			
			// Check if this is already in the map
			map<const DFDCHit*, const DMCTrackHit*>::iterator iter = fdclink.find(fdchit);
			if(iter!=fdclink.end()){
				//_DBG_<<"More than 1 match for FDC hit!"<<endl;
				// If a link for this hit already exists, delete it so neither
				// one is used. This will of course only work for 0,1, or 2
				// hits matched to a given truth point. 3 or more hits will
				// cause the algorithm to fail.
				fdclink.erase(iter);
			}else{
				fdclink[fdchit] = mchit;
			}
		}
	}
}

//------------------
// FindCDCTrackNumbers
//------------------
void DEventProcessor_trackeff_hists::FindCDCTrackNumbers(JEventLoop *loop)
{
	vector<const DCDCTrackHit*> cdchits;
	vector<const DMCTrackHit*> mchits;
	loop->Get(cdchits);
	loop->Get(mchits);

	cdclink.clear();

	// Loop over all CDC wire hits
	for(unsigned int i=0; i<cdchits.size(); i++){
		const DCDCTrackHit *cdchit = cdchits[i];
		const DCDCWire *wire = cdchit->wire;
		if(!wire)continue;
		
		// Loop over CDC truth points
		for(unsigned int j=0; j<mchits.size(); j++){
			const DMCTrackHit *mchit = mchits[j];
			if(mchit->system != SYS_CDC)continue;
			
			// Find the distance between this truth hit and this wire
			DVector3 pos_truth(mchit->r*cos(mchit->phi), mchit->r*sin(mchit->phi), mchit->z);
			DVector3 A = wire->udir.Cross(pos_truth - wire->origin);
			double dist = A.Mag();
			
			// The value of cdchit->dist was calculated using a time
			// that did not have TOF removed. We must estimate the TOF
			// here so so we can correct for that in order to make a
			// more accurate comparison between the truth point
			// and the hit. We estimate TOF by finding the distance
			// that the truth point is from the center of the target
			// and assume beta=1.
			DVector3 target(0,0,65.0);
			DVector3 delta_truth = pos_truth - target;
			double tof = delta_truth.Mag()/3.0E10/1.0E-9;
			double corrected_dist = cdchit->dist*(cdchit->tdrift-tof)/cdchit->tdrift;
//_DBG_<<"x="<<pos_truth.X()<<" y="<<pos_truth.Y()<<" z="<<pos_truth.Z()<<" tof="<<tof<<" tdrift="<<cdchit->tdrift-tof<<" t="<<cdchit->tdrift<<endl;
			// The best we can do is check that the truth hit is inside
			// the straw.
			bool match = fabs(corrected_dist - dist)<0.8;
//_DBG_<<"fabs(corrected_dist - dist)="<<fabs(corrected_dist - dist)<<"  cdchit->dist="<<cdchit->dist<<"  dist="<<dist<<endl;
			if(!match)continue;
			
			// Check if this is already in the map
			map<const DCDCTrackHit*, const DMCTrackHit*>::iterator iter = cdclink.find(cdchit);
			if(iter!=cdclink.end()){
				// If more than one hit occurs in this straw then keep the
				// one closest to corrected_dist
				const DMCTrackHit *mchit_old = cdclink[cdchit];
				DVector3 pos_truth(mchit_old->r*cos(mchit_old->phi), mchit_old->r*sin(mchit_old->phi), mchit_old->z);
				DVector3 A = wire->udir.Cross(pos_truth - wire->origin);
				double dist_old = A.Mag();
				if(fabs(corrected_dist - dist)<fabs(corrected_dist - dist_old)){
					cdclink[cdchit] = mchit;
				}
			}else{
				cdclink[cdchit] = mchit;
			}
		}
	}
}



