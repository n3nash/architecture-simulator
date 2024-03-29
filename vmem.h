//Author: Moinuddin K. Qureshi, Prashant J Nair
//MS ECE ; Georgia Institute of Technology

/************ Header for virtual memory  *************/

#ifndef __VMEM_H
#define __VMEM_H

#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>

#define HW_MAX_THREAD 4
/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

typedef struct tlb_entry{
  uint64_t vpn;
  uint64_t pfn;
  uint32_t threadid; // ignore for lab3 (threadid=0 always)
  bool     valid;
  uint64_t last_access_time;
}tlb_entry;


typedef struct tlb{
  tlb_entry *entries;
  int        size; 
  uint64_t   s_access;//[HW_MAX_THREAD]; // stats
  uint64_t   s_miss;  // stats
  uint64_t   s_miss_thread[HW_MAX_THREAD];
}tlb;


/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

tlb *tlb_new(int size);
bool tlb_access (tlb *t, uint64_t vpn, int threadid, uint64_t *pfn);
void tlb_install(tlb *t, uint64_t vpn, int threadid, uint64_t  pfn);

// functions to get PTE base addr, and actual VPN to PFN mapping
uint64_t vmem_get_pteaddr(uint64_t vpn, int threadid);
uint64_t vmem_vpn_to_pfn(uint64_t vpn, int threadid);

/*********************************************************/


#endif
