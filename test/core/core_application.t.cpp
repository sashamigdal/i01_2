#include <gtest/gtest.h>

#include <iostream>
#include <string.h>

#include <i01_core/Application.hpp>

TEST(core_application, core_application_test)
{
    using i01::core::Application;
    using i01::core::NoopApplication;
    const char *argv1[] = {"test_program"};
    NoopApplication(1, argv1); // should not exit.
    const char *argv2[] = {"test_program"
                          , "--optimize"
                          , "--reserve-memory=100"};
    NoopApplication(3, argv2);
    //const char *argv3[] = {"test_program", "--version"};
    //System(2, argv3); // calls exit(EXIT_SUCCESS).
}
