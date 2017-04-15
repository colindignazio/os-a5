#include "types.h"
#include "user.h"

#define MAX_PSYC_PAGES 15
#define MAX_TOTAL_PAGES 30
#define MAX_EXTERN_PAGES (MAX_TOTAL_PAGES - MAX_PSYC_PAGES)

#define PGSIZE 4096

int main(int argc, char *argv[]) {
  char *pages[MAX_TOTAL_PAGES];
  int i;

  // 3 pages are required to hold the inital process information at addresses 0x0000, 0x1000 and 0x2000
  // So allocate the remaining 12 allowed physical pages at addresses 0x3000 to 0xe000.
  // Since we are not allocating more than 15 physical pages yet there should be no page faults
  // and no external pages.
  for(i = 0; i < MAX_PSYC_PAGES - 3; i++) {
    pages[i] = sbrk(PGSIZE);
    memset(pages[i], '*', PGSIZE);  // Set the values to something so our page files have something in them when we start swapping.
    printf(1, "pages[%d] = %x\n", i, pages[i]);
  }

  // Next pages to be allocated will cause page faults creating external pages and forcing the process
  // to start swapping our physical pages.
  for(i = MAX_PSYC_PAGES; i < MAX_TOTAL_PAGES; i++) {
    pages[i] = sbrk(PGSIZE);
    memset(pages[i], '*', PGSIZE);  // Set the values to something so our page files have something in them when we start swapping.
    printf(1, "pages[%d] = %x\n", i, pages[i]);
  }

  // For FIFO, we expect to see physical pages getting swapped in the order that they were created starting at slot 0.

  exit();
}