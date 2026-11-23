# SPDX-License-Identifier: MIT
# docs/CMakeLists.txt — Doxygen documentation target

set(DOXYGEN_IN  ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

# Doxygen supplied jQuery until version 1.17.
if(DOXYGEN_VERSION VERSION_LESS "1.17")
    set(DOXYGEN_JQUERY_SCRIPT [[<script type="text/javascript" src="$relpath^jquery.js"></script>]])
endif()
configure_file(custom/html/header.html header.html @ONLY)
configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

# Package the recorded tutorial data with the original example input.
set(sample_run "${CMAKE_CURRENT_BINARY_DIR}/sample-run")
foreach(file IN ITEMS results.csv run.json)
    configure_file("guide/sample-run/${file}" "${sample_run}/${file}" COPYONLY)
endforeach()
set(sample_input "examples/serial_coupling/composition.json")
configure_file("${CMAKE_SOURCE_DIR}/${sample_input}" "${sample_run}/${sample_input}" COPYONLY)
add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/serial-coupling-run.zip"
    COMMAND "${CMAKE_COMMAND}" -E tar cf "${CMAKE_CURRENT_BINARY_DIR}/serial-coupling-run.zip"
            --format=zip results.csv run.json "${sample_input}"
    DEPENDS "${sample_run}/results.csv" "${sample_run}/run.json" "${sample_run}/${sample_input}"
    WORKING_DIRECTORY "${sample_run}"
    VERBATIM
)

add_custom_target(docs ALL
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/serial-coupling-run.zip"
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating Doxygen documentation for HydroCoupleSDK"
    VERBATIM
)

# Keep /guide/ and its chapter bookmarks stable across Doxygen's hashed output paths.
add_custom_command(TARGET docs POST_BUILD
    COMMAND "${CMAKE_COMMAND}" "-DSITE=${CMAKE_CURRENT_BINARY_DIR}/doxygen/html"
            "-DPREFIX=sdk" -P "${CMAKE_CURRENT_SOURCE_DIR}/guide-links.cmake"
    VERBATIM
)
