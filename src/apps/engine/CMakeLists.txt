i01_add_executable("engine"
    RECURSE
    PIC OFF
    LINK_LIBS "i01_md" "i01_oe" "i01_ts" "dl"
    DEPENDS "i01_md" "i01_oe" "i01_ts"
)
set_property(GLOBAL PROPERTY "I01_ENGINE_EXECUTABLE" "${I01_BIN}/engine")
