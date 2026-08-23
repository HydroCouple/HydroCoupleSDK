# SPDX-License-Identifier: MIT
# examples/CMakeLists.txt — runnable HydroCoupleSDK examples
#
# Built in-tree when BUILD_EXAMPLES=ON. Each example directory also carries
# its own standalone CMakeLists.txt so it can be configured against an
# *installed* SDK (find_package(HydroCoupleSDK)) — the acceptance test that
# the install export is complete.

add_executable(serial_coupling serial_coupling/main.cpp)
target_link_libraries(serial_coupling PRIVATE HydroCoupleSDK)

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

if(USE_MPI)
    add_executable(distributed_coupling distributed_coupling/main.cpp)
    target_link_libraries(distributed_coupling
        PRIVATE HydroCoupleSDK MPI::MPI_CXX)
endif()
