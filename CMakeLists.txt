cmake_minimum_required(VERSION 3.11)

project(cribbage-solitaire-solver LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	message("Clang detected")

	add_compile_options(-Wall -Wextra -pedantic -pedantic-errors
		-Wconversion -Wno-deprecated-declarations -Wuninitialized
		-Wold-style-cast -Wshadow -Wzero-as-null-pointer-constant
		-Wno-mismatched-tags -Wimplicit-fallthrough -Wno-pragmas
		-Wno-sign-conversion -Werror=implicit-fallthrough)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	message("GCC detected")

	add_compile_options(-Wall -Wextra -pedantic -pedantic-errors
		-Wconversion -Wno-deprecated-declarations -Wuninitialized
		-Wold-style-cast -Wshadow=compatible-local -Wzero-as-null-pointer-constant
		-Wno-mismatched-tags -Wimplicit-fallthrough -Wno-pragmas
		-Wsign-compare -Werror=implicit-fallthrough -Wsuggest-override
		-fconcepts-diagnostics-depth=10)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
	message("ICC detected")

	add_compile_options(-Wall -Wextra -pedantic -pedantic-errors
		-Wconversion -Wno-deprecated-declarations -Wuninitialized
		-Wold-style-cast -Wshadow -Wzero-as-null-pointer-constant
		-Wno-mismatched-tags -Wimplicit-fallthrough -Wno-pragmas)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	message("MSVC detected")

	add_compile_options(/W3)
endif()

if(CMAKE_BUILD_TYPE MATCHES "Debug")
#	add_compile_options(-fsanitize=undefined,address)
#	add_link_options(-fsanitize=undefined,address)
endif()

if(CMAKE_BUILD_TYPE MATCHES "Release")
	add_compile_options(-march=native -mtune=native)
	include(CheckIPOSupported)
	check_ipo_supported()
endif()

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)

add_executable(cribbage-solitaire-solver main.cpp)

# used to add non-installed libraries
include_directories(SYSTEM PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/kblib" ${EXTRA_INCLUDES}
	INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
	PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

install(TARGETS cribbage-solitaire-solver
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
