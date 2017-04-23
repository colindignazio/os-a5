// Simple PIO-based (non-DMA) IDE driver code.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"

#define SECTOR_SIZE   512
#define IDE_BSY       0x80
#define IDE_DRDY      0x40
#define IDE_DF        0x20
#define IDE_ERR       0x01

#define IDE_CMD_READ  0x20
#define IDE_CMD_WRITE 0x30
#define IDE_CMD_RDMUL 0xc4
#define IDE_CMD_WRMUL 0xc5

#define FS_DEV 1
#define FS_BASE 0x1f0
#define FS_BASE2 0x3f0
#define SWAP_DEV 2
#define SWAP_BASE 0x170
#define SWAP_BASE2 0x370

// idequeue points to the buf now being read/written to the disk.
// idequeue->qnext points to the next buf to be processed.
// You must hold idelock while manipulating queue.

static struct spinlock idelock;
static struct buf *idequeue;

static int havedisk1;
static int havedisk2;
static void idestart(struct buf*);

// Wait for IDE disk to become ready.
static int
idewait(int checkerr, int base)
{
  int r;

  while((r = inb(base + 7)) & (IDE_BSY))
    ;
  if(checkerr && (r & (IDE_DF|IDE_ERR)) != 0)
    return -1;
  return 0;
}

void
ideinit(void)
{
  int i;

  initlock(&idelock, "ide");
  picenable(IRQ_IDE);
  ioapicenable(IRQ_IDE, ncpu - 1);
  idewait(0, FS_BASE);

  // Check if disk 1 is present
  outb(0x1f6, 0xe0 | ((FS_DEV&1)<<4));
  for(i=0; i<1000; i++){
    if(inb(0x1f7) != 0){
      havedisk1 = 1;
      break;
    }
  }

  idewait(0, SWAP_BASE);

  // Check if disk 1 is present
  outb(SWAP_BASE + 6, 0xe0 | ((SWAP_DEV&1)<<4));
  for(i=0; i<1000; i++){
    if(inb(SWAP_BASE + 7) != 0){
      havedisk2 = 1;
      break;
    }
  }

  // Switch back to disk 0.
  outb(0x1f6, 0xe0 | (0<<4));
}

// Start the request for b.  Caller must hold idelock.
static void
idestart(struct buf *b)
{
  int base;
  int base2;

  if(b->dev == 1) {
    base = FS_BASE;
    base2 = FS_BASE2;
  } else if(b->dev == 2) {
    base = SWAP_BASE;
    base2 = SWAP_BASE2;
  }

  if(b == 0)
    panic("idestart");
  if(b->blockno >= FSSIZE)
    panic("incorrect blockno");
  int sector_per_block =  BSIZE/SECTOR_SIZE;
  int sector = b->blockno * sector_per_block;
  int read_cmd = (sector_per_block == 1) ? IDE_CMD_READ :  IDE_CMD_RDMUL;
  int write_cmd = (sector_per_block == 1) ? IDE_CMD_WRITE : IDE_CMD_WRMUL;

  if (sector_per_block > 7) panic("idestart");

  idewait(0, base);
  outb(base2 + 6, 0);  // generate interrupt
  outb(base + 2, sector_per_block);  // number of sectors
  outb(base + 3, sector & 0xff);
  outb(base + 4, (sector >> 8) & 0xff);
  outb(base + 5, (sector >> 16) & 0xff);
  outb(base + 6, 0xe0 | ((b->dev&1)<<4) | ((sector>>24)&0x0f));
  if(b->flags & B_DIRTY){
    outb(base + 7, write_cmd);
    outsl(base, b->data, BSIZE/4);
  } else {
    outb(base + 7, read_cmd);
  }
}

// Interrupt handler.
void
ideintr(void)
{
  struct buf *b;

  // First queued buffer is the active request.
  acquire(&idelock);
  if((b = idequeue) == 0){
    release(&idelock);
    // cprintf("spurious IDE interrupt\n");
    return;
  }
  idequeue = b->qnext;

  // Read data if needed.
  if(!(b->flags & B_DIRTY) && idewait(1, FS_BASE) >= 0)
    insl(0x1f0, b->data, BSIZE/4);

  // Wake process waiting for this buf.
  b->flags |= B_VALID;
  b->flags &= ~B_DIRTY;
  wakeup(b);

  // Start disk on next buf in queue.
  if(idequeue != 0)
    idestart(idequeue);

  release(&idelock);
}

// For whatever reason the interupt wouldn't work with the swap disk
// Calling it manually here so that the reads and bits get set properly
void
ideintr2(struct buf *b)
{
  // Read data if needed.
  if(!(b->flags & B_DIRTY) && idewait(1, SWAP_BASE) >= 0)
    insl(SWAP_BASE, b->data, BSIZE/4);

  // Wake process waiting for this buf.
  b->flags |= B_VALID;
  b->flags &= ~B_DIRTY;
}

//PAGEBREAK!
// Sync buf with disk.
// If B_DIRTY is set, write buf to disk, clear B_DIRTY, set B_VALID.
// Else if B_VALID is not set, read buf from disk, set B_VALID.
void
iderw(struct buf *b)
{
  struct buf **pp;

  if(!holdingsleep(&b->lock))
    panic("iderw: buf not locked");
  if((b->flags & (B_VALID|B_DIRTY)) == B_VALID)
    panic("iderw: nothing to do");
  if(b->dev == 1 && !havedisk1)
    panic("iderw: ide disk 1 not present");
  if(b->dev == 2 && !havedisk2)
    panic("iderw: ide disk 2 not present");

  acquire(&idelock);  //DOC:acquire-lock

  if(b->dev == 2) {
    idestart(b);
    ideintr2(b);
    release(&idelock);
    return;
  }


  // Append b to idequeue.
  b->qnext = 0;
  for(pp=&idequeue; *pp; pp=&(*pp)->qnext)  //DOC:insert-queue
    ;
  *pp = b;

  // Start disk if necessary.
  if(idequeue == b)
    idestart(b);

  // Wait for request to finish.
  while((b->flags & (B_VALID|B_DIRTY)) != B_VALID){
    sleep(b, &idelock);
  }

  release(&idelock);
}
