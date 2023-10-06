#ifndef COMMON_H
#define COMMON_H

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <linux/kernel-page-flags.h>
#include <poll.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>
#include <sys/ioctl.h>
#include <signal.h>

#define NOINLINE __attribute__((noinline))
#define UNUSED __attribute__((unused))

#define PAGE_SHIFT 12
#define HPAGE_SHIFT 21

#define PAGE_SIZE (1 << PAGE_SHIFT)
#define HPAGE_SIZE (1 << HPAGE_SHIFT)

#define fail(...) do { fprintf(stderr, __VA_ARGS__); exit(EXIT_FAILURE); } while (false)

static bool verbose = false;

#define log(...) if (verbose) { fprintf(stderr, __VA_ARGS__); }

struct Options {
    // Whether to busy loop on syscalls with non blocking, or whether to block
    bool busy_loop = false;
    bool poll = false;

    // Whether to allocate the buffers in a hige page
    bool huge_page = false;
    bool check_huge_page = false;

    // How big should the buffer be
    size_t buf_size = 1 << 18;
    bool write_with_vmsplice = false;
    bool read_with_splice = false;

    // Whether pages should be gifted (and then moved if with READ_WITH_SPLICE) to vmsplice
    bool gift = false;
    
    // Lock pages to ensure that they aren't reclaimed
    bool lock_memory = false;

    // Don't fault pages in before we start piping
    bool dont_touch_pages = false;

    // Use a single, contiguous buffer, rather than two page-aligned ones. This increases page table
    // contention, as the author of fizzbuzz notes for a ~20% slowdown.
    bool same_buffer = false;

    // Output CSV rather than human readable
    bool csv = false;

    // Bytes to pipe (10GiB)
    size_t bytes_to_pipe = (1ull << 30) * 10ull;

    // Pipe size. If 0, the size will not be set. If we're using vmsplice, the buffer size will be
    // automatically determined, and setting it here is an error.
    size_t pipe_size =0;
};

static size_t read_size_str(const char *str)
{
    size_t sz;
    char control;
    int matched = sscanf(str, "%zu%c", &sz, &control);
    if (matched == 1)
    {
        // no-op -- it's bytes
    }
    else if (matched == 2 && control == 'G')
    {
        sz = sz << 30;
    }
    else if (matched == 2 && control == 'M')
    {
        sz = sz << 20;
    }
    else if (matched == 2 && control == 'K')
    {
        sz = sz << 10;
    }
    else
    {
        fail("bad size specification %s\n", str);
    }

    return sz;
}

__attribute__((unused)) 
static void write_size_str(size_t x, char* buf)
{
    if ((x & ((1 << 30)-1)) == 0)
    {
       sprintf(buf, "%zuGiB", x >> 30); 
    }
    else if ((x & ((1 << 20)-1)) == 0)
    {
       sprintf(buf, "%zuMiB", x >> 20); 
    }
    else if ((x & ((1 << 10)-1)) == 0)
    {
       sprintf(buf, "%zuKiB", x >> 10); 
    }
    else
    {
        sprintf(buf, "%zuB", x);
    }
}

static void parse_options(int argc, char** argv, Options& options)
{
    struct option long_options[] = {
        {"verbose",                 no_argument,        0, 0},
        {"busy_loop",               no_argument,        0, 0},
        {"poll",                    no_argument,        0, 0},
        {"huge_page",               no_argument,        0, 0},
        {"check_huge_page",         no_argument,        0, 0},
        {"buf_size",                required_argument,  0, 0},
        {"write_with_vmsplice",     no_argument,        0, 0},
        {"read_with_splice",        no_argument,        0, 0},
        {"gift",                    no_argument,        0, 0},
        {"bytes_to_pipe",           required_argument,  0, 0},
        {"pipe_size",               required_argument,  0, 0},
        {"lock_memory",             no_argument,        0, 0},
        {"dont_touch_pages",        no_argument,        0, 0},
        {"same_buffer",             no_argument,        0, 0},
        {"csv",                     no_argument,        0, 0},
        {0,                         0,                  0, 0}
    };

    opterr = 0;
    while (true)
    {
        // TODO : Implement
    }
}

#endif 
