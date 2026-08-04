cmake_minimum_required(VERSION 3.20)
project(cp_data_structures LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

enable_testing()

file(GLOB DS_DIRS LIST_DIRECTORIES true "${CMAKE_SOURCE_DIR}/*")

foreach(dir ${DS_DIRS})
    if(IS_DIRECTORY "${dir}")
        get_filename_component(ds_name "${dir}" NAME)

        if(NOT ds_name MATCHES "^\\.|^build$|^cmake-build")
            set(stress_file "${dir}/stress_test.cpp")

            if(EXISTS "${stress_file}")
                file(GLOB headers "${dir}/*.hpp")
                add_executable(stress_${ds_name} "${stress_file}" ${headers})
                target_include_directories(stress_${ds_name} PRIVATE "${dir}")
                add_test(NAME ${ds_name}_stress COMMAND stress_${ds_name})
                set_tests_properties(${ds_name}_stress PROPERTIES TIMEOUT 60)
            endif()
        endif()
    endif()
endforeach()