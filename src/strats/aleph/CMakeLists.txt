set(I01_MODULE "strategy_aleph")
i01_add_library(${I01_MODULE}
    SHARED
    RECURSE
    NOHEADERS
    LINK_LIBS "i01_md" "i01_oe" "i01_ts"
    DEPENDS "i01_md" "i01_oe" "i01_ts" "engine"
    )
#i01_add_global_dependencies(INCLUDE_CURRENT)
#get_property(I01_ENGINE_EXECUTABLE GLOBAL PROPERTY I01_ENGINE_EXECUTABLE)
#add_test(NAME ${I01_MODULE}_regression_test
#    COMMAND ${I01_ENGINE_EXECUTABLE} "--lua-file=${I01_CONF}/sessions_uat.lua"
#    "--lua-file=${I01_DATA}/aleph_test.lua"
#    WORKING_DIRECTORY ${I01_LIB})

