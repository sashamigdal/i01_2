#include <gtest/gtest.h>

#include <algorithm>
#include <iostream>
#include <fstream>
#include <string.h>

#include <i01_core/MappedRegion.hpp>
#include <i01_core/LZ4.hpp>

TEST(core_lz4, core_lz4_test)
{
    using i01::core::MappedRegion;
    using i01::core::LZ4::FileReader;

    std::string path_compressed(STRINGIFY(I01_DATA) "/BZX_UNIT_1_20140724_1500_first5.pcap-ns.lz4");
    std::string path_decompressed(STRINGIFY(I01_DATA) "/BZX_UNIT_1_20140724_1500_first5.pcap-ns");

    MappedRegion d(path_decompressed, 0, true);
    ASSERT_TRUE(d.mapped()) << "reference file not mapped.";
    ASSERT_GT(d.size(), 0);
    FileReader r(path_compressed);
    ASSERT_TRUE(r.is_open()) << "is_open() failed.";
    char * buf = static_cast<char*>(malloc(d.size()));
    size_t n = 0;
    while (n < d.size()) {
        ssize_t nn = r.read(buf + n, d.size() - n);
        ASSERT_GE(nn, 0) << "Read " << n << " bytes out of " << d.size() << " error...";
        if (nn > 0) {
            n += nn;
        } else {
            break;
        }
    }
    ASSERT_EQ(d.size(),n);
    ASSERT_EQ(::memcmp(d.data<char>(), &buf[0], d.size()), 0);
    FileReader r2(path_compressed);
    ::free(buf);
    buf = static_cast<char*>(malloc(d.size()));
    n = 0;
    while (n < d.size()) {
        ssize_t nn = r.read(buf + n, std::max((size_t)d.size() - n,(size_t)100));
        ASSERT_GE(nn, 0) << "Read " << n << " bytes out of " << d.size() << " error...";
        if (nn > 0) {
            n += nn;
        } else {
            break;
        }
    }
}
