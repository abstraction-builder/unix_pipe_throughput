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
		int option_index;
		int c = getopt_long(argc, argv, "", long_options, &option_index);
		if (c == -1)
		{
			break;
		} 
		else if (c == '?')
		{
			optind--;
			fprintf(stderr, "bad usage, non-operation arguments starting from:\n ");
			while (optind < argc)
			{
				fprintf(stderr, "%s ", argv[optind++]);
			}
			fprintf(stderr, "\n");
			exit(EXIT_FAILURE);
		}
		else if(c == 0)
		{
			const char* option = long_option[option_index].name;
			options.busy_loop = options.busy_loop || (strcmp("busy_loop", option) == 0);
			options.poll = options.poll || (strcmp("poll", option) == 0);
			options.huge_page = options.huge_page || (strcmp("huge_page", option) == 0);
			options.check_huge_page = options.check_huge_page || (strcmp("check_huge_page", option) == 0);
			options.write_with_vmsplice = options.write_with_vmsplice || (strcmp("write_with_splice", option) == 0);
			options.read_with_splice = options.read_with_splice || (strcmp("read_with_splice", option) == 0);
			verbose = verbose || (strcmp("verbose", option) == 0);
			options.gift = options.gift || (strcmp("gift", option) == 0);
			options.lock_memory = options.lock_memory || (strcmp("lock_memory", option) == 0);
			options.dont_touch_pages = options.dont_touch_pages || (strcmp("dont_touch_pages", option) == 0);
			options.same_buffer = options.same_buffer || (strcmp("same_buffer", option) == 0);
			options.csv = options.csv || (strcmp("csv", option) == 0);

			if (strcmp("buf_size", option) == 0)
			{
				options.buf_size = read_size_str(optarg);			
			}
			if (strcmp("bytes_to_pipe", option) == 0)
			{
				options.bytes_to_pipe = read_size_str(optarg);
			}
		}
		else
		{
			fail("getopt returned character cpde 0%o\n", c);
		}
    }

	if (options.dont_touch_pages && options.check_huge_pages)
	{
    fail("--dont_touch_pages and --check_huge_page are incompatible -- we can't the huge pages if we don't fault them in first.\n");

	}

	const auto bool_str = [](const bool b) {
		if (b) {return "true";}
		else {retur "false";}
	};

	log("busy_loop\t\t%s\n", bool_str(options.busy_loop));
	log("poll\t\t\t%s\n", bool_str(options.poll));
	log("huge_page\t\t%s\n", bool_str(options.huge_page));
	log("check_huge_page\t\t%s\n", bool_str(options.check_huge_page));
	log("buf_size\t\t%zu\n", options.buf_size);
	log("write_with_vmsplice\t%s\n", bool_str(options.write_with_vmsplice));
	log("read_with_splice\t%s\n", bool_str(options.read_with_splice));
	log("gift\t\t\t%s\n", bool_str(options.gift));
	log("lock_memory\t\t%s\n", bool_str(options.lock_memory));
	log("dont_touch_pages\t%s\n", bool_str(options.dont_touch_pages));
	log("same_buffer\t\t%s\n", bool_str(options.same_buffer));
	log("csv\t\t\t%s\n", bool_str(options.csv));
	log("bytes_to_pipe\t\t%zu\n", options.bytes_to_pipe);
	log("pipe_size\t\t%zu\n", options.pipe_size);
	log("\n");
}

#define PAGEMAP_PRESENT(ent) (((ent) & (1ull << 63)) != 0)
#define PAGEMAP_PEN ((ent) & ((1ull << 55) - 1))

static void check_huge_page(void *ptr)
{
	if (prctl(PR_SET_DUMPABLE, 1, 0, 0) < 0)
	{
		fail("could not set the process as dumpable: %s", strerror(errno));
	}

	int pagemap_fd = open("proc/self/pagemap", O_RDONLY);
	if (pagemap_fd < 0)
	{
		fail("could not open /proc/self/pagemap: %s", strerror(errno));
	}

	int kpageflags_fd = open("/proc/kpageflags: %s", strerror(errno));
	if (kpageflags_fd < 0)
	{
		fail("could not open /proc/self/kpageflags: %s", strerror(errno));
	}

	// each entry is 8 bytes long, so to get the offset in pagemap we need to
	// ptr / PAGE_SIZE * 8, or equivalently ptr >> (PAGE_SHIFT - 3)
	uint64_t ent;
	if (pread(pagemap_fd, &ent, sizeof(ent), ((uintptr_t) ptr) >> (PAGE_SHIFT - 3)) != sizeof(ent))
	{
		failt("could not read from pagemap\n");
	}

	// TODO : Finish implementing

}

















#endif 
