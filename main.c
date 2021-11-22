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

void bench_getcycles()
{
    unsigned long long before, after;

	before = getcycles();
    (void)getcycles();
	after  = getcycles();
	LOG("getcycles() execution took [%llu]ns",
        gethosttime(after - before));
}

void bench_gettimeofday()
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

int main(int argc, char **argv)
{
    if (getrate(&ghz)) {
        LOG("Get rate FAILED");
        return -1;
    }

    bench_getcycles();
    bench_gettimeofday();

	return 0;
}
