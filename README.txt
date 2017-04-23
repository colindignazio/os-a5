Names: Colin Dignazio, Mitchell Hamm
Student #: 7712437, 7712434
Course: COMP 4430 A01
Assignment: 6

To build A5:
  make clean
  make qemu SELECTION=<OPTION>

<OPTION>: NONE, NFU, FIFO

To test A6, build then run:
  myMemTest


Files added since A5:
myMemTest.c  - Tests our algorithms and prints out the number of ticks they took to complete
sysfile.h    - header for open_file function and isdirempty function

Files modified since A5:
Makefile  - Changes to create swap.img on build

fs.c      - Don;t write to log if writing to the swap disk
          - Added functions to get a create inodes from the swap disk

ide.c     - Modifications to read/write to the swap disk if the b->dev == 2

proc.h    - Added inode to keep track of the root directory on the swap disk

proc.c    - Initialize root directory of the swap disk

sysfile.c - open_file now opens/creates a file on the swap disk

defs.h    - Added function prototypes for some functions

file.c    - Added unlink_from_swap function to delete a file from the swap drive

file.h    - Added a function prototype


Description:
Added on to our paging system from A5. Page files now get read and written to a seperate filesystem called swap.img.


Analysis:

Swapping pages appeared to be slower than it was for A5. No other noticable differences with writing to a seperate swap file system.
