cmake_minimum_required(VERSION 3.9)
project(linux84)

set(BINARY_INSTALL_DIR /usr/local/bin)
set(LIBRARY_INSTALL_DIR /usr/local/lib)
set(INCLUDE_INSTALL_DIR /usr/local/include)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CUDA_FOUND FALSE)

if(NOT NO_CUDA)
    find_package(CUDA)
endif()

if(CUDA_FOUND)
    message("INFO: Using CUDA version ${CUDA_VERSION}")
    add_definitions(-DHAVE_CUDA=1)
else()
    message("INFO: Building without CUDA support")
    add_definitions(-DHAVE_CUDA=0)
endif()

if(CUDA_FOUND)
    include_directories(${CUDA_INCLUDE_DIRS})
endif()


add_subdirectory(argon2-gpu)

if(CUDA_FOUND)
add_executable(linux84
        src/linux84/main.cpp
        include/linux84.h
        include/cudalinux84.h
        src/common/cudalinux84.cpp
        include/updater.h
        src/common/updater.cpp
        include/stats.h
        src/common/stats.cpp
        include/linuxdata.h
        src/common/linuxdata.cpp
        include/linuxsettings.h
        src/common/linuxsettings.cpp
        src/common/linux84.cpp)

target_link_libraries(linux84
        argon2-cuda
        -lboost_system
        -lssl
        -lcrypto
        -lgmp
        -lpthread
        -lcpprest
        -largon2
        )
endif()

add_executable(arionum-test
        src/test/main.cpp)

target_link_libraries(arionum-test
        -lgmp
        )
