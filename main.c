#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>

#define LOG(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)

static double ghz = 0;

static inline int getrate(double* ghz)
{
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if (!fp) {
        return -1;
    }

    double hz;

    ssize_t read;
    size_t len = 0;
    char * line = NULL;

    while ((read = getline(&line, &len, fp)) != -1) {
        if (sscanf(line, "cpu MHz\t: %lf", &hz) == 1) {
            break;
        }
    }

    static const unsigned long long MHZ_TO_GHZ = 1000;
    *ghz = hz / MHZ_TO_GHZ;

    free(line);
    fclose(fp);
    return 0;
}

static inline unsigned long long getcycles(void)
{
	unsigned long low, high;
	asm volatile ("rdtsc" : "=a" (low), "=d" (high));
	return ((low) | (high) << 32);
}

static inline unsigned long long gethosttime(unsigned long long cycles)
{
    return cycles / ghz;
}

static inline unsigned long long timeofday_to_ns(const struct timeval *tv)
{
    static const unsigned long long  S_TO_NS = 1000000000;
    static const unsigned long long US_TO_NS = 1000;

    unsigned long long ns = 0;
    ns += tv->tv_sec * S_TO_NS;
    ns += tv->tv_usec * US_TO_NS;
    return ns;
}

static inline unsigned long long calc_mean(const unsigned long long *bench, unsigned n)
{
    unsigned long long mean  = 0;
    for (unsigned i = 0; i < n; i++) {
        mean += bench[i];
    }
    mean /= n;
    return mean;
}

static inline unsigned long long calc_std(const unsigned long long *bench, unsigned n,
                                          unsigned long long mean)
{
    unsigned long long std   = 0;
    for (unsigned i = 0; i < n; i++) {
        unsigned long long diff = bench[i] - mean;
        std += (diff * diff);
    }
    std /= n;
    std = sqrt(std);
    return std;
}

static void bench_getcycles()
{
    unsigned long long before, after;

	before = getcycles();
    (void)getcycles();
	after  = getcycles();
	LOG("getcycles() execution took [%llu]ns",
        gethosttime(after - before));
}

static void bench_gettimeofday()
{
    int rv;
    struct timeval tv;
    unsigned long long before, after;

    before = getcycles();
    rv = syscall(__NR_gettimeofday, &tv, NULL);
    after  = getcycles();
	LOG("gettimeofday() %s and execution took [%llu]ns",
        (rv == 0 ? "succeeded" : "failed"),
        gethosttime(after - before));
}

static void bench_loop_using_getcycles(unsigned outer_iters, unsigned inner_iters)
{
    unsigned long long *bench = calloc(outer_iters, sizeof(*bench));

    unsigned long long before, after;
	unsigned i, j, k;
    for (i = 0; i < outer_iters; i++) {
        before = getcycles();
        for (j = 0; j < inner_iters; j++) {
            k = i + j;
            (void)k;
        }
        after = getcycles();

        bench[i] = after - before;
    }

    unsigned long long mean = calc_mean(bench, outer_iters);
    unsigned long long std = calc_std(bench, outer_iters, mean);
    LOG("getcycles() loop measurement: mean[%llu]ns, std[%llu]ns", mean, std);
}

static void bench_loop_using_gettimeofday(unsigned outer_iters, unsigned inner_iters)
{
    unsigned long long *bench = calloc(outer_iters, sizeof(*bench));

    int rv_before, rv_after;
    struct timeval before, after;
	unsigned i, j, k;
    for (i = 0; i < outer_iters; i++) {
        rv_before = syscall(__NR_gettimeofday, &before, NULL);
        for (j = 0; j < inner_iters; j++) {
            k = i + j;
            (void)k;
        }
        rv_after = syscall(__NR_gettimeofday, &after, NULL);

        if (rv_before != 0 || rv_after != 0) {
            continue;
        }

        bench[i] = timeofday_to_ns(&after) - timeofday_to_ns(&before);
    }

    unsigned long long mean = calc_mean(bench, outer_iters);
    unsigned long long std = calc_std(bench, outer_iters, mean);
    LOG("gettimeofday() loop measurement: mean[%llu]ns, std[%llu]ns", mean, std);
}

int main(int argc, char **argv)
{
    if (getrate(&ghz)) {
        LOG("Get rate FAILED");
        return -1;
    }

    bench_getcycles();
    bench_gettimeofday();

    static const unsigned OUTER_ITERS = 1000;
    static const unsigned INNER_ITERS = 100;
    bench_loop_using_getcycles(OUTER_ITERS, INNER_ITERS);
    bench_loop_using_gettimeofday(OUTER_ITERS, INNER_ITERS);


	return 0;
}
