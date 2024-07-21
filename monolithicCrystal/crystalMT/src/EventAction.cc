#include "EventAction.hh"
#include "G4Event.hh"
#include "G4RunManager.hh"
#include "G4SDManager.hh"
#include "G4PhysicalConstants.hh"
#include "G4SystemOfUnits.hh"
#include "GlobalPars.hh"
#include "SensorSD.hh"

#include <iostream>
#include <fstream>
#include <mutex>
#include <atomic>

//std::mutex EventAction::sensorPosFileMutex;
std::mutex EventAction::sensorDataFileMutex;
std::mutex EventAction::iSensorDataFileMutex;

std::ofstream EventAction::sensorDataFile("sensor_data.csv");
//std::ofstream EventAction::sensorPosFile("sensor_pos.csv");
std::ofstream EventAction::iSensorDataFile("integrated_sensor_data.csv");
std::atomic<bool> EventAction::sensorDataFileWritten(false);
std::atomic<bool> EventAction::isensorDataFileWritten(false);


EventAction::EventAction()
    : G4UserEventAction()
{

  //sensorPosFile.open(sensorPosFileName);
  //sensorPosFile << "sensor_id, sensor_x, sensor_y, sensor_z\n";
  // if (!sensorPosFileWritten.exchange(true))
  //   {
  //     sensorPosFile << sensor_id <<  "," << x << "," << y << "," << z <<"\n";
  //   }
  
  //sensorDataFile.open(sensorDataFileName);
  if (!sensorDataFileWritten.exchange(true))
    {
       sensorDataFile << "event,sensor_id,time,charge\n";
    }
 

  //integratedSensorDataFile.open(integratedSensorDataFileName);
  if (!isensorDataFileWritten.exchange(true))
    {
         iSensorDataFile << "event,sensor_id,amplitude\n";
    }

}


EventAction::~EventAction()
{
  // sensorPosFile.close();
  // sensorDataFile.close();
  // integratedSensorDataFile.close();
}


void EventAction::BeginOfEventAction(const G4Event* event)
{
  fEventNumber = event->GetEventID();
}



void EventAction::EndOfEventAction(const G4Event* event)
{

  G4SDManager* sdmgr = G4SDManager::GetSDMpointer();
  G4HCtable* hct = sdmgr->GetHCtable();

  //G4cout << "I have " << hct->entries() << "entries" << G4endl;
  
   // Loop through the hits collections
  for (auto i=0; i<hct->entries(); i++) {

    // Collection are identified univocally (in principle) using
    // their id number, and this can be obtained using the collection
    // and sensitive detector names.
    
    auto hcname = hct->GetHCname(i);
    auto sdname = hct->GetSDname(i);
    auto hcid = sdmgr->GetCollectionID(sdname+"/"+hcname);
    auto hce  = event->GetHCofThisEvent();

    // Fetch collection using the id number
    G4VHitsCollection* hits = hce->GetHC(hcid);

    //    if (hcname == SensorSD::GetCollectionUniqueName())
    if (hcname == GlobalPars::Instance()->gSDCollection)
      {
        StoreSensorHits(hits);
      }
    else
      {
        G4String msg =
          "Collection of hits '" + sdname + "/" + hcname
          + "' is of an unknown type and will not be stored.";
        G4Exception("[PersistencyManager]", "StoreHits()", JustWarning, msg);
      }
  }
      
}


void EventAction::StoreSensorHits(G4VHitsCollection* hc)
{

  SensorHitsCollection* hits = dynamic_cast<SensorHitsCollection*>(hc);
  if (!hits) return;

  auto sdname = hits->GetSDname();

  std::map<G4String, G4double>::const_iterator sensdet_it = fSensDetBin.find(sdname);
  
  if (sensdet_it == fSensDetBin.end())
    {
      for (auto j=0; j<hits->entries(); j++)
        {
          SensorHit* hit = dynamic_cast<SensorHit*>(hits->GetHit(j));
          if (!hit) continue;
          auto bin_size = hit->fBinSize;
          fSensDetBin[sdname] = bin_size;
          break;
        }
    }

  for (auto i=0; i<hits->entries(); i++)
    {
      SensorHit* hit = dynamic_cast<SensorHit*>(hits->GetHit(i));
      if (!hit) continue;

      auto xyz = hit->fSensorPos;
      auto binsize = hit->fBinSize;

      //G4cout << "hit =" << i << " binsize = " << binsize << G4endl; 

      const std::map<G4double, G4int>& wvfm = hit->GetPhotonHistogram();
      std::map<G4double, G4int>::const_iterator it;
      std::vector< std::pair<unsigned int,float> > data;
      G4double amplitude = 0.;

      for (it = wvfm.begin(); it != wvfm.end(); ++it)
        {
          //G4cout << "time =" << (*it).first << G4endl;
          
          unsigned int time_bin = (unsigned int)((*it).first/binsize + 0.5);
          unsigned int charge = (unsigned int)((*it).second + 0.5);

          data.push_back(std::make_pair(time_bin, charge));
          amplitude = amplitude + (*it).second;

         WriteSensorData(fEventNumber, (unsigned int)hit->fSensorID,
                         time_bin, charge);
        }

      WriteIntegratedSensorData(fEventNumber, (unsigned int)hit->fSensorID, amplitude);


      std::vector<G4int>::iterator pos_it =
        std::find(fSnsPosvec.begin(), fSnsPosvec.end(), hit->fSensorID);
      
    if (pos_it == fSnsPosvec.end())
      {
        //WriteSensorPos((unsigned int)hit->fSensorID,
        //             (float)xyz.x(), (float)xyz.y(), (float)xyz.z());
        
        fSnsPosvec.push_back(hit->fSensorID);
      }
    
    }        
}


// void EventAction::WriteSensorPos(unsigned int sensor_id, float x, float y, float z)
// {
//   std::lock_guard<std::mutex> guard(sensorPosFileMutex);
//   sensorPosFile << sensor_id <<  "," << x << "," << y << "," << z <<"\n";
//   // if (!sensorPosFileWritten.exchange(true))
//   //   {
//   //     sensorPosFile << sensor_id <<  "," << x << "," << y << "," << z <<"\n";
//   //   }
  
// }

void EventAction::WriteSensorData(int64_t evt_number, unsigned int sensor_id, unsigned int time_bin, unsigned int charge)
{

  std::lock_guard<std::mutex> guard(sensorDataFileMutex);
  // Write event data to the first file

  sensorDataFile << evt_number << "," << sensor_id << "," << time_bin << "," << charge <<"\n";
}

void EventAction::WriteIntegratedSensorData(int64_t evt_number, unsigned int sensor_id, double amplitude)
{
  std::lock_guard<std::mutex> guard(iSensorDataFileMutex);

    iSensorDataFile << evt_number << "," << sensor_id << "," << amplitude <<"\n";
}
