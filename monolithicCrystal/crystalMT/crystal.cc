// ********************************************************************
/// \file apacheDemo.cc
/// \brief Main program of the apache example

#include "DetectorConstruction.hh"
#include "ActionInitialization.hh"

#include "G4RunManagerFactory.hh"
#include "G4SteppingVerbose.hh"
#include "G4UImanager.hh"
#include "QBBC.hh"
#include "G4OpticalPhysics.hh"
#include "G4VisExecutive.hh"
#include "G4UIExecutive.hh"
#include "Randomize.hh"
#include "GlobalPars.hh"
#include "PrimaryGeneratorMessenger.hh"
#include "HistogramManager.hh"


//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

int main(int argc,char** argv)
{
  // Detect interactive mode (if no arguments) and define UI session
  //
  G4UIExecutive* ui = nullptr;
  if ( argc == 1 ) { ui = new G4UIExecutive(argc, argv); }

  // Optionally: choose a different Random engine...
  // G4Random::setTheEngine(new CLHEP::MTwistEngine);

  //use G4SteppingVerboseWithUnits
  G4int precision = 4;
  G4SteppingVerbose::UseBestUnit(precision);

  // Construct the default run manager
  //
  auto runManager =
    G4RunManagerFactory::CreateRunManager(G4RunManagerType::Default);

  // Set mandatory initialization classes

  // Set the name of the sensor hit collection (arbitrary) in globals
  GlobalPars::Instance()->gSDCollection = "SensorHitsCollection";

  
  // Book histograms.

    HistogramManager::Instance()->CreateHistogram("PrimaryParticleSpectrum_nm", 80, 0.0, 800.0);
  
  // Detector construction
  runManager->SetUserInitialization(new DetectorConstruction());

  // Invoke here the PrimaryGeneratorMessenger, so that values can be
  // passed to globals (and thus accepted by the PrimaryGenerator constructor)
  
  PrimaryGeneratorMessenger* pgMessenger = new PrimaryGeneratorMessenger();
  
  // Physics list
  auto physicsList = new QBBC;
  physicsList->SetVerboseLevel(0);
  auto opticalPhysics = new G4OpticalPhysics();

  physicsList->RegisterPhysics(opticalPhysics);
  runManager->SetUserInitialization(physicsList);


  // User action initialization
  runManager->SetUserInitialization(new ActionInitialization);


  // Initialize visualization with the default graphics system
  auto visManager = new G4VisExecutive(argc, argv);
  visManager->Initialize();

  // Get the pointer to the User Interface manager
  auto UImanager = G4UImanager::GetUIpointer();

  // Process macro or start UI session
  //
  if ( ! ui ) {
    // batch mode
    G4String command = "/control/execute ";
    G4String fileName = argv[1];
    UImanager->ApplyCommand(command+fileName);
  }
  else {
    // interactive mode
    UImanager->ApplyCommand("/control/execute init_vis.mac");
    ui->SessionStart();
    delete ui;
  }


  // Write histograms to file
  HistogramManager::Instance()->WriteHistograms("histograms.txt");
  
  delete visManager;
  delete runManager;
  delete pgMessenger;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo.....
