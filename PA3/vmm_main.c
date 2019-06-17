#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <stdint.h>
#include <assert.h>

static struct
{
  unsigned int num_page_fault;
  unsigned int num_page_try;
  unsigned int num_tlb_miss;
  unsigned int num_tlb_try;
} perf_stat = {
  .num_page_fault = 0,
  .num_page_try = 0,
  .num_tlb_miss = 0,
  .num_tlb_try = 0
};
/*
unit8_t max 255
unit16_t max 255*255 = 65535
 */

struct tlb_entry
{
  uint8_t page_num; 
  uint8_t frame_num;
  int is_valid; // 1 is valid or not.
};

struct page_entry
{
  uint16_t frame_num;
  int is_valid; // 1 is valid or not.
};

uint8_t lookup_tlb(uint8_t page_num);
uint8_t lookup_page_table(uint8_t page_num);
uint8_t lookup_phy_mem(uint32_t phy_addr);

void page_fault_handler(uint8_t page_num);
uint32_t to_phy_addr(uint32_t virt_addr);

struct tlb_entry tlb[16]; // TLB slot 16
int tlb_fifo_idx = 0;
struct page_entry page_table[256]; // page 256
uint8_t phy_mem[256 * 256] = {0,}; // pyhsical memory size

int main(int argc, char** argv)
{
  // Clean tlb and page table.
  for (int it = 0; it < 16; ++it) {
    tlb[it].is_valid = 0;
  }
  for (int it = 0; it < 256; ++it) {
    page_table[it].is_valid = 0;
  }

  uint32_t virt_addr;
  while (scanf("%u", &virt_addr) != EOF) {
    uint32_t phy_addr = to_phy_addr(virt_addr);
    
    fprintf(stderr, "%d\n", lookup_phy_mem(phy_addr));
  }
  
  // page fault ratio, TLB hit ratio
  printf("pf: %lf\ntlb: %lf\n",
    (double)perf_stat.num_page_fault / perf_stat.num_page_try,
    (double)perf_stat.num_tlb_miss / perf_stat.num_tlb_try);

  return 0;
}

// return frame number for page number
uint8_t lookup_tlb(uint8_t page_num)
{
  perf_stat.num_tlb_try++;

  for (struct tlb_entry* it = tlb; it < tlb + 16; it++) {
    if (it->is_valid && it->page_num == page_num) {
      return it->frame_num;
    }
  }

  // if not found in TLB, access page table to get frame number
  perf_stat.num_tlb_miss++; // for calculate miss rate

  uint8_t frame_num = lookup_page_table(page_num);
  // store in TLB (FIFO page replacement)
  struct tlb_entry* it = tlb + tlb_fifo_idx;
  tlb_fifo_idx = ++tlb_fifo_idx % 16;
  it->page_num = page_num;
  it->frame_num = frame_num;
  it->is_valid = 1;

  return it->frame_num;
}
// if valid page in page table, return frame number for page number
uint8_t lookup_page_table(uint8_t page_num)
{
  if (!page_table[page_num].is_valid) {
    page_fault_handler(page_num);
    perf_stat.num_page_fault++;
  }

  assert(page_table[page_num].is_valid); // trap

  perf_stat.num_page_try++;

  return page_table[page_num].frame_num;
}

void page_fault_handler(uint8_t page_num)
{
  FILE* fp = fopen("./input/BACKINGSTORE.bin", "rb"); // read binary file(BACKINGSTORE.bin)

  char buf[256];
  if(fp == NULL) {
    fputs("file open error!", stderr);
    exit(1);
  }
  // page_num*256 because one frame size is 256 bytes
  fseek(fp, page_num*256, SEEK_SET); // file pointer move to page number from first
  fread(buf, 1, 256, fp); // read 256 bytes

  // load on memory
  for(int i = 0; i < 256; ++i) {
    phy_mem[page_num*256+i] = buf[i]; // because of one frame size 256
  }
  printf("\n");
  // write data in page table
  page_table[page_num].is_valid = 1;
  page_table[page_num].frame_num = page_num;
  
  fclose(fp);
}

uint32_t to_phy_addr(uint32_t virt_addr)
{
  uint16_t addr = ((virt_addr << 16) >> 16); 
  uint8_t page_num = (addr >> 8); 
  uint8_t offset = ((addr << 8) >> 8); 
  uint8_t frame_num = lookup_tlb(page_num); 
  uint32_t mapping_addr = (frame_num << 8) + offset;
  return mapping_addr; 
}

uint8_t lookup_phy_mem(uint32_t phy_addr)
{
  return phy_mem[phy_addr]; 
}
