cmake_minimum_required(VERSION 2.8.3)
project(manda_coverage)

add_compile_options(-std=c++11)

find_package(catkin REQUIRED COMPONENTS
  roscpp
  project11_transformations
  geographic_msgs
  marine_msgs
  message_generation
  actionlib_msgs
  actionlib
  geographic_visualization_msgs
  path_follower
)

find_path(EIGEN_INCLUDES Eigen/Core /usr/include/eigen3)

add_action_files(DIRECTORY action FILES manda_coverage.action)
generate_messages(DEPENDENCIES actionlib_msgs geographic_msgs)


catkin_package(
#  INCLUDE_DIRS include
#  LIBRARIES asv_helm
  CATKIN_DEPENDS roscpp message_runtime
#  DEPENDS system_lib
)

include_directories(
  lib_mandacov/include
  src/lib_geometry/
  src/lib_mbutil/
  ${catkin_INCLUDE_DIRS}
  ${EIGEN_INCLUDES}
)

#add_subdirectory(lib_mandacov)

file(GLOB MANDA_SRC
    "lib_mandacov/src/*.cpp"
    "lib_mandacov/include/lib_mandacov/*.h"
)

add_library(lib_mandacov SHARED ${MANDA_SRC})
target_link_libraries(lib_mandacov
   ${catkin_LIBRARIES}
)

file(GLOB NODE_SOURCES
    "src/*.cpp"
    "src/lib_mbutil/*.cpp"
    "src/lib_geometry/*.cpp"
)

add_executable(${PROJECT_NAME}_node ${NODE_SOURCES})

add_dependencies(${PROJECT_NAME}_node ${${PROJECT_NAME}_EXPORTED_TARGETS} ${catkin_EXPORTED_TARGETS} lib_mandacov)

target_link_libraries(${PROJECT_NAME}_node
    ${catkin_LIBRARIES}
    lib_mandacov
)

install(TARGETS ${PROJECT_NAME}_node
   ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
   LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
   RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
)

