//Author : Moinuddin K Qureshi, Prashant J Nair
//MS ECE : Georgia Institute of Technology

#include <assert.h>
#include "vmem.h"
#include<iostream>
using namespace std;

#define VMEM_MAX_THREADS     4
#define VMEM_PTE_SIZE        8
#define VMEM_PAGE_TABLE_SIZE ((1<<20)*VMEM_PTE_SIZE)


//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

tlb *tlb_new(int size){
  tlb *t = (tlb *) calloc(1, sizeof(tlb));

  t->size=size;
 // for(int i=0;i<VMEM_MAX_THREADS;i++)
//	t->s_access[i] = 0;
  t->entries = (tlb_entry *)calloc(size, sizeof(tlb_entry));

  for(int ii=0; ii<t->size; ii++)
	{
	//t->entries[ii] = (tlb_entry *)malloc(sizeof(tlb_entry));
 	t->entries[ii].vpn = 0;
	t->entries[ii].pfn = 0;
	t->entries[ii].threadid = 0;
	t->entries[ii].last_access_time = 0;
	t->entries[ii].valid = false;
	} 


return t;
	
}



//////////////////////////////////////////////////////////////
// returns 1 if TLB hit, 0 otherwise
// If hit, the PFN field is changed to the translated PFN
//////////////////////////////////////////////////////////////

bool tlb_access (tlb *t, uint64_t vpn, int threadid, uint64_t *pfn){
  bool found=0;

  for(int ii=0; ii<t->size; ii++){
    tlb_entry *entry = &t->entries[ii];
//    cout << "\nEntry in tlb is " << entry->vpn;
  //  cout << "\nValue of vpn passed " << vpn;
	if(vpn == entry->vpn)
//		cout << "MAtched\n";
    if( entry->valid &&
	(entry->threadid==threadid) &&
	(entry->vpn==vpn)){
      found=1;
      *pfn = entry->pfn;
      entry->last_access_time = t->s_access;//[threadid];
      break;
    }
  }

  t->s_access++;//[threadid]++;

  if(found==0){ 
    t->s_miss_thread[threadid]++;
    t->s_miss++;
  }

  return found;
}


//////////////////////////////////////////////////////////////
// Use this function only after TLB miss and DRAM access for PTE completes
// Get the actual PFN not from PTE but from vmem_vpn_to_pfn and then install
//////////////////////////////////////////////////////////////

void tlb_install(tlb *t, uint64_t vpn, int threadid, uint64_t  pfn){

//  tlb_entry *new_entry = (tlb_entry *)malloc(sizeof(tlb_entry));
 // new_entry->threadid = threadid;
  // new_entry->vpn = vpn;
  //  new_entry->pfn = pfn;
   //  new_entry->valid = true;
//	new_entry->last_access_time = t->s_access;

  for(int ii=0 ; ii<t->size ; ii++)
	{
	tlb_entry *entry = &t->entries[ii];
	if(!entry->valid){
	entry->threadid = threadid;
	entry->vpn = vpn;
	entry->pfn = pfn;
	entry->valid = true;
	entry->last_access_time = t->s_access;//[threadid];
	t->s_access++;//[threadid]++;
	return;
	}
	}


  uint64_t access_compare = t->entries[0].last_access_time;
  int index = 0;

  for(int ii=0; ii<t->size; ii++){
	tlb_entry *entry = &t->entries[ii];
	if( entry->valid ) {
	 if(entry->last_access_time < access_compare){
		access_compare = entry->last_access_time;
		index = ii;
	}}
	}

    t->entries[index].threadid = threadid;
    t->entries[index].vpn = vpn;

 //   cout << "\nVpn in Tlb " << vpn;
    t->entries[index].pfn = pfn;
    t->entries[index].valid = true;
 //   cout << "\nPfn in Tlb " << pfn;

    t->entries[index].last_access_time = t->s_access;//[threadid];

   t->s_access++;//[threadid]++;

  // Students to write this function.  
  // Use LRU replacement to find TLB victim
  // Initialize entry->threadid, entry->vpn, entry->pfn 
  // Hint: Check cache_install function 

}


//////////////////////////////////////////////////////////////
// This function provides PTE address for a given VPN
// On TLB miss, use this function to get the DRAM address for PTE
// Do not change this function
//////////////////////////////////////////////////////////////

uint64_t vmem_get_pteaddr(uint64_t vpn, int threadid){

  assert(threadid < VMEM_MAX_THREADS);
  
  uint64_t pte_base_addr = threadid * VMEM_PAGE_TABLE_SIZE;
  uint64_t pte_offset = (vpn%(1<<20))* VMEM_PTE_SIZE;

  uint64_t pte = pte_base_addr + pte_offset;

 // cout << "\n value of pte is " << pte;

  return (pte); // provides a byte address of PTE (change to lineaddr)
}

//////////////////////////////////////////////////////////////
// This function provides the actual VPN to PFN translation
// Access this function once you get the PTE from DRAM, and before TLB install
// Do not change this function
//////////////////////////////////////////////////////////////

uint64_t vmem_vpn_to_pfn(uint64_t vpn, int threadid){

  assert(threadid < VMEM_MAX_THREADS);

  uint64_t total_os_reserved_pages = 16384;
  uint64_t pages_per_thread = (1<<20); // 4GB per thread
  uint64_t pfn_base = threadid*pages_per_thread;
  uint64_t pfn_thread = vpn % pages_per_thread; // simple function (not realistic)
  uint64_t pfn = total_os_reserved_pages + pfn_base + pfn_thread;

  return pfn;
}
