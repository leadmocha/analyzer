////////////////////////////////////////////////////////////////////
//
//   THaEpics
//   Hall A EPICS data.
//
//   EPICS data come in two forms
//     (tag, value)   e.g. (IPM1H04B.XPOS,  0.204)
//   and
//     (tag, value, units)  e.g. (HELG0TSETTLEs, 500, usec)
//
//   All data are received as characters and are parsed.
//   'tags' remain characters, 'values' are either character 
//   or double, and 'units' are characters.
//   Data are stored in an STL map and retrievable by
//   'tag' (e.g. IPM1H04B.XPOS) and by proximity to
//   a physics event number (closest one is picked).
//
//   Replaces THaEpicsStack (obsolete)
//
//   author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include "THaEpics.h"
#include "TMath.h"
#include <iostream>
#include <string>
#include <sstream>

using namespace std;

namespace Decoder {

void THaEpics::Print() {
  cout << "\n\n====================== \n";
  cout << "Print of Epics Data : "<<endl;
  Int_t j = 0;
  for( auto& pm : epicsData ) {
    const vector<EpicsChan>& vepics = pm.second;
    const string& tag = pm.first;
    j++;
    cout << "\n\nEpics Var #" << j;
    cout << "   Var Name =  \""<<tag<<"\""<<endl;
    cout << "Size of epics vector "<<vepics.size();
    for( const auto& chan : vepics ) {
      cout << "\n Tag = " << chan.GetTag();
      //      if (strstr(vepics[k].GetTag().c_str(),"VMI3128")!=nullptr) {
      //	cout << "\n GOT ONE "<<k<<endl;
      //        exit(0);
      //      }
      cout << "   Evnum = " << chan.GetEvNum();
      cout << "   Date = " << chan.GetDate();
      cout << "   Timestamp = " << chan.GetTimeStamp();
      cout << "   Data = " << chan.GetData();
      cout << "   String = " << chan.GetString();
      cout << "   Units = " << chan.GetUnits();
    }
    cout << endl;
  }
}

Bool_t THaEpics::IsLoaded(const char* tag) const
{
  const vector<EpicsChan> ep = GetChan(tag);
  return !ep.empty();
}

Double_t THaEpics::GetData (const char* tag, int event) const
{
  const vector<EpicsChan> ep = GetChan(tag);
  Int_t k = FindEvent(ep, event);
  if ( k < 0) return 0;
  return ep[k].GetData();
}  

string THaEpics::GetString (const char* tag, int event) const
{
  const vector<EpicsChan> ep = GetChan(tag);
  Int_t k = FindEvent(ep, event);
  if ( k < 0) return "";
  return ep[k].GetString();
}  

Double_t THaEpics::GetTimeStamp(const char* tag, int event) const
{
  const vector<EpicsChan> ep = GetChan(tag);
  Int_t k = FindEvent(ep, event);
  if ( k < 0) return 0;
  return ep[k].GetTimeStamp();
}

vector<EpicsChan> THaEpics::GetChan(const char *tag) const
{
  // Return the vector of Epics data for 'tag' 
  // where 'tag' is the name of the Epics variable.
  vector<EpicsChan> ep;
  ep.clear();
  auto pm = epicsData.find(string(tag));
  if (pm != epicsData.end()) ep = pm->second;
  return ep;
}


Int_t THaEpics::FindEvent(const vector<EpicsChan>& ep, int event) const
{
  // Return the index in the vector of Epics data 
  // nearest in event number to event 'event'.
  if (ep.empty())
    return -1;
  Int_t myidx = ep.size()-1;
  if (event == 0) return myidx;  // return last event 
  double min = 9999999;
  for (size_t k = 0; k < ep.size(); k++) {
    double diff = event - ep[k].GetEvNum();
    if (diff < 0) diff = -1*diff;
    if (diff < min) {
      min = diff;
      myidx = k;
    }
  }
  return myidx;
}
        

int THaEpics::LoadData(const UInt_t* evbuffer, int evnum)
{ 
  // load data from the event buffer 'evbuffer' 
  // for event nearest 'evnum'.

  const unsigned int DEBUGL = 0;
  const string::size_type MAX_VAL_LEN = 32;

  const char* cbuff = (const char*)evbuffer;
  size_t len = sizeof(int)*(evbuffer[0]+1);  
  if (DEBUGL>1) cout << "Enter loadData, len = "<<len<<endl;
  if( len<16 ) return 0;
  // The first 16 bytes of the buffer are the event header
  len -= 16;
  cbuff += 16;
  
  // Copy data to internal string buffer
  string buf( cbuff, len );

  // The first line is the time stamp
  string date;
  istringstream ib(buf);
  if( !getline(ib,date) || date.size() < 16 ) {
    cerr << "Invalid time stamp for EPICS event at evnum = " << evnum << endl;
    return 0;
  }
  if(DEBUGL>1) cout << "Timestamp: " << date <<endl;

  string line;
  istringstream il, iv;
  while( getline(ib,line) ) {
    // Here we parse each line
    if(DEBUGL>2) cout << "epics line : "<<line<<endl;
    il.clear(); il.str(line);
    string wtag, wval, wunits;
    il >> wtag;
    if( wtag.empty() || wtag[0] == 0 ) continue;
    istringstream::pos_type spos = il.tellg();
    il >> wval;
    Double_t dval;
    bool got_val = false;
    if( !wval.empty() && wval.length() <= MAX_VAL_LEN ) {
      iv.clear(); iv.str(wval);
      if( iv >> dval )
        got_val = true;
    }
    if( got_val ) {
      il >> wunits; // Assumes that wunits contains no whitespace
    } else {
      // Mimic the old behavior: if the string doesn't convert to a number,
      // then wval = rest of string after tag, dval = 0, sunit = empty
      string::size_type lpos = line.find_first_not_of(" \t",spos);
      wval = ( lpos != string::npos ) ? line.substr(lpos) : "";
      dval = 0;
    }
    if(DEBUGL>2) cout << "wtag = "<<wtag<<"   wval = "<<wval
		      << "   dval = "<<dval<<"   wunits = "<<wunits<<endl;

    // Add tag/value/units to the EPICS data.    
    epicsData[wtag].push_back( EpicsChan(wtag,date,evnum,wval,wunits,dval) );
  }
  if(DEBUGL) Print();
  return 1;
}

}

ClassImp(Decoder::THaEpics)
