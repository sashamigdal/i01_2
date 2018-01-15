/*
 *  Copyright (c) 2013 Ido Rosen
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <libgen.h>

#include <sys/capability.h>
#include <sys/types.h>
#include <sys/prctl.h>
#include <sys/resource.h>

int main(int argc, char *argv[])
{
    char * argv0 = basename(argv[0]);
    /* No executable specified. */
    if (argc < 2)
    {
        printf(
            "usage: %s <executable> [arg...]\n"
            "Uncaps memory, scheduling, and other POSIX capabilities and limits.\n\n"
            , argv0);
        exit(0);
    }

    /* Not setuid root. */
    if (geteuid() != 0)
    {
        fprintf(stderr, "%s: must be setuid root.\n", argv0);
        exit(1);
    }

    /* Write to /dev/cpu_dma_latency */

    {
        const char * cdl = "/dev/cpu_dma_latency";
        uint32_t acceptable_latency = 1;
        int cdlfd = open(cdl, O_WRONLY);
        if (cdlfd < 0)
        {
            fprintf(stderr, "%s: failed to open %s.\n", argv0, cdl); 
        } else {
            int ret = write(cdlfd, &acceptable_latency, sizeof(acceptable_latency));
            if (ret <= 0) {
                fprintf(stderr, "%s: failed to write to %s.\n", argv0, cdl);
            } else {
                fcntl(cdlfd, F_SETFD, fcntl(cdlfd, F_GETFD, 0) | FD_CLOEXEC);
                /* leave the fd open after we exec */
            }
        }
    }

    /* Set POSIX capabilities */

    {
        cap_t c;
        cap_value_t cl[1] = { CAP_SYS_NICE };

        c = cap_get_proc();
        if ( (c == NULL)||(cap_set_flag(c, CAP_EFFECTIVE, 1, cl, CAP_SET) == -1)||(cap_set_proc(c) == -1))
        {
            fprintf(stderr, "%s: failed to set POSIX capabilities.\n", argv0);
            perror(argv0);
        }
        cap_free(c);

        if (prctl(PR_SET_KEEPCAPS, 1) < 0)
        {
            fprintf(stderr, "%s: failed to keep capabilites via prctl.\n", argv0);
            perror(argv0);
        }
    }

    /* Set memory limits */

    {
        struct rlimit l = {RLIM_INFINITY, RLIM_INFINITY};
        if (setrlimit(RLIMIT_MEMLOCK, &l) != 0)
        {
            fprintf(stderr, "%s: failed to uncap memlock.\n", argv0);
            perror(argv0);
        }

        if (setrlimit(RLIMIT_NICE, &l) != 0)
        {
            fprintf(stderr, "%s: failed to uncap nice.\n", argv0);
            perror(argv0);
        }

        struct rlimit fl = {8192, 8192};
        if (setrlimit(RLIMIT_NOFILE, &fl) != 0)
        {
            fprintf(stderr, "%s: failed to uncap number of fds.\n", argv0);
            perror(argv0);
        }

        if (setrlimit(RLIMIT_RTPRIO, &fl) != 0)
        {
            fprintf(stderr, "%s: failed to uncap RT priority.\n", argv0);
            perror(argv0);
        }
    }

    /* Set uid */
    {
        if (getuid() != 0)
        {

            if (setuid(getuid()) != 0)
            {
                fprintf(stderr, "%s: failed to drop root.\n", argv0);
                perror(argv0);
                exit(1);
            }

            if (geteuid() == 0)
            {
                fprintf(stderr, "%s: failed to drop root.\n", argv0);
                exit(1);
            }
        }

    }

    execv(argv[1], argv + 1);
    fprintf(stderr, "%s: execv failed.\n", argv0);
    perror(argv0);
    exit(1);
}
