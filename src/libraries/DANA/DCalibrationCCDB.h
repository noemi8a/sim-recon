#ifndef _DCalibrationCCDB_
#define _DCalibrationCCDB_

#include <exception>
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <pthread.h>

#include <JANA/jerror.h>
#include <JANA/JCalibration.h>
#include <JANA/JStreamLog.h>
#include <CCDB/Calibration.h>

using namespace std;
using namespace jana;

// Place everything in JANA namespace
namespace jana
{

	/** 
	 *  Descendant of JCalibration class which allow to use CCDB as JANA calibration source
	 */
    class DCalibrationCCDB : public JCalibration
    {
    public:
	 		/// DEPRECATED:
			/// This file will be removed from here and the functionality 
			/// replaced with JCalibrationCCDB.h which resides in the JANA
			/// library. That has been updated with mutex locks to match
			/// this file as of JANA version 0.7.3. However, JANA 0.7.3 also
			/// changes the signature of the GetCalib virtual methods to
			/// use uint64_t and int32_t. The following methods are wrappers
			/// that allow this file to be used with JANA 0.7.3 (as well as
			/// previous versions) until 0.7.3 becomes commonplace and this
			/// file can be removed.
			///
			///  June 10, 2015  David Lawrence
		 virtual bool GetCalib(string namepath, map<string, string> &svals, uint64_t event_number=0){return GetCalib(namepath,svals,(int)event_number);}
		 virtual bool GetCalib(string namepath, vector<string> &svals, uint64_t event_number=0){return GetCalib(namepath,svals,(int)event_number);}
		 virtual bool GetCalib(string namepath, vector< map<string, string> > &svals, uint64_t event_number=0){return GetCalib(namepath,svals,(int)event_number);}
		 virtual bool GetCalib(string namepath, vector< vector<string> > &svals, uint64_t event_number=0){return GetCalib(namepath,svals,(int)event_number);}
			
	 

        /** @brief    Constructor
         *
         * @parameter [in] url - connection string. like mysql://...
         * @parameter [in] run - run number
         * @parameter [in] context - variation
         */
        DCalibrationCCDB(ccdb::Calibration* calib, string url, int run, string context="default"):
	        JCalibration(calib->GetConnectionString(), run, context)
	    {

		    mCalibration = calib;
		    pthread_mutex_init(&mutex, NULL);

			//>oO CCDB debug output
			#ifdef CCDB_DEBUG_OUTPUT
			jout<<"CCDB::janaccdb created DCalibrationCCDB with connection string:" << calib->GetConnectionString()<< " run:"<<run<< " context:"<<context<<endl;
			#endif
        }


        /** @brief   destructor
         */
        virtual ~DCalibrationCCDB()
        {
            if(mCalibration!=NULL){
               pthread_mutex_lock(&mutex);
               delete mCalibration;
               pthread_mutex_unlock(&mutex);
			}
        }
        

        /** @brief gets a className
         */
        virtual const char* className(void)
        {
            return static_className();
        }


        /** @brief gets a className static version of function
         */
        static const char* static_className(void)
        {
            return "DCalibrationCCDB";
        }


        /** @brief    get calibration constants
         *
         * @parameter [in]  namepath - full resource string
         * @parameter [out] svals - data to be returned
         * @parameter [in]  event_number - optional parameter of event number
         * @return true if constants were read
         */
        bool GetCalib(string namepath, map<string, string> &svals, int event_number=0)
        {
            // Lock mutex for exclusive use of underlying Calibration object
            pthread_mutex_lock(&mutex);
		
            //
            try
            {  
				//>oO CCDB debug output
                #ifdef CCDB_DEBUG_OUTPUT                
                cout<<"CCDB::janaccdb"<<endl;
                cout<<"CCDB::janaccdb REQUEST map<string, string> request = '"<<namepath<<"'"<<endl;
                #endif  //>end of  CCDB debug output
                 
                bool result = mCalibration->GetCalib(svals, namepath);

                //>oO CCDB debug output
                #ifdef CCDB_DEBUG_OUTPUT
                cout<<"CCDB::janaccdb result = "<<string((result)?string("loaded"):string("failure"))<<endl;
                if(result)
                {
                    string first_value(" --NAN-- ");
                    if(svals.size()>0)
                    {
                        map<string, string>::const_iterator iter = svals.begin();
                        first_value.assign(iter->second);
                    }
                    cout<<"CCDB::janaccdb selected name-values count = '"<<svals.size()<<"' first_value '"<<first_value<<"'"<<endl;
                }
                #endif  //>end of  CCDB debug output

				pthread_mutex_unlock(&mutex);
                return !result; //JANA has false - if success and true if error
            }
            catch (std::exception& ex)
            {
                //>oO CCDB debug output
                #ifdef CCDB_DEBUG_OUTPUT
                cout <<"CCDB::janaccdb Exception caught at GetCalib(string namepath, map<string, string> &svals, int event_number=0)"<<endl;
                cout <<"CCDB::janaccdb what = "<<ex.what()<<endl;
                #endif //end of CCDB debug output

                pthread_mutex_unlock(&mutex);
                return true; //JANA has false - if success and true if error
            }
        }


        /** @brief    get calibration constants
         *
         * @parameter [in]  namepath - full resource string
         * @parameter [out] svals - data to be returned
         * @parameter [in]  event_number - optional parameter of event number
         * @return true if constants were read
         */
        bool GetCalib(string namepath, vector<string> &svals, int event_number=0)
        {
            // Lock mutex for exclusive use of underlying Calibration object
            pthread_mutex_lock(&mutex);
		
            //
            try
            {  
				//>oO CCDB debug output
                #ifdef CCDB_DEBUG_OUTPUT                
                cout<<"CCDB::janaccdb"<<endl;
                cout<<"CCDB::janaccdb REQUEST vector<string> request = '"<<namepath<<"'"<<endl;
                #endif  //>end of  CCDB debug output
                 
                bool result = mCalibration->GetCalib(svals, namepath);

                //>oO CCDB debug output
                #ifdef CCDB_DEBUG_OUTPUT
                cout<<"CCDB::janaccdb result = "<<string((result)?string("loaded"):string("failure"))<<endl;
                if(result)
                {
                    string first_value(" --NAN-- ");
                    if(svals.size()>0)
                    {
                        vector<string>::const_iterator iter = svals.begin();
                        first_value.assign(*iter);
                    }
                    cout<<"CCDB::janaccdb selected name-values count = '"<<svals.size()<<"' first_value '"<<first_value<<"'"<<endl;
                }
                #endif  //>end of  CCDB debug output

				pthread_mutex_unlock(&mutex);
                return !result; //JANA has false - if success and true if error
            }
            catch (std::exception& ex)
            {
                //>oO CCDB debug output
                #ifdef CCDB_DEBUG_OUTPUT
                cout <<"CCDB::janaccdb Exception caught at GetCalib(string namepath, vector<string> &svals, int event_number=0)"<<endl;
                cout <<"CCDB::janaccdb what = "<<ex.what()<<endl;
                #endif //end of CCDB debug output

                pthread_mutex_unlock(&mutex);
                return true; //JANA has false - if success and true if error
            }
        }


         /** @brief    get calibration constants
         *
         * @parameter [in]  namepath - full resource string
         * @parameter [out] vsvals - data to be returned
         * @parameter [in]  event_number - optional parameter of event number
         * @return true if constants were read
         */
        bool GetCalib(string namepath, vector< map<string, string> > &vsvals, int event_number=0)
        {
            // Lock mutex for exclusive use of underlying Calibration object
            pthread_mutex_lock(&mutex);

            try
            {
				
				 //>oO CCDB debug output
                 #ifdef CCDB_DEBUG_OUTPUT
                 cout<<"CCDB::janaccdb"<<endl;
                 cout<<"CCDB::janaccdb REQUEST vector<map<string, string>> request = '"<<namepath<<"'"<<endl;
                 #endif  //end of CCDB debug output
                 
                 bool result = mCalibration->GetCalib(vsvals, namepath);

                 //>oO CCDB debug output
                 #ifdef CCDB_DEBUG_OUTPUT
                 cout<<"CCDB::janaccdb result = "<<string ((result)?string("loaded"):string("failure"))<<endl;
                 if(result)
                 {
                     string first_value(" --NAN-- ");
                     if(vsvals.size()>0 && vsvals[0].size()>0)
                     {
                         map<string, string>::const_iterator iter = vsvals[0].begin();
                         first_value.assign(iter->second);
                     }

                     cout<<"CCDB::janaccdb selected rows = '"<<vsvals.size() <<"' selected columns = '"<<(int)((vsvals.size()>0)? vsvals[0].size() :0)
                         <<"' first value = '"<<first_value<<"'"<<endl;
                 }
                 #endif  //end of CCDB debug output


                pthread_mutex_unlock(&mutex);
                return !result; //JANA has false - if success and true if error, CCDB otherwise
            }
            catch (std::exception& ex)
            {
                //>oO CCDB debug output
                #ifdef CCDB_DEBUG_OUTPUT
                cout <<"CCDB::janaccdb Exception caught at GetCalib(string namepath, vector<map<string, string> > &svals, int event_number=0)"<<endl;
                cout <<"CCDB::janaccdb what = "<<ex.what()<<endl;
                #endif

                pthread_mutex_unlock(&mutex);
                return true; //JANA has false - if success and true if error, CCDB otherwise
            }
        }


          /** @brief    get calibration constants
         *
         * @parameter [in]  namepath - full resource string
         * @parameter [out] vsvals - data to be returned
         * @parameter [in]  event_number - optional parameter of event number
         * @return true if constants were read
         */
        bool GetCalib(string namepath, vector< vector<string> > &vsvals, int event_number=0)
        {
            // Lock mutex for exclusive use of underlying Calibration object
            pthread_mutex_lock(&mutex);

            try
            {
				
				 //>oO CCDB debug output
                 #ifdef CCDB_DEBUG_OUTPUT
                 cout<<"CCDB::janaccdb"<<endl;
                 cout<<"CCDB::janaccdb REQUEST vector<vector<string> > request = '"<<namepath<<"'"<<endl;
                 #endif  //end of CCDB debug output
                 
                 bool result = mCalibration->GetCalib(vsvals, namepath);

                 //>oO CCDB debug output
                 #ifdef CCDB_DEBUG_OUTPUT
                 cout<<"CCDB::janaccdb result = "<<string ((result)?string("loaded"):string("failure"))<<endl;
                 if(result)
                 {
                     string first_value(" --NAN-- ");
                     if(vsvals.size()>0 && vsvals[0].size()>0)
                     {
                         vector<string>::const_iterator iter = vsvals[0].begin();
                         first_value.assign(*iter);
                     }

                     cout<<"CCDB::janaccdb selected rows = '"<<vsvals.size() <<"' selected columns = '"<<(int)((vsvals.size()>0)? vsvals[0].size() :0)
                         <<"' first value = '"<<first_value<<"'"<<endl;
                 }
                 #endif  //end of CCDB debug output


                pthread_mutex_unlock(&mutex);
                return !result; //JANA has false - if success and true if error, CCDB otherwise
            }
            catch (std::exception& ex)
            {
                //>oO CCDB debug output
                #ifdef CCDB_DEBUG_OUTPUT
                cout <<"CCDB::janaccdb Exception caught at GetCalib(string namepath, vector<vector<string> > &svals, int event_number=0)"<<endl;
                cout <<"CCDB::janaccdb what = "<<ex.what()<<endl;
                #endif

                pthread_mutex_unlock(&mutex);
                return true; //JANA has false - if success and true if error, CCDB otherwise
            }
        }


       /** @brief    GetListOfNamepaths
         *
         * @parameter [in] vector<string> & namepaths
         * @return   void
         */
        void GetListOfNamepaths(vector<string> &namepaths)
        {
            // Lock mutex for exclusive use of underlying Calibration object
            pthread_mutex_lock(&mutex);

            try
            {  
				//some ccdb debug output
                #ifdef CCDB_DEBUG_OUTPUT
                cout<<"CCDB::janaccdb Getting list of namepaths. "<<endl;
                #endif
                
                mCalibration->GetListOfNamepaths(namepaths);
            }
            catch (std::exception& ex)
            {

                //some ccdb debug output
                #ifdef CCDB_DEBUG_OUTPUT
                cout<<"CCDB::janaccdb Exception cought at GetListOfNamepaths(vector<string> &namepaths). What = "<< ex.what()<<endl;
                #endif
            }
            pthread_mutex_unlock(&mutex);
        }
        
    private:
        DCalibrationCCDB();					// prevent use of default constructor
        ccdb::Calibration * mCalibration;	///Underlaying CCDB user api class 
        pthread_mutex_t mutex;
        
    };

} // Close JANA namespace

#endif // _DCalibrationCCDB_
