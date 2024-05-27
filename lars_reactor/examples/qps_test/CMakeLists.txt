# for each "src/x.cpp", generate target "x"
# file(GLOB_RECURSE all_srcs CONFIGURE_DEPENDS *.cpp)
set(Protobuf_PREFIX_PATH
  "/home/joytsing/makeInstall/protobuf3.21.12/include"
  "/home/joytsing/makeInstall/protobuf3.21.12/lib"
  "/home/joytsing/makeInstall/protobuf3.21.12/bin")

list(APPEND CMAKE_PREFIX_PATH "${Protobuf_PREFIX_PATH}")

find_package(Protobuf 3.21.12 REQUIRED)

include_directories(${Protobuf_INCLUDE_DIRS})
include_directories(./pb)

file(GLOB_RECURSE all_tests *.cpp)
file(GLOB_RECURSE all_src CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/src/*.cpp ./pb/echoMessage.pb.cc)

configure_file(${PROJECT_SOURCE_DIR}/conf/server.conf ./conf/server.conf COPYONLY)

foreach(v ${all_tests})
  string(REGEX MATCH "examples/qps_test/.*" relative_path ${v})
  string(REGEX REPLACE "examples/qps_test/" "" target_name ${relative_path})
  string(REGEX REPLACE ".cpp" "" target_name ${target_name})

  add_executable(${target_name} ${v} ${all_src})
  target_link_libraries(${target_name} PUBLIC ${Protobuf_LIBRARIES})
endforeach()