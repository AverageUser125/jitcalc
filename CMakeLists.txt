cmake_minimum_required(VERSION 3.20)
# delete .out/CmakeFiles folder to make this work
option(PRODUCTION_BUILD "Make this a production build" OFF)

# FIXME: just using MSVC macro doesn't work for me
set(USING_MSVC WIN32)

# project name
set(PROJECT_NAME "jitcalc")

# Set the Compiler
# set(CMAKE_CXX_COMPILER /usr/bin/g++)

# set to C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CXX_STANDARD 17)
set(CXX_STANDARD_REQUIRED ON)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_WAYLAND TRUE CACHE BOOL "" FORCE)
set(ZLIB_FORCE_STATIC TRUE CACHE BOOL "" FORCE)
set(LLVM_USE_STATIC_LIBS TRUE CACHE BOOL "" FORCE)

# Set the CMP0069 policy, because warning annoying
if(POLICY CMP0069)
    cmake_policy(SET CMP0069 NEW)
endif()

set(LLVM_DIR "D:/apps/LLVM/build/lib/cmake/llvm")
find_package(LLVM REQUIRED CONFIG)

# If LLVM is found, include the required directories and link the libraries
if(LLVM_FOUND)
    llvm_map_components_to_libnames(LLVM_LIBS support core native mcjit asmparser)
    message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
    message(STATUS "Found LLVM: ${LLVM_VERSION}")
    message(STATUS "LLVM Include Directories: ${LLVM_INCLUDE_DIRS}")
    message(STATUS "LLVM Libraries: ${LLVM_LIBS}")

    include_directories(${LLVM_INCLUDE_DIRS})
    add_definitions(${LLVM_DEFINITIONS})
else()
    message(FATAL_ERROR "LLVM not found!")
endif()

if(${USING_MSVC})
    add_compile_options(/W3)
    add_compile_options(/arch:AVX2 /GA)
    add_compile_options(/wd4624  # Disable the warning for deleted destructors
                        /wd4533  # allow goto to skip variable decleration
                        /wd4244  # implicit conversion from return
                        /wd4267  # implicit conversion
                        /wd4273  # multiple definitions with different dllimport
                        /wd4146  # unary stuff
                        /wd4200  # arena.h use non-static size memberm which is a C and not CXX feature
                        /wd6323  # Use of arithmatic with boolean
                        /wd6031  # ignoring return values
                        /wd28125 # try-excpet for windows functions?
                        /wd6393  # array with 365 is not enough for leap years
                        /wd6326  # comparison of constants
    )
    add_compile_options(/permissive-)
    # Disable CRT secure warnings
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    # Show all warnings, just nice to have
    add_compile_options(-Wall -Wextra)
    add_compile_options("-Wno-unused-parameter" "-Wno-unused-variable" "-Wno-missing-field-initializers" "-Wno-sign-compare")
endif()

project("${PROJECT_NAME}")

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glfw")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glad")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glm")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/imgui")

# Add source files recursively from the src directory
file(GLOB_RECURSE MY_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

if(WIN32)
    # set(CMAKE_AUTORCC ON)
    set(MY_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/resources/icon.rc" ${MY_SOURCES})
endif()

message(STATUS ${MY_SOURCES})
# Add source to this project's executable
add_executable(${PROJECT_NAME} ${MY_SOURCES})

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)

if(PRODUCTION_BUILD)
    # setup the ASSETS_PATH macro to be in the root folder of your exe
    target_compile_definitions(${PROJECT_NAME} PUBLIC RESOURCES_PATH="./resources/") 
    # remove the option to debug asserts.
    target_compile_definitions(${PROJECT_NAME} PUBLIC PRODUCTION_BUILD=1) 
else()
    # This is useful to get an ASSETS_PATH in your IDE during development
    target_compile_definitions(${PROJECT_NAME} PUBLIC RESOURCES_PATH="${CMAKE_CURRENT_SOURCE_DIR}/resources/")
    target_compile_definitions(${PROJECT_NAME} PUBLIC PRODUCTION_BUILD=0) 
endif()

if(${USING_MSVC})
    target_compile_options(${PROJECT_NAME} PRIVATE "/analyze")
	target_compile_definitions("${CMAKE_PROJECT_NAME}" PUBLIC _CRT_SECURE_NO_WARNINGS)
	set_target_properties("${CMAKE_PROJECT_NAME}" PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup") #no console
else()
    target_compile_options(${PROJECT_NAME} PRIVATE 
    "-fanalyzer" 
    "-Wno-analyzer-use-of-uninitialized-value" # platform::getTypedInput() raises the unintalized every time
    )
endif()

target_compile_definitions(${PROJECT_NAME} PUBLIC GLFW_INCLUDE_NONE) 
# add .h and .hpp files
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/")

if(WIN32)
    target_link_libraries(${PROJECT_NAME}
        ${LLVM_LIBS}
        glfw
        glad
        glm
        imgui
        Dwmapi.lib
)
else()
    target_link_libraries(${PROJECT_NAME}
        ${LLVM_LIBS}
        glfw
        glad
        glm
        imgui
)
endif()
# Specify the output directory for the build (/bin directory)
# set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
# Specify the output directory for the build (/build/bin directory)
# set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")