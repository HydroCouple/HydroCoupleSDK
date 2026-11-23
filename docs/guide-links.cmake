# Doxygen places explicitly named pages under two hash directories with CREATE_SUBDIRS.
file(GLOB manual_pages RELATIVE "${SITE}" "${SITE}/d*/d*/${PREFIX}_guide*.html")
if(NOT manual_pages)
    message(FATAL_ERROR "No pages generated for ${PREFIX}_guide")
endif()
file(MAKE_DIRECTORY "${SITE}/guide")
foreach(page IN LISTS manual_pages)
    get_filename_component(stem "${page}" NAME_WE)
    if(stem STREQUAL "${PREFIX}_guide")
        set(bookmark index)
    else()
        string(REGEX REPLACE "^${PREFIX}_guide_" "" bookmark "${stem}")
    endif()
    file(WRITE "${SITE}/guide/${bookmark}.html"
        "<!doctype html><html lang=\"en\"><meta charset=\"utf-8\"><title>Open manual</title>"
        "<meta http-equiv=\"refresh\" content=\"0;url=../${page}\"><a href=\"../${page}\">Open manual</a></html>\n")
endforeach()
