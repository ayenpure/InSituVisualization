#include <cmath>
#include <iostream>
#include <string.h>
#include <VisItControlInterface_V2.h>
#include <VisItDataInterface_V2.h>

#define NX 10
#define NY 10
#define NZ 10
#define PI 3.14159265
#define PERIOD 100

typedef struct
{
  int cycle;
  double time;
  int runMode;
  int done;
} SimulationData;

static float *meshx = new float[NX];
static float *meshy = new float[NY];
static float *meshz = new float[NZ];

static float *data = new float[NX*NY*NZ];
static int meshdims[] = {NX, NY,NZ};
static int meshaxes = 3;

float calculateVelocityMagnitude(float x, float y, float z, float t)
{
  return sin(PI*sqrt(pow(x, 2) + pow(y, 2) + pow(z, 2))+t);
}

void initMeshMetaData()
{
  int xindex, yindex, zindex;
  for(xindex = 0; xindex < NX; xindex++)
  {
    meshx[xindex] = xindex;
  }
  for(yindex = 0; yindex < NY; yindex++)
  {
    meshy[yindex] = yindex;
  }
  for(zindex = 0; zindex < NX; zindex++)
  {
    meshz[zindex] = zindex;
  }
  int dindex = 0;
  for(xindex =0; xindex < NX; xindex++)
    for(yindex = 0; yindex < NY; yindex++)
      for(zindex = 0; zindex < NZ; zindex++)
        data[dindex++] = calculateVelocityMagnitude(meshx[xindex], meshy[yindex], meshz[zindex], 0.0f);
}

visit_handle GetSimulationMesh(int domain, const char * name, void *simData)
{
  visit_handle mesh = VISIT_INVALID_HANDLE;
  if(VisIt_RectilinearMesh_alloc(&mesh) != VISIT_ERROR)
  {
    visit_handle xc, yc, zc;
    VisIt_VariableData_alloc(&xc);
    VisIt_VariableData_alloc(&yc);
    VisIt_VariableData_alloc(&zc);
    VisIt_VariableData_setDataF(xc, VISIT_OWNER_SIM, 1, meshdims[0], meshx);
    VisIt_VariableData_setDataF(yc, VISIT_OWNER_SIM, 1, meshdims[1], meshy);
    VisIt_VariableData_setDataF(zc, VISIT_OWNER_SIM, 1, meshdims[2], meshz);
    VisIt_RectilinearMesh_setCoordsXYZ(mesh, xc, yc, zc);
  }
  return mesh;
}

visit_handle GetSimulationVariable(int domain, const char *name, void *simData)
{
  visit_handle var = VISIT_INVALID_HANDLE;
  if(VisIt_VariableData_alloc(&var) == VISIT_OKAY)
  {
    VisIt_VariableData_setDataF(var, VISIT_OWNER_SIM, 1, NX*NY*NZ, data);
  }
  return var;
}

void simulateOneTimeStep(SimulationData *sim)
{
  /*simulate one time step*/
  sim->cycle++;
  sim->time += 0.1f;
  int dindex = 0;
  for(int xindex =0; xindex < NX; xindex++)
    for(int yindex = 0; yindex < NY; yindex++)
      for(int zindex = 0; zindex < NZ; zindex++)
        data[dindex++] = calculateVelocityMagnitude(meshx[xindex], meshy[yindex], meshz[zindex], static_cast<float>(sim->time));
  std::cerr << "Advanced one time step : " << sim->cycle << std::endl;
  VisItTimeStepChanged();
  VisItUpdatePlots();
}

void constructSimulationData(SimulationData *sim)
{
  /*construct simulation object*/
  sim->cycle = 0;
  sim->time = 0.0f;
  sim->runMode = 1;
  sim->done = false;
}

void ControlCommandCallback(const char *cmd, const char *args, void *simData)
{
  SimulationData *sim = (SimulationData *)simData;
  if(strcmp(cmd, "halt") == 0)
    sim->runMode = VISIT_SIMMODE_STOPPED;
  else if(strcmp(cmd, "step") == 0)
    simulateOneTimeStep(sim);
  else if(strcmp(cmd, "run") == 0)
    sim->runMode = VISIT_SIMMODE_RUNNING;
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

    visit_handle mesh3d;
    if(VisIt_MeshMetaData_alloc(&mesh3d) == VISIT_OKAY)
    {
      VisIt_MeshMetaData_setName(mesh3d, "mesh3d");
      VisIt_MeshMetaData_setMeshType(mesh3d, VISIT_MESHTYPE_RECTILINEAR);
      VisIt_MeshMetaData_setTopologicalDimension(mesh3d, 3);
      VisIt_MeshMetaData_setSpatialDimension(mesh3d, 3);
      VisIt_MeshMetaData_setXUnits(mesh3d, "cm");
      VisIt_MeshMetaData_setXUnits(mesh3d, "cm");
      VisIt_MeshMetaData_setYUnits(mesh3d, "cm");
      VisIt_MeshMetaData_setXLabel(mesh3d, "Width");
      VisIt_MeshMetaData_setYLabel(mesh3d, "Height");
      VisIt_MeshMetaData_setZLabel(mesh3d, "Length");
      VisIt_SimulationMetaData_addMesh(md, mesh3d);
    }

    visit_handle meshVar;
    if(VisIt_VariableMetaData_alloc(&meshVar) == VISIT_OKAY)
    {
      VisIt_VariableMetaData_setName(meshVar, "nodal");
      VisIt_VariableMetaData_setMeshName(meshVar, "mesh3d");
      VisIt_VariableMetaData_setType(meshVar, VISIT_VARTYPE_SCALAR);
      VisIt_VariableMetaData_setCentering(meshVar, VISIT_VARCENTERING_NODE);
      VisIt_SimulationMetaData_addVariable(md, meshVar);
    }
    /* Add commands to simulation */
    const char *cmd_names[] = {"halt", "step", "run"};
    for(int i = 0; i < sizeof(cmd_names)/sizeof(const char *); ++i)
    {
      visit_handle cmd = VISIT_INVALID_HANDLE;
      if(VisIt_CommandMetaData_alloc(&cmd) == VISIT_OKAY)
      {
        VisIt_CommandMetaData_setName(cmd, cmd_names[i]);
        VisIt_SimulationMetaData_addGenericCommand(md, cmd);
      }
    }
  }
  return md;
}

void mainLoop(SimulationData *sim)
{
  int blocking, visitState, err = 0;
  do
  {
    blocking = (sim->runMode == VISIT_SIMMODE_RUNNING) ? 0 : 1;
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
        /* Register command callback */
        VisItSetCommandCallback(ControlCommandCallback,(void*)sim);
        /*register data access callbacks*/
        VisItSetGetMetaData(GetSimulationMetaData, (void*)&sim);
        VisItSetGetMesh(GetSimulationMesh, (void*)&sim);
        VisItSetGetVariable(GetSimulationVariable, (void*)&sim);
      }
      else
        std::cerr << "VisIt failed to connect!!!" << std::cout;
    }
    else if(visitState == 2)
    {
      /*VisIt reporting to the engine*/
      sim->runMode = VISIT_SIMMODE_STOPPED;
      if(!VisItProcessEngineCommand())
      {
        /*disconnect in case of an error or closed connection*/
        VisItDisconnect();
        /*Start running again if VisIt closes*/
        sim->runMode = VISIT_SIMMODE_RUNNING;
      }
    }
  } while(!sim->done && err == 0);
}

int main()
{
  std::cout << "We be simulatin' all day!!!" << std::endl;
  SimulationData sim;
  initMeshMetaData();
  constructSimulationData(&sim);
  /*Set VisIt directory*/
  VisItSetDirectory("/home/abhishek/big/visit2_12_3.linux-x86_64");
  /*init environment variables*/
  VisItSetupEnvironment();
  /*instructions for VisIt to connect*/
  VisItInitializeSocketAndDumpSimFile("mysimulation", "InSituVis",
    "/home/abhishek/repositories/insitu/visitlibsim", NULL, NULL, NULL);
  //readInputDeck(sim);
  mainLoop(&sim);
  return 0;
}
