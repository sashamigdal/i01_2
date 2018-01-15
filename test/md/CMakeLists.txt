i01_add_test("md_ut"
    RECURSE GTEST CTEST
    INCLUDE_DIRS "${I01_SRC}/md"
    LINK_LIBS "i01_md"
    DEPENDS "i01_md")

