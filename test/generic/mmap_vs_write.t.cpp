#include <gtest/gtest.h>
#include <iostream>

// Not all of these are needed:
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <i01_core/macro.hpp>
#include <i01_core/Time.hpp>

namespace {
    const size_t PAGESIZE = sysconf(_SC_PAGE_SIZE);
    const size_t BUFSIZE = 8 * 1024 * 1024; //< 8MB.
    const char FILENAME[] = "/tmp/mmap_test";
}

using i01::core::MonotonicTimer;


TEST(system_performance, filewrite_seqwrite_speed)
{
    mlockall(MCL_CURRENT|MCL_FUTURE);
    MonotonicTimer t;
    char* buffer = nullptr;
    if (0 != posix_memalign((void**)&buffer, PAGESIZE, PAGESIZE))
        abort();
    memset((void*)buffer, 'X', PAGESIZE);
    int fd = open(FILENAME, O_CREAT | O_RDWR, (mode_t)0600);
    if (fd == -1)
    {
        perror("open");
        exit(EXIT_FAILURE);
    }
    if (0 != ftruncate(fd, BUFSIZE))
        abort();
    posix_fadvise(fd, 0, BUFSIZE, POSIX_FADV_SEQUENTIAL);
    t.start();
    for (uint64_t i = 0; i < BUFSIZE; i+=PAGESIZE)
    {
        if (UNLIKELY(write(fd, buffer, PAGESIZE) != (int)PAGESIZE))
        {
            perror("write");
            exit(EXIT_FAILURE);
        }
    }
    t.stop();
    std::cout << "write: " << t.interval() << std::endl;
    close(fd);
    free(buffer);
}

TEST(system_performance, mmapcpywrite_seqwrite_speed)
{
    mlockall(MCL_CURRENT|MCL_FUTURE);
    MonotonicTimer t;
    char* buffer = nullptr;
    if (0 != posix_memalign((void**)&buffer, PAGESIZE, PAGESIZE))
        abort();
    memset((void*)buffer, 'X', sizeof(buffer));
    int fd = open(FILENAME, O_CREAT | O_RDWR, (mode_t)0600);
    if (fd == -1)
    {
        perror("open");
        exit(EXIT_FAILURE);
    }
    if (0 != ftruncate(fd, BUFSIZE))
        abort();
    posix_fadvise(fd, 0, BUFSIZE, POSIX_FADV_SEQUENTIAL);
    char * map_p = (char*)mmap((void*)0, BUFSIZE, PROT_WRITE, MAP_SHARED, fd, 0);
    if (map_p == MAP_FAILED)
    {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    mlock(map_p, BUFSIZE);
    for (uint64_t i = 0; i < BUFSIZE; i+=PAGESIZE)
    {
        map_p[i] = 0;
    }
    madvise(map_p, BUFSIZE, MADV_SEQUENTIAL);
    t.start();
    for (uint64_t i = 0; i < BUFSIZE; i+=PAGESIZE)
    {
        if (UNLIKELY(memcpy((map_p + i), buffer, PAGESIZE) != (map_p + i)))
        {
            perror("memcpy");
            exit(EXIT_FAILURE);
        }
    }
    t.stop();
    std::cout << "mmapa: " << t.interval() << std::endl;
    munmap((void*)map_p, BUFSIZE);
    close(fd);
    free(buffer);
}

TEST(system_performance, mmapsetwrite_seqwrite_speed)
{
    mlockall(MCL_CURRENT|MCL_FUTURE);
    MonotonicTimer t;
    int fd = open(FILENAME, O_CREAT | O_RDWR, (mode_t)0600);
    if (fd == -1)
    {
        perror("open");
        exit(EXIT_FAILURE);
    }
    if (0 != ftruncate(fd, BUFSIZE))
        abort();
    posix_fadvise(fd, 0, BUFSIZE, POSIX_FADV_SEQUENTIAL);
    char * map_p = (char*)mmap((void*)0, BUFSIZE, PROT_WRITE, MAP_SHARED, fd, 0);
    if (map_p == MAP_FAILED)
    {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    mlock(map_p, BUFSIZE);
    for (uint64_t i = 0; i < BUFSIZE; i+=PAGESIZE)
    {
        map_p[i] = 0;
    }
    madvise(map_p, BUFSIZE, MADV_SEQUENTIAL);
    t.start();
    for (uint64_t i = 0; i < BUFSIZE; i+=8)
    {
        map_p[i] = 'X';
    }
    t.stop();
    std::cout << "mmapb: " << t.interval() << std::endl;
    munmap((void*)map_p, BUFSIZE);
    close(fd);
}
