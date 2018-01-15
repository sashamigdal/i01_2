find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

file(GLOB I01_TEST_DIRS *)

foreach (d ${I01_TEST_DIRS})
    if(IS_DIRECTORY "${d}")
        add_subdirectory(${d})
    endif()
endforeach()
