Names: Colin Dignazio, Mitchell Hamm
Student #: 7712437, 7712434
Course: COMP 4430 A01
Assignment: 5

To build A5:
  make clean
  make qemu SELECTION=<OPTION>

<OPTION>: NONE, NFU, FIFO

To test A5, build then run:
  myMemTest


Files added:
myMemTest.c  - Tests our algorithms and prints out the number of ticks they took to complete
sysfile.h    - header for open_file function and isdirempty function

Files modified:
proc.h    - Added members to keep track of proc pages

proc.c    - Added functions to save and read external page files
          - Copy external page files on fork
          - Increment process page's "age" for NFU
          - Initialize page tracker values on creation

trap.c    - Increment "age" of pages on each tick interupt
          - Check external page files on page fault

sysfile.c - Added open_file function to open files from the kernel

vm.c      - Logic for page selection algorithms
          - Keep track of proc pages as they are allocated, deallocated and swapped

defs.h    - Added function prototypes for some functions

exec.c    - Reseting values to track proc pages

file.c    - Adding filesetoffset function
          - Added unlink function to free external page file

file.h    - Added a function prototype

mmu.h     - Added PTE_PG flag

string.c  - Added itoa, inplace_reverse and strncat functions


Description:
Implemented external page handling for xv6 so that once a certain number of physical pages is reached, the kernel starts swapping pages out to page files. 
Three algorithms are implemented: 
NFU - Prefers to select pages that have been accessed less recently
FIFO - Selects pages based on when they were swapped into physical memory on a first in, first out basis
NONE - Disable external paging framework


Analysis:

See myMemTest.c for a detailed line by line description of the sanity test. Basically we allocate the maximum number of pages and do some accesses to test that page faulting works. Additionally, we fork the process and do some allocations to ensure that page files get copied on fork. We end by doing a test and timing the results so that we can compare algorithms.

We do two tests:
ALLOC_TEST: Allocate 10 new pages and access them.
STRESS_TEST: Do 5 page accesses 100 times with 4 of the accesses being the same page in each iteration.

====RESULTS====
Alg: NONE   
ALLOC_TEST:   0 ticks
STRESS_TEST:  1 tick

Alg: NFU
ALLOC_TEST:   330 ticks
STRESS_TEST:  2178 ticks

Alg: FIFO
ALLOC_TEST:   341 ticks
STRESS_TEST:  2694 ticks
===============

As expected, the NONE algorithm far out performed the others. This is because using the NONE algorithm keeps all of the pages in memory giving very fast access time. The draw back however is the large amount of physical memory that this requires. Because of this, its not really fair to compare it to the other two algorithms.

The NFU algorithm outperforms FIFO by a bit in ALLOC_TEST and by a lot in STRESS_TEST. This is because NFU keeps pages that are accessed frequently in the physical memory while FIFO will swap out any page regardless of how much it is used if it is that page's turn. This is especially apparent in STRESS_TEST since we access the same four pages in every iteration.
