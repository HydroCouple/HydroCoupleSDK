# SPDX-License-Identifier: MIT
# examples/CMakeLists.txt — runnable HydroCoupleSDK examples
#
# Built in-tree when BUILD_EXAMPLES=ON. Each example directory also carries
# its own standalone CMakeLists.txt so it can be configured against an
# *installed* SDK (find_package(HydroCoupleSDK)) — the acceptance test that
# the install export is complete.

add_executable(serial_coupling serial_coupling/main.cpp)
target_link_libraries(serial_coupling PRIVATE HydroCoupleSDK)
hydrocouplesdk_colocate_runtime_dlls(serial_coupling)

# Composition documents live beside the binary so it runs from the build dir.
add_custom_command(TARGET serial_coupling POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/serial_coupling/composition.yaml
        ${CMAKE_CURRENT_SOURCE_DIR}/serial_coupling/composition.json
        $<TARGET_FILE_DIR:serial_coupling>
)

# Reopens a completed run with no model components linked — the analysis
# and visualization case the run manifest exists for.
add_executable(analysis_reopen analysis_reopen/main.cpp)
target_link_libraries(analysis_reopen PRIVATE HydroCoupleSDK)
hydrocouplesdk_colocate_runtime_dlls(analysis_reopen)

if(USE_MPI)
    add_executable(distributed_coupling distributed_coupling/main.cpp)
    target_link_libraries(distributed_coupling
        PRIVATE HydroCoupleSDK MPI::MPI_CXX)
    hydrocouplesdk_colocate_runtime_dlls(distributed_coupling)
endif()

# The developer guide's coupling tutorial (chapter 4) runs these two in
# sequence; as tests they keep its commands and output current. The run is
# written to build/docs-examples/serial_coupling/ and stays for review.
if(BUILD_TESTS)
    set(serial_coupling_output "${CMAKE_BINARY_DIR}/docs-examples/serial_coupling")
    add_test(NAME examples.serial_coupling
             COMMAND serial_coupling composition.json "${serial_coupling_output}"
             WORKING_DIRECTORY $<TARGET_FILE_DIR:serial_coupling>)
    set_tests_properties(examples.serial_coupling PROPERTIES
        FIXTURES_SETUP serial_coupling_run LABELS docs-example)
    add_test(NAME examples.analysis_reopen
             COMMAND analysis_reopen "${serial_coupling_output}/run.json")
    set_tests_properties(examples.analysis_reopen PROPERTIES
        FIXTURES_REQUIRED serial_coupling_run LABELS docs-example)
    # The YAML document initializes the same composition (yaml-cpp builds only).
    if(HYDROCOUPLESDK_HAVE_YAML_CPP)
        add_test(NAME examples.serial_coupling_yaml
                 COMMAND serial_coupling composition.yaml
                         "${CMAKE_BINARY_DIR}/docs-examples/serial_coupling_yaml"
                 WORKING_DIRECTORY $<TARGET_FILE_DIR:serial_coupling>)
        set_tests_properties(examples.serial_coupling_yaml PROPERTIES
            LABELS docs-example)
    endif()
endif()
