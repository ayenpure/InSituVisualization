cmake_minimum_required(VERSION 3.0)

project(mysimulation)

include("FindAscent.cmake")
include("FindConduit.cmake")
if(VTKM_DIR)
    include("FindVTKm.cmake")
    include("FindVTKh.cmake")
endif()

# setup the ascent & conduit include paths
include_directories(${ASCENT_INCLUDE_DIRS})
include_directories(${CONDUIT_INCLUDE_DIRS})

# create our example 
add_executable(mysimulation mysimulation.cxx 
               #open_simplex_noise.h open_simplex_noise.c
              )

# link to ascent
target_link_libraries(mysimulation ascent)

