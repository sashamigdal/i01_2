find_package(Libsystemd REQUIRED)

i01_add_executable("logger"
    RECURSE
    PIC OFF
    LINK_LIBS "i01_core" "i01_net" "dl" "${LIBSYSTEMD_LIBRARIES}"
    DEPENDS "i01_core" "i01_net"
)
set_property(GLOBAL PROPERTY "I01_LOGGER_EXECUTABLE" "${I01_BIN}/logger")
