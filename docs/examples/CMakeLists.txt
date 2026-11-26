# SPDX-License-Identifier: MIT
# docs/examples/CMakeLists.txt — the developer guide's compiled examples
#
# Each directory here holds one example that a guide page shows with @include
# or @snippet. It builds when BUILD_EXAMPLES=ON and runs as the ctest test
# docs.example.<directory> when BUILD_TESTS=ON, so a page cannot show code
# that no longer compiles or runs.
#
#   <directory>/main.cpp    the program; every *.cpp in the directory is built
#   <directory>/REQUIRES    optional: CMake options it needs, one per line
#                           (for example USE_NETCDF); skipped if any is off
#   <directory>/LIBRARIES   optional: extra targets to link, one per line
#                           (for example HydroCoupleTools)
#
# An example reads its own inputs through DOCS_EXAMPLE_SOURCE_DIR and writes
# into its working directory, build/docs-examples/<directory>/, which stays
# for review after the test runs.

file(GLOB example_dirs LIST_DIRECTORIES true CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/*")

foreach(dir IN LISTS example_dirs)
    if(NOT IS_DIRECTORY "${dir}" OR NOT EXISTS "${dir}/main.cpp")
        continue()
    endif()
    get_filename_component(name "${dir}" NAME)

    set(enabled TRUE)
    if(EXISTS "${dir}/REQUIRES")
        file(STRINGS "${dir}/REQUIRES" requirements)
        foreach(requirement IN LISTS requirements)
            if(NOT ${requirement})
                set(enabled FALSE)
            endif()
        endforeach()
    endif()
    if(NOT enabled)
        message(STATUS "Docs example ${name}: skipped (needs ${requirements})")
        continue()
    endif()

    set(target "docs_example_${name}")
    file(GLOB sources CONFIGURE_DEPENDS "${dir}/*.cpp")
    add_executable(${target} ${sources})
    target_link_libraries(${target} PRIVATE HydroCoupleSDK)
    if(EXISTS "${dir}/LIBRARIES")
        file(STRINGS "${dir}/LIBRARIES" libraries)
        target_link_libraries(${target} PRIVATE ${libraries})
    endif()
    target_compile_definitions(${target} PRIVATE
        DOCS_EXAMPLE_SOURCE_DIR="${dir}")
    hydrocouplesdk_colocate_runtime_dlls(${target})

    if(BUILD_TESTS)
        set(output "${CMAKE_BINARY_DIR}/docs-examples/${name}")
        file(MAKE_DIRECTORY "${output}")
        add_test(NAME docs.example.${name} COMMAND ${target}
                 WORKING_DIRECTORY "${output}")
        set_tests_properties(docs.example.${name} PROPERTIES
            LABELS docs-example TIMEOUT 300)
    endif()
endforeach()
