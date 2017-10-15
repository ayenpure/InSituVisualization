#include <iostream>
#include <VisItControlInterface_V2.h>
#include <VisItDataInterface_V2.h>

typedef struct
{
  int cycle;
  double time;
  int runMode;
  int done;
} SimulationData;

void simulateOneTimeStep(SimulationData &sim)
{
  /*simulate one time step*/
}

void constructSimulationData(SimulationData &sim)
{
  /*construct simulation object*/
  sim.cycle = 0;
  sim.time = 0.0f;
  sim.runMode = 1;
  sim.done = false;
}

void readInputDeck(SimulationData &sim)
{
}

void dumpVisualization(SimulationData &sim)
{
}

visit_handle GetSimulationMetaData(void *simData)
{
  visit_handle md = VISIT_INVALID_HANDLE;
  SimulationData *sim = (SimulationData*)simData;
  /*metadata with no variables*/
  if(VisIt_SimulationMetaData_alloc(&md) == VISIT_OKAY)
  {
    /*set the simulation state*/
    if(sim->runMode == VISIT_SIMMODE_STOPPED)
      VisIt_SimulationMetaData_setMode(md, VISIT_SIMMODE_STOPPED);
    else
      VisIt_SimulationMetaData_setMode(md, VISIT_SIMMODE_RUNNING);
    VisIt_SimulationMetaData_setCycleTime(md, sim->cycle, sim->time);

    visit_handle mesh2d;
    if(VisIt_MeshMetaData_alloc(&mesh2d) == VISIT_OKAY)
    {
    VisIt_MeshMetaData_setName(mesh2d, "mesh2d");
    VisIt_MeshMetaData_setMeshType(mesh2d, VISIT_MESHTYPE_RECTILINEAR);
    VisIt_MeshMetaData_setTopologicalDimension(mesh2d, 2);
    VisIt_MeshMetaData_setSpatialDimension(mesh2d, 2);
    VisIt_MeshMetaData_setXUnits(mesh2d, "cm");
    VisIt_MeshMetaData_setXUnits(mesh2d, "cm");
    VisIt_MeshMetaData_setXLabel(mesh2d, "Width");
    VisIt_MeshMetaData_setYLabel(mesh2d, "Height");
    VisIt_SimulationMetaData_addMesh(md, mesh2d);
    }
    visit_handle meshVar;
    if(VisIt_VariableMetaData_alloc(&meshVar) == VISIT_OKAY)
    {
      VisIt_VariableMetaData_setName(meshVar, "nodal");
      VisIt_VariableMetaData_setMeshName(meshVar, "mesh2d");
      VisIt_VariableMetaData_setType(meshVar, VISIT_VARTYPE_SCALAR);
      VisIt_VariableMetaData_setCentering(meshVar, VISIT_VARCENTERING_NODE);
      VisIt_SimulationMetaData_addVariable(md, meshVar);
    }
  }
  return md;
}

float meshx[] = {0, 1, 2, 3};
float meshy[] = {0, 1, 2, 3};
int meshdims[] = {4, 4, 1};
int meshaxes = 2;

visit_handle GetSimulationMesh(int domain, const char * name, void *simData)
{
  visit_handle mesh = VISIT_INVALID_HANDLE;
  if(VisIt_RectilinearMesh_alloc(&mesh) != VISIT_ERROR)
  {
    visit_handle xc, yc;
    VisIt_VariableData_alloc(&xc);
    VisIt_VariableData_alloc(&yc);
    VisIt_VariableData_setDataF(xc, VISIT_OWNER_SIM, 1, meshdims[0], meshx);
    VisIt_VariableData_setDataF(yc, VISIT_OWNER_SIM, 1, meshdims[1], meshy);
    VisIt_RectilinearMesh_setCoordsXY(mesh, xc, yc);
  }
  return mesh;
}

void mainLoop(SimulationData &sim)
{
  int blocking, visitState, err = 0;
  do
  {
    blocking = (sim.runMode == VISIT_SIMMODE_RUNNING) ? 0 : 1;

    visitState = VisItDetectInput(blocking, -1);
    /*depending on the result from VisItDetectInput.*/
    if(visitState <= -1)
    {
      std::cerr << "Simulation is in irrecoverable error state" << std::cout;
      err = 1;
    }
    else if(visitState == 0)
    {
      simulateOneTimeStep(sim);
    }
    else if(visitState == 1)
    {
      /*VisIt tries to connect to the simulation*/
      if(VisItAttemptToCompleteConnection())
      {
        std::cerr << "VisIt connected!!!" << std::cout;
        /*register data access callbacks*/
        VisItSetGetMetaData(GetSimulationMetaData, (void*)&sim);
        VisItSetGetMesh(GetSimulationMesh, (void*)&sim);
      }
      else
        std::cerr << "VisIt failed to connect!!!" << std::cout;
    }
    else if(visitState == 2)
    {
      /*VisIt reporting to the engine*/
      sim.runMode = VISIT_SIMMODE_STOPPED;
      if(!VisItProcessEngineCommand())
      {
        /*disconnect in case of an error or closed connection*/
        VisItDisconnect();
        /*Start running again if VisIt closes*/
        sim.runMode = VISIT_SIMMODE_RUNNING;
      }
    }
  } while(!sim.done && err == 0);
}

int main()
{
  std::cout << "We be simulatin' all day!!!" << std::endl;
  SimulationData sim;
  constructSimulationData(sim);
  /*Set VisIt directory*/
  VisItSetDirectory("/home/abhishek/big/visit2_12_3.linux-x86_64");
  /*init environment variables*/
  VisItSetupEnvironment();
  /*instructions for VisIt to connect*/
  VisItInitializeSocketAndDumpSimFile("mysimulation", "InSituVis",
    "/home/abhishek/repositories/insitu/visitlibsim", NULL, NULL, NULL);
  readInputDeck(sim);
  mainLoop(sim);
  return 0;
}
