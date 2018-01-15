i01_add_executable("reorderpcap"
    RECURSE
    PIC OFF
    LINK_LIBS "i01_core"
    DEPENDS "i01_core"
)
set_property(GLOBAL PROPERTY "I01_REORDERPCAP_EXECUTABLE" "${I01_BIN}/reorderpcap")
