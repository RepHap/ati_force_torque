cmake_minimum_required(VERSION 2.8.3)
project(ati_force_torque)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")
message("CMAKE_MODULE_PATH: ${CMAKE_MODULE_PATH}")

find_package(catkin REQUIRED COMPONENTS
  cmake_modules
  #cob_generic_can
  dynamic_reconfigure
  gazebo_ros
  geometry_msgs
  iirob_filters
  message_generation
  roscpp
  roslaunch
  rosparam_handler
  rospy
  std_msgs
  std_srvs
  teleop_twist_joy
  tf2
  tf2_geometry_msgs
  tf2_ros
  trajectory_msgs
  visualization_msgs
  realtime_tools
  filters
  force_torque_sensor
  pluginlib
  #libmodbus
  #NIDAQmx
)

# <Program Files (x86)>\National Instruments\Shared\ExternalCompilerSupport\C\lib64\msvc

find_package(NIDAQmx REQUIRED)
message("NIDAQmx_INCLUDE_DIRS ${NIDAQmx_INCLUDE_DIRS}")
message("NIDAQmx_LIBRARIES ${NIDAQmx_LIBRARIES}")
if(NOT NIDAQmx_FOUND)
  message(FATAL_ERROR "Could not find NIDAQmx")
endif()
include_directories(${NIDAQmx_INCLUDE_DIRS})

find_package(Eigen3)

if(NOT EIGEN3_FOUND)
  # Fallback to cmake_modules
  find_package(cmake_modules REQUIRED)
  find_package(Eigen REQUIRED)
else()
  set(Eigen_INCLUDE_DIRS ${EIGEN3_INCLUDE_DIR})
  set(Eigen_LIBRARIES ${EIGEN3_LIBRARIES})
endif()

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" Cpp11CompilerFlag)
if (Cpp11CompilerFlag)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

add_definitions(-DDYNAMIC_RECONFIGURE_FOUND)

add_service_files(
  FILES
  CalculateAverageMasurement.srv
  CalculateSensorOffset.srv
  DiagnosticVoltages.srv
)

generate_messages(
  DEPENDENCIES
  geometry_msgs
  std_msgs
)

###################################
## catkin specific configuration ##
###################################
catkin_package(
 INCLUDE_DIRS common/include include
 CATKIN_DEPENDS geometry_msgs iirob_filters message_runtime roscpp std_msgs std_srvs rospy tf2 tf2_ros tf2_geometry_msgs trajectory_msgs realtime_tools filters force_torque_sensor pluginlib #cob_generic_can libmodbus
 DEPENDS Eigen
 LIBRARIES ${PROJECT_NAME} ${PROJECT_NAME}_sensor_hw
)

###########
## Build ##
###########

include_directories(
  common/include
  include
  ${catkin_INCLUDE_DIRS}
  ${Eigen_INCLUDE_DIRS}
)

#add_library(${PROJECT_NAME} common/src/ForceTorqueCtrl.cpp)
#add_dependencies(${PROJECT_NAME}
#  ${PROJECT_NAME}_gencfg  # For dynamic_reconfigure
#  ${PROJECT_NAME}_generate_messages_cpp
#  ${PROJECT_NAME}_genparam # For rosparam_handler
#  )
#target_link_libraries(${PROJECT_NAME} ${catkin_LIBRARIES} ${Eigen_LIBRARIES})

include_directories(
  3rdparty/ATICombinedDAQFT
)
add_library(ATICombinedDAQFT 
  3rdparty/ATICombinedDAQFT/ATICombinedDAQFT.cpp
  3rdparty/ATICombinedDAQFT/ATIDAQHardwareInterface.cpp
  3rdparty/ATICombinedDAQFT/dom.c
  3rdparty/ATICombinedDAQFT/expatls.c
  # 3rdparty/ATICombinedDAQFT/events.c # unused
  3rdparty/ATICombinedDAQFT/ftconfig.c
  3rdparty/ATICombinedDAQFT/ftrt.c
  3rdparty/ATICombinedDAQFT/node.c
  3rdparty/ATICombinedDAQFT/stack.c
  3rdparty/ATICombinedDAQFT/Stdafx.cpp
  3rdparty/ATICombinedDAQFT/xmlparse.c
  3rdparty/ATICombinedDAQFT/xmlrole.c
  3rdparty/ATICombinedDAQFT/xmltok.c
  # 3rdparty/ATICombinedDAQFT/xmltok_impl.c # included by xmltoc.c
  # 3rdparty/ATICombinedDAQFT/xmltok_ns.c # included by xmltoc.c
)
target_link_libraries(ATICombinedDAQFT ${NIDAQmx_LIBRARIES})

add_library(${PROJECT_NAME}_sensor_hw common/src/ati_force_torque_hw_nidaqmx.cpp) # common/src/ati_force_torque_hw_rs485.cpp common/src/ati_force_torque_hw_can.cpp)
target_link_libraries(${PROJECT_NAME}_sensor_hw ${catkin_LIBRARIES} ${Eigen_LIBRARIES} ATICombinedDAQFT)# ${Libmodbus_LIBRARIES})

add_executable(${PROJECT_NAME}_node src/force_torque_sensor_node.cpp)
add_dependencies(${PROJECT_NAME}_node ${PROJECT_NAME}_generate_messages_cpp)
target_link_libraries(${PROJECT_NAME}_node ${catkin_LIBRARIES} ${Eigen_LIBRARIES} ${PROJECT_NAME}_sensor_hw )

add_executable(${PROJECT_NAME}_config src/force_torque_sensor_config.cpp)
target_link_libraries(${PROJECT_NAME}_config ${catkin_LIBRARIES} ${Eigen_LIBRARIES} ${PROJECT_NAME}_sensor_hw ) # ${PROJECT_NAME} 
add_dependencies(${PROJECT_NAME}_config ${catkin_LIBRARIES} ${Eigen_LIBRARIES})# ${PROJECT_NAME}_generate_messages_cpp) # ${PROJECT_NAME} 

#############
## Install ##
#############
install(TARGETS ${PROJECT_NAME}_node ${PROJECT_NAME}_config # ${PROJECT_NAME} 
  ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
)

install(DIRECTORY common/include ros/include
  DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION}
  FILES_MATCHING PATTERN "*.h"
  PATTERN ".svn" EXCLUDE
)

install(DIRECTORY launch config description
  DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}
)

install(FILES ati_fts_plugin.xml
DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION})

#############
## Tests ##
#############

roslaunch_add_file_check(launch)
