i01_add_test("core_config_ut"
    RECURSE GTEST CTEST
    INCLUDE_DIRS "${I01_SRC}/core"
    LINK_LIBS "i01_core"
    DEPENDS "i01_core")
