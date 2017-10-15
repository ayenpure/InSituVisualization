cmake_minimum_required(VERSION 2.8)

SET(PROJ_NAME mysimulation)

project(${PROJ_NAME})

SET(LIBSIM_LIBRARIES /home/abhishek/big/visit2_12_3.linux-x86_64/2.12.3/linux-x86_64/libsim/V2/lib)
SET(LIBSIM_INCLUDE_DIRS /home/abhishek/big/visit2_12_3.linux-x86_64/2.12.3/linux-x86_64/libsim/V2/include)
link_directories(${LIBSIM_LIBRARIES})

add_executable(${PROJ_NAME} ${PROJ_NAME}.cxx)
include_directories(${LIBSIM_INCLUDE_DIRS})
target_link_libraries(${PROJ_NAME} PRIVATE simV2 simV2f ${CMAKE_DL_LIBS})
