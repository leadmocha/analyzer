#ifndef Podd_THaEvData_h_
#define Podd_THaEvData_h_

/////////////////////////////////////////////////////////////////////
//
//   THaEvData
//
/////////////////////////////////////////////////////////////////////


#include "Decoder.h"
#include "Module.h"
#include "TObject.h"
#include "TString.h"
#include "THaSlotData.h"
#include "TBits.h"
#include <cassert>
#include <iostream>
#include <cstdio>

class THaBenchmark;

class THaEvData : public TObject {

public:
  THaEvData();
  virtual ~THaEvData();

  // Return codes for LoadEvent
  enum { HED_OK = 0, HED_WARN = -63, HED_ERR = -127, HED_FATAL = -255 };

  // Parse raw data in 'evbuffer'. Actual decoding/unpacking takes place here.
  // Derived classes MUST implement this function.
  virtual Int_t LoadEvent(const UInt_t* evbuffer) = 0;

  // return a pointer to a full event
  const UInt_t* GetRawDataBuffer() const { return buffer;}  

  virtual Bool_t IsMultiBlockMode() { return fMultiBlockMode; };
  virtual Bool_t BlockIsDone() { return fBlockIsDone; };
  virtual void   FillBankData(UInt_t* /*rdat*/, Int_t /*roc*/, Int_t /*bank*/,
			      Int_t /*offset*/=0, Int_t /*num*/=1) const {};

  // Derived class to implement this if multiblock mode supported
  virtual Int_t LoadFromMultiBlock() { assert(fgAllowUnimpl); return HED_ERR;};

  virtual Int_t Init();

  // Set the EPICS event type
  void      SetEpicsEvtType(Int_t itype) { fEpicsEvtType = itype; };

  void SetEvTime(const ULong64_t evtime) { evt_time = evtime; }

  // Basic access to the decoded data
  Int_t     GetEvType()      const { return event_type; }
  Int_t     GetEvLength()    const { return event_length; }
  Int_t     GetEvNum()       const { return event_num; }
  Int_t     GetRunNum()      const { return run_num; }
  Int_t     GetDataVersion() const { return fDataVersion; }
  // Run time/date. Time of prestart event (UNIX time).
  ULong64_t GetRunTime()     const { return fRunTime; }
  Int_t     GetRunType()     const { return run_type; }
  Int_t     GetRocLength(Int_t crate) const;   // Get the ROC length

  Bool_t    IsPhysicsTrigger() const;  // physics trigger (event types 1-14)
  Bool_t    IsScalerEvent()    const;  // scalers from data stream
  Bool_t    IsPrestartEvent()  const;  // prestart event
  Bool_t    IsEpicsEvent()     const;  // slow control data
  Bool_t    IsPrescaleEvent()  const;  // prescale factors
  Bool_t    IsSpecialEvent()   const;  // e.g. detmap or trigger file insertion
  // number of raw words in crate, slot
  Int_t     GetNumRaw(Int_t crate, Int_t slot) const;
  // raw words for hit 0,1,2.. on crate, slot
  Int_t     GetRawData(Int_t crate, Int_t slot, Int_t hit) const;
  // To retrieve data by crate, slot, channel, and hit# (hit=0,1,2,..)
  Int_t     GetRawData(Int_t crate, Int_t slot, Int_t chan, Int_t hit) const;
  // To get element #i of the raw evbuffer
  Int_t     GetRawData(Int_t i) const;
  // Get raw element i within crate
  Int_t     GetRawData(Int_t crate, Int_t i) const;
  // Get raw data buffer for crate
  const UInt_t* GetRawDataBuffer(Int_t crate) const;
  Int_t     GetNumHits(Int_t crate, Int_t slot, Int_t chan) const;
  Int_t     GetData(Int_t crate, Int_t slot, Int_t chan, Int_t hit) const;
  Bool_t    InCrate(Int_t crate, Int_t i) const;
  // Num unique channels hit
  Int_t     GetNumChan(Int_t crate, Int_t slot) const;
  // List unique chan
  Int_t     GetNextChan(Int_t crate, Int_t slot, Int_t index) const;
  const char* DevType(Int_t crate, Int_t slot) const;

  Bool_t HasCapability( Decoder::EModuleType type, Int_t crate, Int_t slot ) const
  {
    Decoder::Module* module = GetModule(crate, slot);
    if (!module) {
      std::cerr << "No module at crate "<<crate<<"   slot "<<slot<<std::endl;
      return false;
    }
    return module->HasCapability(type);
  }
  Bool_t IsMultifunction( Int_t crate, Int_t slot ) const
  {
    Decoder::Module* module = GetModule(crate, slot);
    if (!module) {
      std::cerr << "No module at crate "<<crate<<"   slot "<<slot<<std::endl;
      return false;
    }
    return module->IsMultiFunction();
  }

  Int_t GetNumEvents( Decoder::EModuleType type, Int_t crate, Int_t slot, Int_t chan) const
  {
    Decoder::Module* module = GetModule(crate, slot);
    if (!module) return 0;
    if (module->HasCapability( type )) {
      return module->GetNumEvents(type, chan);
    } else {
      return GetNumHits(crate, slot, chan);
    }
  }

  Int_t GetData( Decoder::EModuleType type, Int_t crate, Int_t slot, Int_t chan, Int_t hit ) const
  {
    Decoder::Module* module = GetModule(crate, slot);
    if (!module) return 0;
    if (module->HasCapability( type )) {
      if (hit >= module->GetNumEvents(type, chan)) return 0;
      return module->GetData(type, chan, hit);
    } else {
      return GetData( crate, slot, chan, hit );
    }
  }

  Int_t GetLEbit(Int_t crate, Int_t slot, Int_t chan, Int_t hit ) const 
  { // get the Leading Edge bit (works for fastbus)
    return GetOpt(crate, slot, chan, hit );
  }

  Int_t GetOpt( Int_t crate, Int_t slot, Int_t /*chan*/, Int_t hit ) const
  {// get the "Opt" bit (works for fastbus, is otherwise zero)
    Decoder::Module* module = GetModule(crate, slot);
    if (!module) {
      std::cerr << "No module at crate "<<crate<<"   slot "<<slot<<std::endl;
      return 0;
    }
    return module->GetOpt(GetRawData(crate, slot, hit));
  }

  // Optional functionality that may be implemented by derived classes
  virtual ULong64_t GetEvTime() const { return evt_time; }
   // Returns Beam Helicity (-1,0,+1)  '0' is 'unknown'
  virtual Int_t GetHelicity() const   { return 0; }
  // Beam Helicity for spec="left","right"
  virtual Int_t GetHelicity(const TString& /*spec*/) const
  { return GetHelicity(); }
  virtual Int_t GetPrescaleFactor(Int_t /*trigger*/ ) const
  { assert(fgAllowUnimpl); return -1; }
  // User can GetScaler, alternatively to GetSlotData for scalers
  // spec = "left", "right", "rcs" for event type 140 scaler "events"
  // spec = "evleft" or "evright" for L,R scalers injected into data stream.
  virtual Int_t GetScaler(Int_t /*roc*/, Int_t /*slot*/, Int_t /*chan*/) const
  { assert(ScalersEnabled() && fgAllowUnimpl); return kMaxInt; };
  virtual Int_t GetScaler(const TString& /*spec*/,
			  Int_t /*slot*/, Int_t /*chan*/) const
  { return GetScaler(0,0,0); }
  virtual void SetDebugFile( std::ofstream *file ) { fDebugFile = file; };
  virtual Decoder::Module* GetModule(Int_t roc, Int_t slot) const;

  // Access functions for EPICS (slow control) data
  virtual double GetEpicsData(const char* tag, Int_t event=0) const;
  virtual double GetEpicsTime(const char* tag, Int_t event=0) const;
  virtual TString GetEpicsString(const char* tag, Int_t event=0) const;
  virtual Bool_t IsLoadedEpics(const char* /*tag*/ ) const
  { return false; }

  Int_t GetNslots() const { return fNSlotUsed; };
  virtual void PrintSlotData(Int_t crate, Int_t slot) const;
  virtual void PrintOut() const;
  virtual void SetRunTime( ULong64_t tloc );
  virtual Int_t SetDataVersion( Int_t version );

  // Status control
  void    EnableBenchmarks( Bool_t enable=true );
  void    EnableHelicity( Bool_t enable=true );
  Bool_t  HelicityEnabled() const;
  void    EnableScalers( Bool_t enable=true );
  Bool_t  ScalersEnabled() const;
  void    SetOrigPS( Int_t event_type );
  TString GetOrigPS() const;

  UInt_t  GetInstance() const { return fInstance; }
  static UInt_t GetInstances() { return fgInstances.CountBits(); }

  Decoder::THaCrateMap* GetCrateMap() const { return fMap; }

  // Reporting level
  void SetVerbose( UInt_t level );
  void SetDebug( UInt_t level );

  // Utility function for hexdumping any sort of data
  static void hexdump(const char* cbuff, size_t len);

  void SetCrateMapName( const char* name );
  static void SetDefaultCrateMapName( const char* name );

  enum { MAX_PSFACT = 12 };

protected:
  // Initialization routines
  virtual Int_t init_cmap();
  virtual Int_t init_cmap_openfile(FILE*&, TString&) { return 0; }
  virtual Int_t init_slotdata();
  virtual void  makeidx(Int_t crate, Int_t slot);
  virtual void  FindUsedSlots();

  // Helper functions
  Int_t  idx(Int_t crate, Int_t slot) const;
  Int_t  idx(Int_t crate, Int_t slot);
  Bool_t GoodCrateSlot(Int_t crate, Int_t slot) const;
  Bool_t GoodIndex(Int_t crate, Int_t slot) const;

  // Data
  Decoder::THaCrateMap* fMap;      // Pointer to active crate map

  struct RocDat_t {           // ROC raw data descriptor
    RocDat_t() : pos(0), len(0) {}
    Int_t pos;                // position in evbuffer[]
    Int_t len;                // length of data
  } rocdat[Decoder::MAXROC];

  // Control bits in TObject::fBits used by decoders
  enum {
    kHelicityEnabled = BIT(14),
    kScalersEnabled  = BIT(15),
  };

  struct BankDat_t {           // Bank raw data descriptor
    Int_t pos;                 // position in evbuffer[]
    Int_t len;                 // length of data
  } bankdat[Decoder::MAXBANK * Decoder::MAXROC]{};
  Decoder::THaSlotData** crateslot;

  Bool_t first_decode;
  Bool_t fTrigSupPS;
  Bool_t fMultiBlockMode, fBlockIsDone;
  Int_t  fDataVersion;    // Data format version (implementation-defined)
  Int_t  fEpicsEvtType;

  const UInt_t *buffer;

  std::ofstream *fDebugFile;  // debug output

  Int_t  event_type, event_length, event_num, run_num, evscaler;
  Int_t  bank_tag, data_type, block_size, tbLen;
  Int_t  run_type;    // Run type
  ULong64_t fRunTime; // Run start time (Unix time)
  ULong64_t evt_time; // Event time
  Int_t  recent_event;
  Bool_t buffmode,synchmiss,synchextra;

  Int_t     fNSlotUsed;   // Number of elements of crateslot[] actually used
  Int_t     fNSlotClear;  // Number of elements of crateslot[] to clear
  UShort_t* fSlotUsed;    // [fNSlotUsed] Indices of crateslot[] used
  UShort_t* fSlotClear;   // [fNSlotClear] Indices of crateslot[] to clear

  Bool_t fDoBench;
  THaBenchmark *fBench;

  UInt_t fInstance;            // My instance
  static TBits fgInstances;    // Number of instances of this object

  static const Double_t kBig;  // default value for invalid data
  static Bool_t fgAllowUnimpl; // If true, allow unimplemented functions

  static TString fgDefaultCrateMapName; // Default crate map name
  TString fCrateMapName; // Crate map database file name to use
  Bool_t fNeedInit;  // Crate map needs to be (re-)initialized

  Int_t  fDebug;     // Debug/verbosity level

  TObject* fExtra;   // additional member data, for binary compatibility

  ClassDef(THaEvData,0)  // Base class for raw data decoders

};

//=============== inline functions ================================

//Utility function to index into the crateslot array
inline Int_t THaEvData::idx( Int_t crate, Int_t slot) const {
  return slot+Decoder::MAXSLOT*crate;
}
//Like idx() const, but initializes empty slots
inline Int_t THaEvData::idx( Int_t crate, Int_t slot) {
  Int_t ix = slot+Decoder::MAXSLOT*crate;
  if( !crateslot[ix] ) makeidx(crate,slot);
  return ix;
}

inline Bool_t THaEvData::GoodCrateSlot( Int_t crate, Int_t slot ) const {
  return ( crate >= 0 && crate < Decoder::MAXROC &&
	   slot >= 0 && slot < Decoder::MAXSLOT );
}

inline Bool_t THaEvData::GoodIndex( Int_t crate, Int_t slot ) const {
  return ( GoodCrateSlot(crate,slot) && crateslot[idx(crate,slot)] != nullptr);
}

inline Int_t THaEvData::GetRocLength(Int_t crate) const {
  assert(crate >= 0 && crate < Decoder::MAXROC);
  return rocdat[crate].len;
}

inline Int_t THaEvData::GetNumHits(Int_t crate, Int_t slot, Int_t chan) const {
  // Number hits in crate, slot, channel
  assert( GoodCrateSlot(crate,slot) );
  if( crateslot[idx(crate,slot)] != nullptr )
    return crateslot[idx(crate,slot)]->getNumHits(chan);
  return 0;
}

inline Int_t THaEvData::GetData(Int_t crate, Int_t slot, Int_t chan,
				Int_t hit) const {
  // Return the data in crate, slot, channel #chan and hit# hit
  assert( GoodIndex(crate,slot) );
  return crateslot[idx(crate,slot)]->getData(chan,hit);
}

inline Int_t THaEvData::GetNumRaw(Int_t crate, Int_t slot) const {
  // Number of raw words in crate, slot
  assert( GoodCrateSlot(crate,slot) );
  if( crateslot[idx(crate,slot)] != nullptr )
    return crateslot[idx(crate,slot)]->getNumRaw();
  return 0;
}

inline Int_t THaEvData::GetRawData(Int_t crate, Int_t slot, Int_t hit) const {
  // Raw words in crate, slot
  assert( GoodIndex(crate,slot) );
  return crateslot[idx(crate,slot)]->getRawData(hit);
}

inline Int_t THaEvData::GetRawData(Int_t crate, Int_t slot, Int_t chan,
				   Int_t hit) const {
  // Return the Rawdata in crate, slot, channel #chan and hit# hit
  assert( GoodIndex(crate,slot) );
  return crateslot[idx(crate,slot)]->getRawData(chan,hit);
}

inline Int_t THaEvData::GetRawData(Int_t i) const {
  // Raw words in evbuffer at location #i.
  assert( buffer && i >= 0 && i < GetEvLength() );
  return buffer[i];
}

inline Int_t THaEvData::GetRawData(Int_t crate, Int_t i) const {
  // Raw words in evbuffer within crate #crate.
  assert( crate >= 0 && crate < Decoder::MAXROC );
  Int_t index = rocdat[crate].pos + i;
  return GetRawData(index);
}

inline const UInt_t* THaEvData::GetRawDataBuffer(Int_t crate) const {
  // Direct access to the event buffer for the given crate,
  // e.g. for fast header word searches
  assert( crate >= 0 && crate < Decoder::MAXROC );
  Int_t index = rocdat[crate].pos;
  return buffer+index;
}

inline Bool_t THaEvData::InCrate(Int_t crate, Int_t i) const {
  // To tell if the index "i" poInt_ts to a word inside crate #crate.
  assert( crate >= 0 && crate < Decoder::MAXROC );
  // Used for crawling through whole event
  if (crate == 0) return (i >= 0 && i < GetEvLength());
  if (rocdat[crate].pos == 0 || rocdat[crate].len == 0) return false;
  return (i >= rocdat[crate].pos &&
	  i <= rocdat[crate].pos+rocdat[crate].len);
}

inline Int_t THaEvData::GetNumChan(Int_t crate, Int_t slot) const {
  // Get number of unique channels hit
  assert( GoodCrateSlot(crate,slot) );
  if( crateslot[idx(crate,slot)] != nullptr )
    return crateslot[idx(crate,slot)]->getNumChan();
  return 0;
}

inline Int_t THaEvData::GetNextChan(Int_t crate, Int_t slot,
				    Int_t index) const {
  // Get list of unique channels hit (indexed by index=0,getNumChan()-1)
  assert( GoodIndex(crate,slot) );
  assert( index >= 0 && index < GetNumChan(crate,slot) );
  return crateslot[idx(crate,slot)]->getNextChan(index);
}

inline
Bool_t THaEvData::IsPhysicsTrigger() const {
  return ((event_type > 0) && (event_type <= Decoder::MAX_PHYS_EVTYPE));
}

inline
Bool_t THaEvData::IsScalerEvent() const {
  // Either 'event type 140' or events with the synchronous readout
  // of scalers (roc11, etc).
  // Important: A scaler event can also be a physics event.
  return (event_type == Decoder::SCALER_EVTYPE || evscaler == 1);
}

inline
Bool_t THaEvData::IsPrestartEvent() const {
  return (event_type == Decoder::PRESTART_EVTYPE);
}

inline
Bool_t THaEvData::IsEpicsEvent() const {
  return (event_type == fEpicsEvtType);
}

inline
Bool_t THaEvData::IsPrescaleEvent() const {
  return (event_type == Decoder::TS_PRESCALE_EVTYPE ||
	  event_type == Decoder::PRESCALE_EVTYPE);
}

inline
Bool_t THaEvData::IsSpecialEvent() const {
  return ( (event_type == Decoder::DETMAP_FILE) ||
	   (event_type == Decoder::TRIGGER_FILE) );
}

inline
Bool_t THaEvData::HelicityEnabled() const
{
  // Test if helicity decoding enabled
  return TestBit(kHelicityEnabled);
}

inline
Bool_t THaEvData::ScalersEnabled() const
{
  // Test if scaler decoding enabled
  return TestBit(kScalersEnabled);
}

// Dummy versions of EPICS data access functions. These will always fail
// in debug mode unless IsLoadedEpics is changed. This is by design -
// clients should never try to retrieve data that are not loaded.
inline
double THaEvData::GetEpicsData(const char* /*tag*/, Int_t /*event*/ ) const
{
  assert(IsLoadedEpics("") && fgAllowUnimpl);
  return kBig;
}

inline
double THaEvData::GetEpicsTime(const char* /*tag*/, Int_t /*event*/ ) const
{
  assert(IsLoadedEpics("") && fgAllowUnimpl);
  return kBig;
}

inline
TString THaEvData::GetEpicsString(const char* /*tag*/,
				  Int_t /*event*/ ) const
{
  assert(IsLoadedEpics("") && fgAllowUnimpl);
  return TString("");
}

#endif
