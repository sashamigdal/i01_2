file(GLOB I01_STRAT_DIRS *)

foreach (d ${I01_STRAT_DIRS})
    if(IS_DIRECTORY "${d}")
        add_subdirectory(${d})
    endif()
endforeach()
