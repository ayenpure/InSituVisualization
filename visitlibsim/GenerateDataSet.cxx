#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>

#define DIM 100
#define T 10

//Assume origin 0 ,0, 0
//Bounds in X 0..1
//Bounds in Y 0..1
//Bounds in Z 0..1

int main(int argc, char **argv)
{
  if(argc != 2)
  {
    std::cout << "Required input : generate <filename prefix>" << std::endl;
    exit(1);
  }
  std::string filePrefix = std::string(argv[1]);
  float spacing = 1./(DIM - 1);
  size_t numDataPoints = DIM*DIM*DIM;
  float *xLoc, *yLoc, *zLoc, *datapoints;
  xLoc = new float[DIM];
  yLoc = new float[DIM];
  zLoc = new float[DIM];
  datapoints = new float[numDataPoints];
  xLoc[0] = 0.0f;
  yLoc[0] = 0.0f;
  zLoc[0] = 0.0f;
  for(int index = 1; index < DIM; index++)
  {
    xLoc[index] = xLoc[index-1] + spacing;
    yLoc[index] = yLoc[index-1] + spacing;
    zLoc[index] = zLoc[index-1] + spacing;
  }
  long sliceSize = DIM*DIM;
  long rowSize = DIM;
  for(int xind = 0; xind < DIM; xind++)
   for(int yind = 0; yind < DIM; yind++) 
     for(int zind = 0; zind < DIM; zind++)
     {
       datapoints[zind*sliceSize + yind*rowSize + xind] = 
         sin(sqrt(pow(xLoc[xind],2)) + pow(yLoc[yind],2) + pow(zLoc[zind],2));
     }
  FILE *fp = fopen(filePrefix.c_str(), "wb");
  fwrite((void *)datapoints, sizeof(float), DIM*DIM*DIM, fp);
  fclose(fp);
  std::string headerName = filePrefix.append(".bov");
  std::ofstream header;
  header.open(headerName.c_str());
  header << "DATA_FILE:" << filePrefix << std::endl;
  header << "DATA_SIZE: 100 100 100" << std::endl;
  header << "DATA_FORMAT : FLOAT" << std::endl;
  header << "VARIABLE : sineFunction" << std::endl;
  header << "BRICK_ORIGIN : 0. 0. 0." << std::endl;
  header << "BRICK_SIZE : 1. 1. 1." << std::endl;
  header.close();
  return 0;
}
