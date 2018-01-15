set(I01_MODULE "i01_oe")

i01_add_library(${I01_MODULE}
    ${I01_LIBRARY_TYPE}
    RECURSE
    INSTALL_DIRS ${I01_MODULE}
    LINK_LIBS "i01_md" "i01_fix"
    DEPENDS "i01_md" "i01_fix"
    )
i01_add_global_dependencies(INCLUDE_CURRENT)
