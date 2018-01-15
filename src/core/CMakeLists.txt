
file(GLOB_RECURSE i01_core_SOURCES *.cpp *.cxx *.cc *.c)
file(GLOB_RECURSE i01_core_INCLUDES 
    i01_core/*.hpp i01_core/*.hxx i01_core/*.hh i01_core/*.h)

set(i01_core_VERSION "${I01_VERSION}")

configure_file(Version.cpp.in "${CMAKE_CURRENT_BINARY_DIR}/Version.cpp" @ONLY)
list(APPEND i01_core_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/Version.cpp)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")
link_directories("${LIBLZ4_LIBRARY_DIRS}")

add_library(i01_core ${I01_LIBRARY_TYPE} ${i01_core_SOURCES})
set_target_properties(i01_core PROPERTIES VERSION "${i01_core_VERSION}" SOVERSION 0)

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    target_link_libraries(i01_core
        ${PYTHON_LIBRARIES}
        ${Boost_LIBRARIES}
        ${TBB_LIBRARIES}
        ${LUA_LIBRARIES}
        ${LIBLZ4_LIBRARIES}
        rt)
else()
    target_link_libraries(i01_core
        ${PYTHON_LIBRARIES}
        ${Boost_LIBRARIES}
        ${TBB_DEBUG_LIBRARIES}
        ${LUA_LIBRARIES}
        ${LIBLZ4_LIBRARIES}
        rt)
endif()

i01_add_global_dependencies(ABSOLUTE 
    INCLUDE_CURRENT
    LINK_LIBS "i01_core"
    DEPENDS "i01_core")

install(TARGETS i01_core
    RUNTIME DESTINATION bin COMPONENT bin
    LIBRARY DESTINATION lib COMPONENT bin
    ARCHIVE DESTINATION lib/static COMPONENT devel
    )

install(DIRECTORY i01_core DESTINATION "include" COMPONENT devel)

