#include "types.h"
#include "user.h"

#define PAGE_SIZE 4096

int main(int argc, char* argv[]) {
  int i, size;
  char* mems[10];

  if(argc != 2) {
    exit();
  }

  size = atoi(argv[1]);

  for(i = 0; i < size; i++) {
    mems[i] = malloc(PAGE_SIZE);
    memset(mems[i], 0, PAGE_SIZE);
  }

  exit();
}