cmake_minimum_required(VERSION 3.13)

# Set the name and the supported language of the project
PROJECT(desktop-native C)

if (UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK REQUIRED gtk+-3.0)
    include_directories(${GTK_INCLUDE_DIRS})
    link_directories(${GTK_LIBRARY_DIRS})
    add_definitions(${GTK_CFLAGS_OTHER})
    set(PLATFORM_LIBS ${GTK_LIBRARIES})
    file(GLOB PLATFORM_SRC "src/desktop_linux.c")
endif()

if(APPLE)
#    # for MacOS X or iOS, watchOS, tvOS (since 3.10.3)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ObjC++ -fno-exceptions")
#    set(CMAKE_XCODE_ATTRIBUTE_GCC_ENABLE_CPP_EXCEPTIONS "NO")
#        set(CMAKE_XCODE_ATTRIBUTE_OTHER_CFLAGS "-ObjC++")
#        set(CMKAE_XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET "10.8")
##    set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
#    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework AppKit -framework Foundation")
    set(PLATFORM_LIBS "-framework AppKit -framework Foundation")
    file(GLOB PLATFORM_SRC "src/desktop_apple.m")
endif()


# Essential include files to build a node addon,
# you should add this line in every CMake.js based project.
include_directories(${CMAKE_JS_INC} "node_modules/napi-macros")

# Build a shared library named after the project from the files in `src/`
file(GLOB COMMON_SRC "src/desktop.c" "src/common.h")
set(SOURCE_FILES ${COMMON_SRC} ${PLATFORM_SRC})
add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})

# Gives our library file a .node extension without any "lib" prefix
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")

# Essential include files to build a node addon,
# You should add this line in every CMake.js based project
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_JS_INC})

# Essential library files to link to a node addon
# You should add this line in every CMake.js based project
target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB} ${PLATFORM_LIBS})
