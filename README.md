# Homework 2

Daniel Trugman, ID 303922611

## 1. Timing methodologies

(c) Both methods are sub-optimal. Why?
First an foremost, the purpose of benchmarking is to be able to exactly count the time it took to perform an operation.
In this case, we are benchmarking a CPU bound operation. Meaning, we are effectively trying to measure the amount of
CPU cycles it takes to perform a calculation that uses only the CPU.
The problem is, that if there is a context switch while running this measurement, both the CPU ticks and the "time of day"
continue to increment, even though our task is not executing.
Hence, we're measuring not only the time it took to run the calculation, but also time consumed by other threads running
unrelated computations.
The right way to benchmark CPU bound operations is by using something like `pthread_getcpuclockid`. Unlike our current
benchmarking techniques, this method reports the number of CPU tickets consumed by a specific thread, thus returning
much more accurate values.

However, between the two methods in our program, here are some key considerations:
1. Since `gettimeofday` takes longer, there are higher chances of a context switch in between. Thus, the standard deviation
is almost always bigger than the one for `getcycles`.
2. Since `gettimeofday` takes longer, the amount of time it takes for the program to reach the point where it fetches the
value is more significant then the time it takes for `getcycles`, so it "clouds" the measurement values, resulting in longer
loop times.
3. The method `getcycles` relies on our ability to get an exact indication of the CPU frequency, which might be difficult on a
real production system. The CPU can utilize different frequencies while idle/active, turbo-boost, hibernation, etc.
4. The method `getcycles` might not be handled correctly if during the execution, we had a context switch and our task
resumed its execution on a different CPU, where the TSC might have a different value, as TSC values are not synchronized
between different CPUs.
5. On my system, according to `/proc/cpuinfo`, there's a `constant_tsc` flag that guarantees a constant tick values that
adheres to the nominal CPU frequency. When this flag is enabled, the TSC counts time, not ticks and thus points #3 and #4
are not longer valid.

All in all, between those two, `getcycles` seems a better option when `constant_tsc` is available.

## 2. Understanding /proc/%pid%/maps

(a) Here is the output of `cat /proc/self/maps` on my system with a relevant explanation above every line:
```
>>> cat /proc/self/maps
# Binary ELF mapped as read-only & executable. This is the .text segment and instructions are fetched from here
# during the execution
00400000-0040c000 r-xp 00000000 fc:00 6029325                            /bin/cat

# Binary ELF mapped as read-only. This is the ELF's .rodata segment
0060b000-0060c000 r--p 0000b000 fc:00 6029325                            /bin/cat

# Binary ELF mapped as read-write. This is the ELF's .data segment
0060c000-0060d000 rw-p 0000c000 fc:00 6029325                            /bin/cat

# Heap memory in use by the program during execution
01768000-01789000 rw-p 00000000 00:00 0                                  [heap]

# glibc library mapped as read-only & executable. This is the .text segment of glibc
7f7253b3a000-7f7253cfa000 r-xp 00000000 fc:00 17044282                   /lib/x86_64-linux-gnu/libc-2.23.so
7f7253cfa000-7f7253efa000 ---p 001c0000 fc:00 17044282                   /lib/x86_64-linux-gnu/libc-2.23.so

# glibc library mapped as read-only. This is the .rodata segment of glibc
7f7253efa000-7f7253efe000 r--p 001c0000 fc:00 17044282                   /lib/x86_64-linux-gnu/libc-2.23.so

# glibc library mapped as read-write. This is the .data segment of glibc
7f7253efe000-7f7253f00000 rw-p 001c4000 fc:00 17044282                   /lib/x86_64-linux-gnu/libc-2.23.so
7f7253f00000-7f7253f04000 rw-p 00000000 00:00 0

# loader mapped as read-only & executable. This is the .text segment of ld
7f7253f04000-7f7253f2a000 r-xp 00000000 fc:00 17044274                   /lib/x86_64-linux-gnu/ld-2.23.so
# shared memory or buffers not allocated on the heap, i.e. anonymous mmap
7f72540f6000-7f725411b000 rw-p 00000000 00:00 0

# loader mapped as read-only. This is the .rodata segment of ld
7f7254129000-7f725412a000 r--p 00025000 fc:00 17044274                   /lib/x86_64-linux-gnu/ld-2.23.so

# loader mapped as read-only. This is the .data segment of ld
7f725412a000-7f725412b000 rw-p 00026000 fc:00 17044274                   /lib/x86_64-linux-gnu/ld-2.23.so
# shared memory or buffers not allocated on the heap, i.e. anonymous mmap
7f725412b000-7f725412c000 rw-p 00000000 00:00 0

# Stack memory in use by this thread during execution
7fff4cba0000-7fff4cbc1000 rw-p 00000000 00:00 0                          [stack]
7fff4cbd3000-7fff4cbd5000 r--p 00000000 00:00 0                          [vvar]

# Virtual dynamic share objects memory region
7fff4cbd5000-7fff4cbd7000 r-xp 00000000 00:00 0                          [vdso]

# Virtual syscall memory regio. Mostly for backward compatibility,
# vDSO is the new mechanism used nowadays
ffffffffff600000-ffffffffff601000 r-xp 00000000 00:00 0                  [vsyscall]
```

(b) Explaining the additional regions we can find for systemd-journal:

We can see that the binary has a bigger .rodata section, so it's separately mapped:
```
56090ebe1000-56090ebe9000 r--p 0001e000 08:05 2234586                    /usr/lib/systemd/systemd-journald
56090ebe9000-56090ebeb000 r--p 00025000 08:05 2234586                    /usr/lib/systemd/systemd-journald
```

We can find some memory-backed shared memory segments (A file multiple processes open and mmap a shared view): 
```
7faa40307000-7faa40b07000 rw-s 00000000 08:05 1589602                    /var/log/journal/a6b5310522aa43228fec8389a0c806b4/user-1000.journal
...
7faa41308000-7faa41b08000 rw-s 00000000 08:05 1583100                    /var/log/journal/a6b5310522aa43228fec8389a0c806b4/system.journal
...
7faa42f8f000-7faa42f90000 rw-s 00000000 00:18 296                        /run/systemd/journal/kernel-seqnum
```

We can also see many additional shared objects:
```
7faa42a6f000-7faa42a71000 r--p 00000000 08:05 2234956                    /usr/lib/x86_64-linux-gnu/libcap.so.2.32
7faa42a71000-7faa42a74000 r-xp 00002000 08:05 2234956                    /usr/lib/x86_64-linux-gnu/libcap.so.2.32
7faa42a74000-7faa42a75000 r--p 00005000 08:05 2234956                    /usr/lib/x86_64-linux-gnu/libcap.so.2.32
7faa42a75000-7faa42a76000 ---p 00006000 08:05 2234956                    /usr/lib/x86_64-linux-gnu/libcap.so.2.32
7faa42a76000-7faa42a77000 r--p 00006000 08:05 2234956                    /usr/lib/x86_64-linux-gnu/libcap.so.2.32
7faa42a77000-7faa42a78000 rw-p 00007000 08:05 2234956                    /usr/lib/x86_64-linux-gnu/libcap.so.2.32
7faa42a78000-7faa42a82000 r--p 00000000 08:05 2234912                    /usr/lib/x86_64-linux-gnu/libblkid.so.1.1.0
7faa42a82000-7faa42ab8000 r-xp 0000a000 08:05 2234912                    /usr/lib/x86_64-linux-gnu/libblkid.so.1.1.0
7faa42ab8000-7faa42ac8000 r--p 00040000 08:05 2234912                    /usr/lib/x86_64-linux-gnu/libblkid.so.1.1.0
7faa42ac8000-7faa42ac9000 ---p 00050000 08:05 2234912                    /usr/lib/x86_64-linux-gnu/libblkid.so.1.1.0
7faa42ac9000-7faa42ace000 r--p 00050000 08:05 2234912                    /usr/lib/x86_64-linux-gnu/libblkid.so.1.1.0
7faa42ace000-7faa42acf000 rw-p 00055000 08:05 2234912                    /usr/lib/x86_64-linux-gnu/libblkid.so.1.1.0
7faa42acf000-7faa42ad1000 r--p 00000000 08:05 2234821                    /usr/lib/x86_64-linux-gnu/libacl.so.1.1.2253
7faa42ad1000-7faa42ad6000 r-xp 00002000 08:05 2234821                    /usr/lib/x86_64-linux-gnu/libacl.so.1.1.2253
7faa42ad6000-7faa42ad8000 r--p 00007000 08:05 2234821                    /usr/lib/x86_64-linux-gnu/libacl.so.1.1.2253
7faa42ad8000-7faa42ad9000 r--p 00008000 08:05 2234821                    /usr/lib/x86_64-linux-gnu/libacl.so.1.1.2253
7faa42ad9000-7faa42ada000 rw-p 00009000 08:05 2234821                    /usr/lib/x86_64-linux-gnu/libacl.so.1.1.2253
7faa42ada000-7faa42ae0000 r--p 00000000 08:05 2235953                    /usr/lib/x86_64-linux-gnu/libselinux.so.1
7faa42ae0000-7faa42af9000 r-xp 00006000 08:05 2235953                    /usr/lib/x86_64-linux-gnu/libselinux.so.1
7faa42af9000-7faa42b00000 r--p 0001f000 08:05 2235953                    /usr/lib/x86_64-linux-gnu/libselinux.so.1
7faa42b00000-7faa42b01000 ---p 00026000 08:05 2235953                    /usr/lib/x86_64-linux-gnu/libselinux.so.1
7faa42b01000-7faa42b02000 r--p 00026000 08:05 2235953                    /usr/lib/x86_64-linux-gnu/libselinux.so.1
7faa42b02000-7faa42b03000 rw-p 00027000 08:05 2235953                    /usr/lib/x86_64-linux-gnu/libselinux.so.1
```

(c) We can see that the addresses change. This happens because the operating system randomizes the location of libraries in the system.
This is a security feature that aims to make it harder for attackers predict the addresses of known and useful methods in glibc,
and write shellcodes that exploit them, for example, but using return oriented programming (ROP) attacks.

(d) We can think of vDSO as the "successor" of vsyscall.
Both mechanisms try to avoid the overhead of context switching into the kernel when calling
"simple" system-calls. For example, consider the systemcall "gettimeofday", all it does is read a timer from the kernel space.
Instead of actually switching into the kernel, reading the value and returning to user-space, we can conceptually map a small memory
region into user-space that exports both the kernel timer and a tiny piece of code to read it. Then, the user-space program don't
really need to call the syscall using a traditional way and instead it can simply execute the mapped code snippet.
There were two disadvantages:
1. vsyscall only mapped four hard-coded methods
2. vsyscall used hard-coded (constant) addresses, and made it easier for attackers to exploit them
So, they "replaced" the vsyscall mechanism (they left it for backward compatibility by replacing the calls with traps that emulate the
virtual syscall flow so they'll still work, but with increased latency) with the vDSO.
The vDSO mechanism does pretty much the same thing, but only it uses dynamically allocated memory to export the virtual syscalls API
to user-space. The kernel maps a "virtual" library into every process, and passes the address of that library in the initial
auxiliary vector using the AT_SYSINFO_EHDR tag.
It solves the randomization part and also allows future expansion, backward compatibility and more.
