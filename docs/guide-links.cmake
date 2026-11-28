# Doxygen places explicitly named pages under two hash directories with CREATE_SUBDIRS.
# Each guide chapter (<prefix>_guide_<name>) gets the stable address guide/<name>.html and each
# reference chapter (<prefix>_ref_<name>) gets guide/reference_<name>.html.
file(GLOB manual_pages RELATIVE "${SITE}" "${SITE}/d*/d*/${PREFIX}_guide*.html")
if(NOT manual_pages)
    message(FATAL_ERROR "No pages generated for ${PREFIX}_guide")
endif()
file(GLOB reference_pages RELATIVE "${SITE}" "${SITE}/d*/d*/${PREFIX}_ref_*.html")
file(MAKE_DIRECTORY "${SITE}/guide")
foreach(page IN LISTS manual_pages reference_pages)
    get_filename_component(stem "${page}" NAME_WE)
    if(stem STREQUAL "${PREFIX}_guide")
        set(bookmark index)
    elseif(stem MATCHES "^${PREFIX}_ref_")
        string(REGEX REPLACE "^${PREFIX}_ref_" "reference_" bookmark "${stem}")
    else()
        string(REGEX REPLACE "^${PREFIX}_guide_" "" bookmark "${stem}")
    endif()
    file(WRITE "${SITE}/guide/${bookmark}.html"
        "<!doctype html><html lang=\"en\"><meta charset=\"utf-8\"><title>Open manual</title>"
        "<meta http-equiv=\"refresh\" content=\"0;url=../${page}\"><a href=\"../${page}\">Open manual</a></html>\n")
endforeach()
