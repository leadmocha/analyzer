#ifndef Podd_THaCrateMap_h_
#define Podd_THaCrateMap_h_

/////////////////////////////////////////////////////////////////////
//
//  THaCrateMap
//  Layout, or "map", of DAQ Crates.
//
//  THaCrateMap contains info on how the DAQ crates
//  are arranged in Hall A, i.e whether slots are
//  fastbus or vme, what the module types are, and
//  what header info to expect.  Probably nobody needs
//  to know about this except the author, and at present
//  an object of this class is a private member of the decoder.
//
//  author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////


#include "TString.h"
#include "Decoder.h"
#include <fstream>
#include <cstdio>  // for FILE
#include <cassert>
#include <iostream>

namespace Decoder {

class THaCrateMap {


 public:
     static const UInt_t MAXCHAN;
     static const UInt_t MAXDATA;

     explicit THaCrateMap( const char* db = "cratemap" ); // Construct uninitialized
     virtual ~THaCrateMap() = default;
     bool isFastBus(int crate) const;               // True if fastbus crate;
     bool isVme(int crate) const;                   // True if VME crate;
     bool isCamac(int crate) const;                 // True if CAMAC crate;
     bool isScalerCrate(int crate) const;           // True if a Scaler crate
     bool isBankStructure(int crate) const;         // True if modules in banks
     int getNslot(int crate) const;                 // Returns num occupied slots
     int getMinSlot(int crate) const;               // Returns min slot number
     int getMaxSlot(int crate) const;               // Returns max slot number

 // This class must inform the crateslot where the modules are.

     int getModel(int crate, int slot) const;       // Return module type
     int getHeader(int crate, int slot) const;      // Return header
     int getMask(int crate, int slot) const;        // Return header mask
     int getBank(int crate, int slot) const;        // Return bank number
     int getScalerCrate(int word) const;            // Return scaler crate if word=header
     const char* getScalerLoc(int crate) const;     // Return scaler crate location
     int setCrateType(int crate, const char* type); // set the crate type
     int setModel(int crate, int slot, UInt_t mod,
		  UInt_t nchan=MAXCHAN,
		  UInt_t ndata=MAXDATA);            // set the module type
     int setHeader(int crate, int slot, int head);  // set the header
     int setMask(int crate, int slot, int mask);    // set the header mask
     int setBank(int crate, int slot, int bank);    // set the bank
     int setScalerLoc(int crate, const char* location); // Sets the scaler location
     UInt_t getNchan(int crate, int slot) const;  // Max number of channels
     UInt_t getNdata(int crate, int slot) const;  // Max number of data words
     bool slotDone(int slot) const;                       // Used to speed up decoder
     bool crateUsed(int crate) const;               // True if crate is used
     bool slotUsed(int crate, int slot) const;      // True if slot in crate is used
     bool slotClear(int crate, int slot) const;     // Decide if not clear ea event
     void setSlotDone(int slot);                    // Used to speed up decoder
     void setSlotDone();                            // Used to speed up decoder
     void setUnused(int crate,int slot);            // Disables this crate,slot
     int init(TString the_map);                     // Initialize from text-block
     int init(ULong64_t time = 0);                  // Initialize by Unix time.
     int init( FILE* fi, const TString& fname );    // Initialize from given file
     void print(std::ostream& os = std::cout) const;

     static const Int_t CM_OK;
     static const Int_t CM_ERR;

     const char* GetName() const { return fDBfileName.Data(); }

 private:

     enum ECrateCode { kUnknown, kFastbus, kVME, kScaler, kCamac };

     TString fDBfileName;             // Database file name
     struct CrateInfo_t {           // Crate Information data descriptor
       TString crate_type;
       bool bank_structure;
       ECrateCode crate_code;
       Int_t nslot, minslot, maxslot;
       bool crate_used;
       bool slot_used[MAXSLOT], slot_clear[MAXSLOT];
       UInt_t model[MAXSLOT];
       Int_t header[MAXSLOT], headmask[MAXSLOT];
       Int_t bank[MAXSLOT];
       UInt_t nchan[MAXSLOT], ndata[MAXSLOT];
       TString scalerloc;
     } crdat[MAXROC];
     bool didslot[MAXSLOT];
     void incrNslot(int crate);
     void setUsed(int crate,int slot);
     void setClear(int crate,int slot,bool clear);
     int  SetModelSize(int crate, int slot, UInt_t model );

     ClassDef(THaCrateMap,0) // Map of modules in DAQ crates
};

//=============== inline functions ================================
inline
bool THaCrateMap::isFastBus(int crate) const
{
  assert( crate >= 0 && crate < MAXROC );
  return (crdat[crate].crate_code == kFastbus);
}

inline
bool THaCrateMap::isVme(int crate) const
{
  assert( crate >= 0 && crate < MAXROC );
  return  (crdat[crate].crate_code == kVME ||
	   crdat[crate].crate_code == kScaler );
}

inline
bool THaCrateMap::isCamac(int crate) const
{
  assert( crate >= 0 && crate < MAXROC );
  return (crdat[crate].crate_code == kCamac);
}

inline
bool THaCrateMap::isScalerCrate(int crate) const
{
  assert( crate >= 0 && crate < MAXROC );
  return (crdat[crate].crate_code == kScaler);
}

inline
bool THaCrateMap::isBankStructure(int crate) const
{
  assert( crate >= 0 && crate < MAXROC );
  return (crdat[crate].bank_structure);
}

inline
bool THaCrateMap::crateUsed(int crate) const
{
  assert( crate >= 0 && crate < MAXROC );
  return crdat[crate].crate_used;
}

inline
bool THaCrateMap::slotUsed(int crate, int slot) const
{
  assert( crate >= 0 && crate < MAXROC && slot >= 0 && slot < MAXSLOT );
  if( crate < 0 || crate >= MAXROC || slot < 0 || slot >= MAXSLOT )
    return false;
  return crdat[crate].slot_used[slot];
}

inline
bool THaCrateMap::slotClear(int crate, int slot) const
{
  assert( crate >= 0 && crate < MAXROC && slot >= 0 && slot < MAXSLOT );
  return crdat[crate].slot_clear[slot];
}

inline
int THaCrateMap::getModel(int crate, int slot) const
{
  assert( crate >= 0 && crate < MAXROC && slot >= 0 && slot < MAXSLOT );
  return crdat[crate].model[slot];
}

inline
int THaCrateMap::getMask(int crate, int slot) const
{
  assert( crate >= 0 && crate < MAXROC && slot >= 0 && slot < MAXSLOT );
  return crdat[crate].headmask[slot];
}

inline
int THaCrateMap::getBank(int crate, int slot) const
{
  assert( crate >= 0 && crate < MAXROC && slot >= 0 && slot < MAXSLOT );
  return crdat[crate].bank[slot];
}

inline
UInt_t THaCrateMap::getNchan(int crate, int slot) const
{
  assert( crate >= 0 && crate < MAXROC && slot >= 0 && slot < MAXSLOT );
  return crdat[crate].nchan[slot];
}

inline
UInt_t THaCrateMap::getNdata(int crate, int slot) const
{
  assert( crate >= 0 && crate < MAXROC && slot >= 0 && slot < MAXSLOT );
  return crdat[crate].ndata[slot];
}

inline
int THaCrateMap::getNslot(int crate) const
{
  assert( crate >= 0 && crate < MAXROC );
  return crdat[crate].nslot;
}

inline
const char* THaCrateMap::getScalerLoc(int crate) const
{
  assert( crate >= 0 && crate < MAXROC );
  return crdat[crate].scalerloc.Data();
}

inline
int THaCrateMap::getMinSlot(int crate) const
{
  assert( crate >= 0 && crate < MAXROC );
  return crdat[crate].minslot;
}

inline
int THaCrateMap::getMaxSlot(int crate) const
{
  assert( crate >= 0 && crate < MAXROC );
  return crdat[crate].maxslot;
}

inline
int THaCrateMap::getHeader(int crate, int slot) const
{
  assert( crate >= 0 && crate < MAXROC && slot >= 0 && slot < MAXSLOT );
  return crdat[crate].header[slot];
}

inline
void THaCrateMap::setUsed(int crate, int slot)
{
  assert( crate >= 0 && crate < MAXROC && slot >= 0 && slot < MAXSLOT );
  crdat[crate].crate_used = true;
  crdat[crate].slot_used[slot] = true;
}

inline
void THaCrateMap::setUnused(int crate, int slot)
{
  assert( crate >= 0 && crate < MAXROC && slot >= 0 && slot < MAXSLOT );
  crdat[crate].slot_used[slot] = false;
}

inline
void THaCrateMap::setClear(int crate, int slot, bool clear)
{
  assert( crate >= 0 && crate < MAXROC && slot >= 0 && slot < MAXSLOT );
  crdat[crate].slot_clear[slot] = clear;
}

inline
bool THaCrateMap::slotDone(int slot) const
{
  assert( slot >= 0 && slot < MAXSLOT );
  return didslot[slot];
}

inline
void THaCrateMap::setSlotDone(int slot)
{
  assert( slot >= 0 && slot < MAXSLOT );
  didslot[slot] = true;
}

inline
void THaCrateMap::setSlotDone()
{
  // initialize
  for( bool& slotflag : didslot ) {
    slotflag = false;
  }
}

}

#endif
