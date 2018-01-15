file(GLOB I01_APPS_DIRS *)

foreach (d ${I01_APPS_DIRS})
    if(IS_DIRECTORY "${d}")
        add_subdirectory(${d})
    endif()
endforeach()
