
cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)

if(${ESP_PLATFORM})
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Include Smooth as a component
set(EXTRA_COMPONENT_DIRS
         externals/smooth/smooth_component
         components/runnables)

project(PDG1 1.0)

else()
   # Empty project when not building for ESP (i.e. when loading the project into an IDE with already configured tool chains for native Linux)
endif()