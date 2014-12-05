// $Id$
// $HeadURL$
//
//    File: JEventSource_EVIO.h
// Created: Tue Aug  7 15:22:29 EDT 2012
// Creator: davidl (on Darwin harriet.jlab.org 11.4.0 i386)
//

#ifndef _JEventSource_EVIO_
#define _JEventSource_EVIO_


#include <map>
#include <vector>
#include <queue>
#include <deque>
#include <list>
#include <set>
using std::map;
using std::vector;
using std::queue;
using std::set;

#include <JANA/jerror.h>
#include <JANA/JEventSource.h>
#include <JANA/JEvent.h>
#include <JANA/JFactory.h>
#include <JANA/JStreamLog.h>
using namespace jana;

#ifdef HAVE_EVIO
#include <evioChannel.hxx>
#include <evioUtil.hxx>
using namespace evio;
#endif // HAVE_EVIO

#ifdef HAVE_ET
#include <evioETChannel.hxx>
#include <et.h>
#endif // HAVE_ET

#include "daq_param_type.h"
#include "DModuleType.h"
#include "Df250Config.h"
#include "Df250PulseIntegral.h"
#include "Df250StreamingRawData.h"
#include "Df250WindowSum.h"
#include "Df250PulseRawData.h"
#include "Df250TriggerTime.h"
#include "Df250PulseTime.h"
#include "Df250PulsePedestal.h"
#include "Df250WindowRawData.h"
#include "Df125Config.h"
#include "Df125TriggerTime.h"
#include "Df125PulseIntegral.h"
#include "Df125PulseTime.h"
#include "Df125PulsePedestal.h"
#include "Df125PulseRawData.h"
#include "Df125WindowRawData.h"
#include "DF1TDCConfig.h"
#include "DF1TDCHit.h"
#include "DF1TDCTriggerTime.h"
#include "DCAEN1290TDCConfig.h"
#include "DCAEN1290TDCHit.h"
#include "DCODAEventInfo.h"
#include "DCODAROCInfo.h"
#include "DEPICSvalue.h"

//-----------------------------------------------------------------------
/// The JEventSource_EVIO class implements a JEventSource capable of reading in
/// EVIO data from raw data files produced in Hall-D. It can read in entangled
/// (or blocked) events as well as single events. The low-level objects produced
/// reflect the data generated by the front end digitization electronics.
///
/// The type of boards it can understand can be expanded to include additional
/// boards. To do this, files must be edited in a few places:
///
/// In DModuleType.h
/// ------------------
/// 1.) The new module type must be added to the type_id_t enum.
///     Make sure "N_MODULE_TYPES" is the last item in the enum.
///
/// 2.) Add a case for the module type to the switch in the
///     GetModule method.
///
///
/// Create Data Object files
/// -------------------------
///
/// 1.) For each type of data produced by the new module, create
///     a class to represent it. It is highly recommended to use
///     a naming scheme that reflects the naming convention used
///     in the manual. This makes it easier for people trying to
///     understand the low-level data objects in terms of the
///     manual for the module. See Df250WindowSum.h or
///     Df250PulseIntegral.h for examples.
///
///
/// JEventSource_EVIO.h
/// -------------------
/// 1.) Add an appropriate #include near the top of the file for
///     each type of data object created in the previous step.
///
/// 2.) Add an appropriate declaration for a "ParseXXXBank"
///     where the "XXX" is the new module type.
///     example:
///        void ParseF1TDCBank(evioDOMNodeP bankPtr, list<ObjList*> &events);
///
/// 3.) If the routine JFactory_base_CopyTo() still exists at the
///     bottom of this file, then add a line for each data type to it.
///
///
/// JEventSource_EVIO.cc
/// --------------------
/// 1.) In the JEventSource_EVIO::JEventSource_EVIO() constructor,
///     add a line to insert the data type into event_source_data_types
///     for each data type the module produces.
///
/// 2.) In the "ParseEVIOEvent()" method, add a case for the
///     new module type that calls the new "ParseXXXBank()"
///     method. (Note if this is JLab module, then you'll
///     need to add a case to ParseJLabModuleData() ).
///
/// 3.) Add the new ParseXXXBank() method. Preferrably to the
///     end of the file or more importantly, in the order the
///     method appears in the class definition.
///
///
/// JFactoryGenerator_DAQ.h
/// --------------------
/// 1.) Add an include line to the top of the file for each new
///     data type.
///
/// 2.) Add a line for each new data type to the GenerateFactories()
///     method of JFactoryGenerator_DAQ.
///
///----------------------------------------------------------------------

class JEventSource_EVIO: public jana::JEventSource{
	public:

		enum EVIOSourceType{
			kNoSource,
			kFileSource,
			kETSource
		};


		                    JEventSource_EVIO(const char* source_name);
		           virtual ~JEventSource_EVIO();
		virtual const char* className(void){return static_className();}
		 static const char* static_className(void){return "JEventSource_EVIO";}

		          jerror_t GetEvent(jana::JEvent &event);
		              void FreeEvent(jana::JEvent &event);
				    jerror_t GetObjects(jana::JEvent &event, jana::JFactory_base *factory);

                    bool quit_on_next_ET_timeout;

	
#ifdef HAVE_EVIO		
                    void ReadOptionalModuleTypeTranslation(void);
		  virtual jerror_t ReadEVIOEvent(uint32_t* &buf);
             inline void GetEVIOBuffer(jana::JEvent &jevent, uint32_t* &buff, uint32_t &size) const;
     inline evioDOMTree* GetEVIODOMTree(jana::JEvent &jevent) const;
          EVIOSourceType GetEVIOSourceType(void){ return source_type; }
		            void AddROCIDtoParseList(uint32_t rocid){ rocids_to_parse.insert(rocid); }
		   set<uint32_t> GetROCIDParseList(uint32_t rocid){ return rocids_to_parse; }

	protected:
	
		void ConnectToET(const char* source_name);
		
		int32_t last_run_number;
		int32_t filename_run_number;
		
		evioChannel *chan;
		EVIOSourceType source_type;
		map<tagNum, MODULE_TYPE> module_type;
		map<MODULE_TYPE, MODULE_TYPE> modtype_translate;
		set<uint32_t> rocids_to_parse;

		JStreamLog evioout;

		bool  AUTODETECT_MODULE_TYPES;
		bool  DUMP_MODULE_MAP;
		bool  PARSE_EVIO_EVENTS;
		bool  PARSE_F250;
		bool  PARSE_F125;
		bool  PARSE_F1TDC;
		bool  PARSE_CAEN1290TDC;
		bool  MAKE_DOM_TREE;
		int   ET_STATION_NEVENTS;
		bool  ET_STATION_CREATE_BLOCKING;
		int   ET_DEBUG_WORDS_TO_DUMP;
		bool  LOOP_FOREVER;
		int   VERBOSE;
		float TIMEOUT;
		bool  EMULATE_PULSE_INTEGRAL_MODE;
		uint32_t  EMULATE_SPARSIFICATION_THRESHOLD;
		uint32_t EMULATE_FADC250_TIME_THRESHOLD;
		uint32_t EMULATE_FADC125_TIME_THRESHOLD;
		string MODTYPE_MAP_FILENAME;
		bool ENABLE_DISENTANGLING;
		bool F250_IGNORE_PULSETIME;
		bool F125_IGNORE_PULSETIME;
		uint32_t F250_THRESHOLD;             ///< Threshold to use for firmware emulation
		uint32_t F125_THRESHOLD;
		uint32_t F250_NSA;                   ///< Number of samples to integrate after thershold crossing
		uint32_t F250_NSB;                   ///< Number of samples to integrate before thershold crossing
		uint32_t F250_NSPED;                 ///< Number of samples to integrate for pedestal
		uint32_t F250_EMULATION_THRESHOLD;   ///< Minimum difference between max and min samples to do emulation
		uint32_t F125_EMULATION_THRESHOLD; 
		uint32_t F125_NSPED;                 ///< Number of samples to integrate for pedestal
		uint32_t USER_RUN_NUMBER;            ///< Run number supplied by user

		// Utility class with multiple roles:
		//
		// First is to hold pointers to input EVIO buffer and
		// the evioDOMTree made out of it. When an event is
		// first read in, the buffer pointer is set, but the
		// DOM tree is not made until either GetObjects or
		// FreeEvent are called. In the case of multiple physics
		// events in a single DAQ event, the buffer pointer
		// and DOM tree pointers will be NULL.
		//
		// Second is to hold pointers to containers for
		// all types of data objects we produce. This gets passed
		// into bank processor methods so that they can append
		// to the lists. Note that the naming scheme here needs to
		// include the exact name of the class with a "v" in front
		// and an "s" in back. (See #define in JEventSource_EVIO.cc
		// for more details.)
		vector< vector<DDAQAddress*> > hit_objs;
		class ObjList{
		public:

			ObjList():run_number(0),own_objects(true),eviobuff_parsed(false),eviobuff(NULL),eviobuff_size(0),DOMTree(NULL){}
			
			int32_t run_number;
			bool own_objects; // keeps track of whether these objects were copied to factories or not
			
			vector<DDAQAddress*>    hit_objs;
			vector<DDAQConfig*>     config_objs;
			vector<JObject*>        misc_objs;

			bool eviobuff_parsed;     // flag used to keep track of whether this buffer has been parsed
			uint32_t *eviobuff;       // Only holds original EVIO event buffer
			uint32_t eviobuff_size;   // size of eviobuff in bytes
			evioDOMTree *DOMTree;     // DOM tree which may be modified before generating output buffer from it
		};
	
		// EVIO events with more than one DAQ event ("blocked" or
		// "entangled" events") are parsed and have the events
		// stored in the following container so they can be dispensed
		// as needed.
		pthread_mutex_t stored_events_mutex;
		queue<ObjList*> stored_events;

		// We need to keep the EVIO buffers around for events since they
		// may be needed again before we are done with the event (especially
		// for L3). It is more efficient to maintain a pool of such events
		// and recycle them.
		uint32_t BUFFER_SIZE;
		pthread_mutex_t evio_buffer_pool_mutex;
		deque<uint32_t*> evio_buffer_pool;
		
		// List of the data types this event source can provide
		// (filled in the constructor)
		set<string> event_source_data_types;

		void AddSourceObjectsToCallStack(JEventLoop *loop, string className);
		void AddEmulatedObjectsToCallStack(JEventLoop *loop, string caller, string callee);
		void EmulateDf250PulseIntegral(vector<JObject*> &wrd_objs, vector<JObject*> &pi_objs);
		void EmulateDf125PulseIntegral(vector<JObject*> &wrd_objs, vector<JObject*> &pi_objs);
		void EmulateDf250PulseTime(vector<JObject*> &wrd_objs, vector<JObject*> &pt_objs, vector<JObject*> &pp_objs);
		void EmulateDf125PulseTime(vector<JObject*> &wrd_objs, vector<JObject*> &pt_objs, vector<JObject*> &pp_objs);
		jerror_t ParseEvents(ObjList *objs_ptr);
		int32_t GetRunNumber(evioDOMTree *evt);
		int32_t FindRunNumber(uint32_t *iptr);
		MODULE_TYPE GuessModuleType(const uint32_t *istart, const uint32_t *iend);
		bool IsF250ADC(const uint32_t *istart, const uint32_t *iend);
		bool IsF1TDC(const uint32_t *istart, const uint32_t *iend);
		void DumpModuleMap(void){}
		void DumpBinary(const uint32_t *iptr, const uint32_t *iend=NULL, uint32_t MaxWords=0, const uint32_t *imark=NULL);

		
		void MergeObjLists(list<ObjList*> &events1, list<ObjList*> &events2);

		void ParseEVIOEvent(evioDOMTree *evt, list<ObjList*> &full_events);
		void ParseBuiltTriggerBank(evioDOMNodeP trigbank, list<ObjList*> &tmp_events);
		void ParseModuleConfiguration(int32_t rocid, const uint32_t* &iptr, const uint32_t *iend, list<ObjList*> &events);
		void ParseJLabModuleData(int32_t rocid, const uint32_t* &iptr, const uint32_t *iend, list<ObjList*> &events);
		void Parsef250Bank(int32_t rocid, const uint32_t* &iptr, const uint32_t *iend, list<ObjList*> &events);
		void Parsef125Bank(int32_t rocid, const uint32_t* &iptr, const uint32_t *iend, list<ObjList*> &events);
		void ParseF1TDCBank(int32_t rocid, const uint32_t* &iptr, const uint32_t *iend, list<ObjList*> &events);
		uint32_t F1TDC_channel(uint32_t chip, uint32_t chan_on_chip, int modtype);
		void ParseTSBank(int32_t rocid, const uint32_t* &iptr, const uint32_t *iend, list<ObjList*> &events);
		void ParseTIBank(int32_t rocid, const uint32_t* &iptr, const uint32_t *iend, list<ObjList*> &events);
		void ParseCAEN1190(int32_t rocid, const uint32_t* &iptr, const uint32_t *iend, list<ObjList*> &events);
		void ParseEPICSevent(evioDOMNodeP bankPtr, list<ObjList*> &events);


		// f250 methods
		void MakeDf250WindowRawData(ObjList *objs, uint32_t rocid, uint32_t slot, uint32_t itrigger, const uint32_t* &iptr);
		void MakeDf250PulseRawData(ObjList *objs, uint32_t rocid, uint32_t slot, uint32_t itrigger, const uint32_t* &iptr);
		void MakeDf125WindowRawData(ObjList *objs, uint32_t rocid, uint32_t slot, uint32_t itrigger, const uint32_t* &iptr);
		void MakeDf125PulseRawData(ObjList *objs, uint32_t rocid, uint32_t slot, uint32_t itrigger, const uint32_t* &iptr);

#endif // HAVE_EVIO		

#ifdef HAVE_ET
		et_sys_id sys_id;
		et_att_id att_id;
		et_stat_id sta_id;
#endif
};


#ifdef HAVE_EVIO		


//======================================================================================
// Some of the following methods are inlined so that
// they can be used by programs that only have access 
// to this header at link time. (This class is normally
// compiled into a plugin so there is no library file
// available to link to.)
//
// There are also some templates that are used to make
// some of the code in the implmentation file cleaner.
//======================================================================================

//----------------
// GetEVIOBuffer
//----------------
void JEventSource_EVIO::GetEVIOBuffer(jana::JEvent &jevent, uint32_t* &buff, uint32_t &size) const
{
	/// Use the reference stored in the supplied JEvent to extract the evio
	/// buffer and size for the event. If there is no buffer for the event
	/// then buff will be set to NULL and size to zero. This can happen if
	/// reading entangled events and this is not the first event in the block.

	// In case we bail early
	buff = NULL;
	size = 0;

	// Make sure this JEvent actually came from this source
	if(jevent.GetJEventSource() != this){
		jerr<<" ERROR: Attempting to get EVIO buffer for event not produced by this source!!"<<endl;
		return;
	}

	// Get pointer to ObjList object
	const ObjList *objs_ptr = (ObjList*)jevent.GetRef();
	if(!objs_ptr) return;

	// Copy buffer pointer and size to user's variables
	buff = objs_ptr->eviobuff;
	size = objs_ptr->eviobuff_size;
}

//----------------
// GetEVIODOMTree
//----------------
evioDOMTree* JEventSource_EVIO::GetEVIODOMTree(jana::JEvent &jevent) const
{
	/// Use the reference stored in the supplied JEvent to extract the evio
	/// DOM tree for the event. If there is no DOM tree for the event
	/// then NULL will be returned. This can happen if reading entangled events
	/// and this is not the first event in the block.

	// Make sure this JEvent actually came from this source
	if(jevent.GetJEventSource() != this){
		jerr<<" ERROR: Attempting to get EVIO buffer for event not produced by this source!!"<<endl;
		return NULL;
	}

	// Get pointer to ObjList object
	const ObjList *objs_ptr = (ObjList*)jevent.GetRef();
	if(!objs_ptr) return NULL;

	return objs_ptr->DOMTree;
}


////----------------------------------------------------------------------
///// JFactory_base_CopyToT and JFactory_base_CopyTo
/////
///// A Mantis request has been submitted to add a virtual method to
///// JFactory_base that takes a vector<JObject*>& with an overload
///// of that method in the JFactory<T> subclass. The JFactory<T> method
///// will then try to dynamically cast each JObject* into the appropriate
///// type and store it in the factory. When that is working, these two
///// routines will not be required here.
///// 
///// In the  meantime, this serves as a placeholder that can be easily
///// converted once JANA has been updated.
////----------------------------------------------------------------------
//template<class T>
//bool JFactory_base_CopyToT(jana::JFactory_base *fac, vector<jana::JObject *>& objs)
//{
//	// Try casting this factory to the desired type of JFactory<>
//	jana::JFactory<T> *tfac = dynamic_cast<jana::JFactory<T>* >(fac);
//	if(!tfac) return false;
//	
//	// Factory cast worked. Cast all pointers
//	vector<T*> tobjs;
//	for(unsigned int i=0; i<objs.size(); i++){
//		T *tobj = dynamic_cast<T*>(objs[i]);
//		if(tobj) tobjs.push_back(tobj);
//	}
//	
//	// If all input objects weren't converted, then just return false
//	if(tobjs.size() != objs.size()) return false;
//	
//	// Copy pointers into factory
//	if(tobjs.size()>0) tfac->CopyTo(tobjs);
//	return true;
//}
//
////----------------------------
//// JFactory_base_CopyTo
////----------------------------
//static bool JFactory_base_CopyTo(jana::JFactory_base *fac, vector<jana::JObject *>& objs)
//{
//	// Eventually, this will be a virtual method of JFactory_base
//	// that gets implemented in JFactory<T> which will know how
//	// to cast the objects. For now though, we have to try all known
//	// data types.
//	if( JFactory_base_CopyToT<Df250PulseIntegral>(fac, objs) ) return true;
//	if( JFactory_base_CopyToT<Df250StreamingRawData>(fac, objs) ) return true;
//	if( JFactory_base_CopyToT<Df250WindowSum>(fac, objs) ) return true;
//	if( JFactory_base_CopyToT<Df250PulseRawData>(fac, objs) ) return true;
//	if( JFactory_base_CopyToT<Df250TriggerTime>(fac, objs) ) return true;
//	if( JFactory_base_CopyToT<Df250PulseTime>(fac, objs) ) return true;
//	if( JFactory_base_CopyToT<Df250WindowRawData>(fac, objs) ) return true;
//	if( JFactory_base_CopyToT<Df125PulseIntegral>(fac, objs) ) return true;
//	if( JFactory_base_CopyToT<Df125TriggerTime>(fac, objs) ) return true;
//	if( JFactory_base_CopyToT<Df125PulseTime>(fac, objs) ) return true;
//	if( JFactory_base_CopyToT<DF1TDCHit>(fac, objs) ) return true;
//	if( JFactory_base_CopyToT<DF1TDCTriggerTime>(fac, objs) ) return true;
//
//	return false;
//}

//----------------------------
// AddIfAppropriate
//----------------------------
template<class T>
void AddIfAppropriate(DDAQAddress *obj, vector<T*> &v)
{
	T *t = dynamic_cast<T*>(obj);
	if(t!= NULL) v.push_back(t);
}

//----------------------------
// LinkAssociationsWithPulseNumber
//----------------------------
template<class T, class U>
void LinkAssociationsWithPulseNumber(vector<T*> &a, vector<U*> &b)
{
	/// Template routine to loop over two vectors of pointers to
	/// objects derived from DDAQAddress. This will find any hits
	/// coming from the same DAQ channel and add each to the other's
	/// AssociatedObjects list. This will also check if the member
	/// "pulse_number" is the same (use LinkAssociations to not check
	/// the pulse_number such as when either "T" or "U" does not have
	/// a member named "pulse_number".)
	for(unsigned int j=0; j<a.size(); j++){
		for(unsigned int k=0; k<b.size(); k++){
			if(a[j]->pulse_number != b[k]->pulse_number) continue;
			if(*a[j] == *b[k]){ // compare only the DDAQAddress parts
				a[j]->AddAssociatedObject(b[k]);
				b[k]->AddAssociatedObject(a[j]);
			}
		}
	}
}

//----------------------------
// LinkAssociations
//----------------------------
template<class T, class U>
void LinkAssociations(vector<T*> &a, vector<U*> &b)
{
	/// Template routine to loop over two vectors of pointers to
	/// objects derived from DDAQAddress. This will find any hits
	/// coming from the same DAQ channel and add each to the other's
	/// AssociatedObjects list. This will NOT check if the member
	/// "pulse_number" is the same (use LinkAssociationsWithPulseNumber
	/// for that.)
	for(unsigned int j=0; j<a.size(); j++){
		for(unsigned int k=0; k<b.size(); k++){
			if( *((DDAQAddress*)a[j]) == *((DDAQAddress*)b[k]) ){ // compare only the DDAQAddress parts
				a[j]->AddAssociatedObject(b[k]);
				b[k]->AddAssociatedObject(a[j]);
			}
		}
	}
}

//----------------------------
// LinkAssociationsModuleOnly
//----------------------------
template<class T, class U>
void LinkAssociationsModuleOnly(vector<T*> &a, vector<U*> &b)
{
	/// Template routine to loop over two vectors of pointers to
	/// objects derived from DDAQAddress. This will find any hits
	/// coming from the same DAQ module (channel number is not checked)
	/// When a match is found, the pointer from "a" will be added
	/// to "b"'s AssociatedObjects list. This will NOT do the inverse
	/// of adding "b" to "a"'s list. It is intended for adding a module
	/// level trigger time object to all hits from that module. Adding
	/// all of the hits to the trigger time object seems like it would
	/// be a little expensive with no real use case.
	for(unsigned int j=0; j<a.size(); j++){
		for(unsigned int k=0; k<b.size(); k++){
			if(a[j]->rocid != b[k]->rocid) continue;
			if(a[j]->slot != b[k]->slot) continue;

			b[k]->AddAssociatedObject(a[j]);
		}
	}
}

//----------------------------
// LinkAssociationsROCIDOnly
//----------------------------
template<class T, class U>
void LinkAssociationsROCIDOnly(vector<T*> &a, vector<U*> &b)
{
	/// Template routine to loop over two vectors of pointers to
	/// objects derived from DDAQAddress. This will find any hits
	/// coming from the same DAQ module (channel number is not checked)
	/// When a match is found, the pointer from "a" will be added
	/// to "b"'s AssociatedObjects list. This will NOT do the inverse
	/// of adding "b" to "a"'s list. It is intended for adding a module
	/// level trigger time object to all hits from that module. Adding
	/// all of the hits to the trigger time object seems like it would
	/// be a little expensive with no real use case.
	for(unsigned int j=0; j<a.size(); j++){
		for(unsigned int k=0; k<b.size(); k++){
			if(a[j]->rocid != b[k]->rocid) continue;

			b[k]->AddAssociatedObject(a[j]);
		}
	}
}

//----------------------------
// CopyContainerElementsWithCast
//----------------------------
template<class T, class U>
void CopyContainerElementsWithCast(vector<T*> &a, vector<U*> &b)
{
	/// This is used to copy pointers from a vector of one type of
	/// pointer to a vector of another type, doing a static cast
	/// in the process. For example, to fill a vector<JObject*> a
	/// from a vector<Df250PulseIntegral*> b, call:
	///
	///  CopyContainerElementsWithCast(b, a);
	///
	/// Note that this does not do any dynamic_cast-ing to ensure
	/// that the objects really are of compatible types. So be
	/// cautious.

	for(uint32_t i=0; i<a.size(); i++){
		b.push_back((U*)a[i]);
	}
}

#endif // HAVE_EVIO		


#endif // _JEventSourceGenerator_DAQ_

