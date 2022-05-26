cmake_minimum_required(VERSION 3.5)

project(remccg)

set(PROTO_CXX_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(PROTO_CXX_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

# use clang++-6.0 so need to compile libFuzzer.a independentl
set(CMAKE_CXX_COMPILER clang++-6.0)
set(CMAKE_C_COMPILER clang-6.0)
set(CMAKE_CXX_FLAGS "-g -Wall -fPIC -std=c++11 -pthread")

set(FUZZING_FLAGS "")
set(FUZZING_FLAGS_BINARY "")
set(FUZZING_FLAGS_FUZZER "-fsanitize-coverage=edge,trace-pc-guard,indirect-calls,trace-cmp,trace-div,trace-gep -fsanitize=fuzzer")

# install libprotobuf-mutator first
include_directories("/usr/local/include/libprotobuf-mutator")

LINK_DIRECTORIES("xxxxx/ccg")
set(PBM_LIB_PATH /usr/local/lib/libprotobuf-mutator.a)
set(PBM_FUZZ_LIB_PATH /usr/local/lib/libprotobuf-mutator-libfuzzer.a)
set(LIB_PROTOBUF_MUTATOR_LIBRARIES ${PBM_LIB_PATH} ${PBM_FUZZ_LIB_PATH})


# compile protobuf
include(FindProtobuf)
find_package(Protobuf REQUIRED)
include_directories(${PROTOBUF_INCLUDE_DIR})

protobuf_generate_cpp(C_PROTO_SRCS C_PROTO_HDRS c_proto.proto)
#protobuf_generate_cpp(MUTATOR_TEST_PROTO_SRCS MUTATOR_TEST_PROTO_HDRS mutator_proto.proto)

add_library(c-proto ${C_PROTO_SRCS})

#add_library(mutator-proto ${MUTATOR_TEST_PROTO_SRCS})

# add ccg library
add_subdirectory(lib/ccg)


add_library(proto_to_c proto_to_c.cc util.cc)
target_link_libraries(proto_to_c
          c-proto
          ccg
          )

add_executable(remccg
		${LIBFUZZER_SOURCES}
              	src/remccg.cc
	        src/binary_format.cc
	        src/text_format.cc
	        src/utf8_fix.cc
              	src/util.cc
	      )
target_link_libraries(remccg
              #${LIB_LIBFUZZER_MUTATOR_PATH}
              c-proto
              proto_to_c
              protobuf-mutator
              ${PROTOBUF_LIBRARIES}
              ${LIB_PROTOBUF_MUTATOR_LIBRARIES}
              ccg
              )


set_property(TARGET remgcc
    PROPERTY COMPILE_FLAGS ${FUZZING_FLAGS}
    )
set_property(TARGET remgcc
    PROPERTY LINK_FLAGS ${FUZZING_FLAGS_BINARY}
    )

