#include <gtest/gtest.h>

#include <iostream>
#include <fstream>
#include <string.h>

#include <i01_core/MappedRegion.hpp>

TEST(core_mmap, core_mmap_test)
{
    using i01::core::MappedRegion;
    std::string path("/tmp/mmap_test.txt");
    MappedRegion mf(path.c_str(), 4096);
    ASSERT_TRUE(mf.opened()) << "MappedRegion file not opened.";
    ASSERT_TRUE(mf.mapped()) << "MappedRegion file not mapped.";
    char * p = mf.data<char>();
    ::strcpy(p, "The quick brown fox jumps over the lazy dog.");
    ::strcpy(p + 80, "The quick brown fox jumps over the lazy dog.");
    ASSERT_EQ('T', p[0]) << "First char not equal to T.";
    ASSERT_EQ('T', p[80]) << "80th char not equal to T.";
    mf.sync();
    mf.munmap();
    mf.close();

    std::ifstream ifs;
    ifs.open(path.c_str(), std::ifstream::in);
    ASSERT_EQ('T', ifs.get()) << "Opened file (ifstream) first char not equal to T.";
    ifs.close();
}
