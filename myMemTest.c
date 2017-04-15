#include "types.h"
#include "user.h"

#define MAX_PSYC_PAGES 15
#define MAX_TOTAL_PAGES 30
#define MAX_EXTERN_PAGES (MAX_TOTAL_PAGES - MAX_PSYC_PAGES)

#define PGSIZE 4096

int main(int argc, char *argv[]) {
#ifdef NFU
  const char *alg = "NFU";
#elif FIFO
  const char *alg = "FIFO";
#elif NONE
  const char *alg = "NONE";
#endif

  char *pages[MAX_TOTAL_PAGES];
  int i;
  int start_ticks;

  // 3 pages are required to hold the inital process information at addresses 0x0000, 0x1000 and 0x2000
  // So allocate the remaining 12 allowed physical pages at addresses 0x3000 to 0xe000.
  // Since we are not allocating more than 15 physical pages yet there should be no page faults
  // and no external pages. We don't access the last 5 to give them a higher likelyhood of 
  // being chosen for NFU
  for(i = 3; i < MAX_PSYC_PAGES; i++) {
    pages[i] = sbrk(PGSIZE);
    printf(1, "pages[%d] = %x\n", i, pages[i]);
  }

  // Access pages at indexes 3-10 so that pages indexes 10-14 are more likely to be chosen for NFU
  for(i = 3; i < 10; i++) {
    memset(pages[i], '*', PGSIZE);
  }

  // Next pages to be allocated will cause page faults creating external pages and forcing the process
  // to start swapping our physical pages.  Since we just accessed pages 3-10, we should see 10-14 being
  // swapped here for NFU.
  for(i = MAX_PSYC_PAGES; i < MAX_PSYC_PAGES + 5; i++) {
    pages[i] = sbrk(PGSIZE);
    memset(pages[i], '*', PGSIZE);
    printf(1, "pages[%d] = %x\n", i, pages[i]);
  }

  // Allocate some memory in a forked process to ensure the page tables are being copied properly
  if(fork() == 0) {
    for(i = MAX_PSYC_PAGES + 5; i < MAX_PSYC_PAGES + 10; i++) {
      pages[i] = sbrk(PGSIZE);
      memset(pages[i], '*', PGSIZE);
      printf(1, "pages[%d] = %x\n", i, pages[i]);
    }
    exit();
  }

  wait();

  // ALLOC_TEST
  start_ticks = uptime();
  for(i = MAX_PSYC_PAGES + 10; i < MAX_TOTAL_PAGES; i++) {
    pages[i] = sbrk(PGSIZE);
    memset(pages[i], '*', PGSIZE);  // Set the values to something so our page files have something in them when we start swapping.
    printf(1, "pages[%d] = %x\n", i, pages[i]);
  }
  printf(1, "Algorithm: %s took %d ticks to allocate the maximum number of pages...\n\n", alg, uptime() - start_ticks);

  // STRESS_TEST
  start_ticks = uptime();
  for(i = 0; i < 100; i++) {
    pages[(i % (MAX_TOTAL_PAGES - 3)) + 3][0] = '*';
    pages[10][0] = '*';
    pages[14][0] = '*';
    pages[3][0] = '*';
    pages[29][0] = '*';
    printf(1, ".");
  }
  printf(1, "\nAlgorithm: %s took %d ticks to complete the heavy swapping test...\n\n", alg, uptime() - start_ticks);

  exit();
}