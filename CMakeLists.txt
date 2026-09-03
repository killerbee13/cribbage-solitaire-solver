cmake_minimum_required(VERSION 3.11)

project(cribbage-solitaire-solver LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(cribbage-solitaire-solver main.cpp)

# used to add non-installed libraries
include_directories(SYSTEM PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/kblib" ${EXTRA_INCLUDES}
	INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
	PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

install(TARGETS cribbage-solitaire-solver
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
