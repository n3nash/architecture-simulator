#include "common.h"
#include "sim.h"
#include "trace.h" 
#include "memory.h" 
#include "knob.h"
#include "all_knobs.h"

#include <stdlib.h>
/*******************************************************************/
/* Memory Related frame */ 
/*******************************************************************/
/* You do not need to change the code in this part                 */ 


void memory_c::dprint_queues(void) 
{
  // traverse the queue and print out all the values inside the queue 

  /* print out mshr entries */ 
  
  list<m_mshr_entry_s *>::const_iterator cii; 
  
  /* do not modify print MSHR print statement here. These will be used for grading */ 

  cout <<"** cycle_count: " << cycle_count << endl; 
  cout <<"***MSHR entry print outs: mshr_size: " <<  m_mshr.size() << endl; 


  for (cii= m_mshr.begin() ; cii != m_mshr.end(); cii++) {
    
    int jj = 0; 
    cout << "mshr_entry_num: " << jj;
    jj++; 

    m_mshr_entry_s* entry = (*cii);
    
    if (entry->valid) {
      mem_req_s *req = entry->m_mem_req; 
      
      cout <<" mem_req_id: " << req->m_id; 
      cout <<" mem_addr: " << req->m_addr; 
      cout <<" size: " << (int)req->m_size; 
      cout <<" insert_time: " << entry->insert_time;
      cout <<" rdy_cycle: " << req->m_rdy_cycle; 
      cout <<" dirty: " << req->m_dirty; 
      cout <<" done: " << req->m_done; 
      cout <<" state: " << req->m_state; 
      cout <<" type:  " << req->m_type; 
      cout <<" ops_size: " << entry->req_ops.size(); 
      
      if (entry->req_ops.size()) {
	list<Op *>::const_iterator cii; 
	int kk = 0; 
	for (cii = entry->req_ops.begin() ; cii != entry->req_ops.end(); cii++) {
	  
	  Op *m_op = (*cii);
	  
	  if(m_op->mem_type == MEM_LD) {
	    printf("op[%d]:LD id: %lu ", kk, (uint64_t)m_op->inst_id);
	  }
	  else 
	    printf("op[%d]:ST id: %lu", kk, (uint64_t)m_op->inst_id);
	  	  
	}
	kk++;

      }
     
      cout << endl; 
    }
  }
  
  // print queues 

  
  cout <<"***DRAM_IN_QUEUE entry print outs ****" << endl; 
  
  list<mem_req_s *>::const_iterator cqii; 
  for (cqii = dram_in_queue.begin() ; cqii != dram_in_queue.end(); cqii++) {
    mem_req_s *req = *cqii; 
    
    cout << " req_id: " << req->m_id; 
    cout << " addr: " << req->m_addr; 
    cout << " rdy_cycle: " << req->m_rdy_cycle; 
    cout <<" state: " << req->m_state; 
    cout <<" type:  " << req->m_type; 
    cout << " ||| " ; 
  }
  
  cout << endl; 
  // end printing dram in queue 
  

  // print dram scheduler queues 
  
  list<mem_req_s *>::const_iterator clii; 
  
  cout <<"***DRAM_SCH_QUEUE entry print outs ****" << endl; 

  for (int bb = 0; bb < m_dram_bank_num; bb++) { 
    cout <<"***DRAM_SCH_QUEUE BANK[" << bb << "]" << endl; 
    
    for (clii = dram_bank_sch_queue[bb].begin(); 
	 clii != dram_bank_sch_queue[bb].end(); 
	 clii++) { 
      
      mem_req_s *req = *clii; 
      
      cout << " req_id: " << req->m_id; 
      cout << " addr: " << req->m_addr; 
      cout << " rdy_cycle: " << req->m_rdy_cycle; 
      cout <<" state: " << req->m_state; 
      cout <<" type:  " << req->m_type; 
      cout << " ||| " ; 
    
    }
    cout << endl; 
  }
  cout << endl; 
  // ending print dram scheduler 

  
  // print dram out queue 
  
  cout <<"***DRAM_OUT_QUEUE entry print outs ****" << endl; 
  for (cqii= dram_out_queue.begin() ; cqii != dram_out_queue.end(); cqii++) {
    mem_req_s *req = *cqii; 
    
    cout << " req_id: " << req->m_id; 
    cout << " addr: " << req->m_addr; 
    cout << " rdy_cycle: " << req->m_rdy_cycle; 
    cout <<" state: " << req->m_state; 
    cout <<" type:  " << req->m_type; 
    cout << " ||| " ; 
      
  }
  
  cout << endl; 
  
  
}

/*******************************************************************/
/* print dram status. debugging help function 
/*******************************************************************/

void memory_c::dprint_dram_banks()
{
  
  cout << " DRAM_BANK_STATUS cycle_count "<< cycle_count << endl; 

  for (int ii = 0; ii <m_dram_bank_num; ii++) {
    printf("bank_num[%d]: row_id:%d rdy_cycle:%lu \n",
	   ii, dram_bank_open_row[ii], (uint64_t)dram_bank_rdy_cycle[ii]); 
  }
  
}


/*******************************************************************/
/* Initialize the memory related data structures                   */
/*******************************************************************/

void memory_c::init_mem()
{
  /* For Lab #2, you do not need to modify this code */

  /* you can add other code here if you want */ 
  

  /* init mshr */ 
  m_mshr_size = KNOB(KNOB_MSHR_SIZE)->getValue(); 
  m_dram_bank_num = KNOB(KNOB_DRAM_BANK_NUM)->getValue(); 
  m_block_size = KNOB(KNOB_BLOCK_SIZE)->getValue();

  
  for (int ii = 0 ; ii < m_mshr_size; ii++) 
  {

    m_mshr_entry_s* entry = new m_mshr_entry_s; 
    entry->m_mem_req = new mem_req_s;  // create a memory rquest data structure here 
    
    entry->valid = false; 
    entry->insert_time = 0; 
    m_mshr_free_list.push_back(entry); 
  }
  
  /* init DRAM scheduler queues */ 

  dram_in_queue.clear();
  dram_out_queue.clear();

  //cout << "SIZE OF DRAM IN AND OUT" << dram_in_queue.size() << dram_out_queue.size();
  
 // cout << "INIT MEM\n";

  dram_bank_sch_queue = new list<mem_req_s*>[m_dram_bank_num];

  dram_bank_open_row = new int64_t[m_dram_bank_num];
  dram_bank_rdy_cycle = new uint64_t[m_dram_bank_num];
  for(int ii=0;ii<m_dram_bank_num;ii++)
  {
    dram_bank_open_row[ii] = -1;
    dram_bank_rdy_cycle[ii] = 0; 
  }
 
  //dram_bank_rdy_cycle[i] = -1; 
  
}

/*******************************************************************/
/* free MSHR entry (init the corresponding mshr entry  */ 
/*******************************************************************/

void memory_c::free_mshr_entry(m_mshr_entry_s *entry)
{
  entry->valid = false; 
  entry->insert_time = 0; 
  m_mshr_free_list.push_back(entry); 

}

/*******************************************************************/
/* This function is called every cycle                             */ 
/*******************************************************************/

void memory_c::run_a_cycle() 
{

  
  if (KNOB(KNOB_PRINT_MEM_DEBUG)->getValue()) {
    dprint_queues();
    dprint_dram_banks();
  }
  
  /* This function is called from run_a_cycle() every cycle */ 
  /* You do not add new code here */ 
  /* insert D-cache/I-cache (D-cache for only Lab #2) and wakes up instructions */ 
  fill_queue(); 

  /* move memory requests from dram to cache and MSHR*/ /* out queue */ 
  send_bus_out_queue(); 

  /* memory requests are scheduled */ 
  dram_schedule(); 

  /* memory requests are moved from bus_queue to DRAM scheduler */
  push_dram_sch_queue();
  
  /* new memory requests send from MSRH to in_bus_queue */ 
  send_bus_in_queue(); 

}

/*******************************************************************/
/* get a new mshr entry 
/*******************************************************************/

m_mshr_entry_s* memory_c::allocate_new_mshr_entry(void)
{
  
  if (m_mshr_free_list.empty())
    return NULL; 
  
  m_mshr_entry_s* entry = m_mshr_free_list.back(); 
  m_mshr_free_list.pop_back(); 
  m_mshr.push_back(entry); 
  
  return entry; 
  
 }






/* insert memory request into the MSHR, if there is no space return false */ 
/*******************************************************************/
/* memory_c::insert_mshr
/*******************************************************************/
bool memory_c::insert_mshr(Op *mem_op)
{

  bool insert=false; 
 
  //  step 1. create a new memory request 
  m_mshr_entry_s * entry = allocate_new_mshr_entry();
 
  //  step 2. no free entry means no space return ; 
  if (!entry) return false; // cannot insert a request into the mshr; 

  //if(mem_op->mem_type == MEM_LD) 
  entry->req_ops.push_back(mem_op);
  //else{
  //entry->req_ops.push_back(NULL); }
//latest changes here

    // step 3. initialize the memory request here 
  mem_req_s *t_mem_req = entry->m_mem_req; 
  
  if (mem_op->mem_type == MEM_LD ) { 
    t_mem_req->m_type = MRT_DFETCH; 
    t_mem_req->m_addr = mem_op->ld_vaddr; 
    t_mem_req->m_size = mem_op->mem_read_size;
  }
  else {
    t_mem_req->m_type = MRT_DSTORE; 
    t_mem_req->m_addr = mem_op->st_vaddr; 
    t_mem_req->m_size = mem_op->mem_write_size;
  }
    t_mem_req->m_rdy_cycle = 0; 
    t_mem_req->m_dirty = false; 
    t_mem_req->m_done = false; 
    t_mem_req->m_id = ++m_unique_m_count; 
    t_mem_req->m_state = MEM_NEW;
    t_mem_req->threadid = mem_op->thread_id; 
    
    entry->valid  = true; 
    entry->insert_time = cycle_count; 
    
    return true; 
}


/*******************************************************************/
/* searching for matching mshr entry using memory address 
/*******************************************************************/


m_mshr_entry_s * memory_c::search_matching_mshr(ADDRINT addr) {
  
  list<m_mshr_entry_s *>::const_iterator cii; 
  
  for (cii= m_mshr.begin() ; cii != m_mshr.end(); cii++) {
    
    m_mshr_entry_s* entry = (*cii);
    if (entry->valid) {
      mem_req_s *req = entry->m_mem_req; 
      if (req) {
	if ((req->m_addr)/m_block_size == (addr/m_block_size)) 
	  return entry; 
      }
    }
  }
  return NULL;
}

/*******************************************************************/
/* searching for matching mshr entry using memory address and return the iterator 
/*******************************************************************/

list<m_mshr_entry_s*>::iterator memory_c::search_matching_mshr_itr(ADDRINT addr) {
  
  list<m_mshr_entry_s *>::iterator cii; 
  
  for (cii= m_mshr.begin() ; cii != m_mshr.end(); cii++) {
    
    m_mshr_entry_s* entry = (*cii);
    if (entry->valid) {
      mem_req_s *req = entry->m_mem_req; 
      if (req) {
	if ((req->m_addr)/m_block_size == (addr/m_block_size)) 
	  return cii; 
      }
    }
  }
  //return NULL;
  //return cii;
}



/*******************************************************************/
/*  search MSHR entries and find a matching MSHR entry and piggyback 
/*******************************************************************/

bool memory_c::check_piggyback(Op *mem_op)
{
  bool match = false; 
  ADDRINT addr;
  if (mem_op->mem_type == MEM_LD) 
    addr = mem_op->ld_vaddr;
  else 
    addr = mem_op->st_vaddr; 
  
  m_mshr_entry_s *entry = search_matching_mshr(addr);
  
  if(!entry)
  {
    return false;
  } 
  else
  { // cout << "\nFOUND PIGGYBACKING!" ;

    //if(mem_op->mem_type == entry->m_mem_req->m_type || (mem_op->mem_type==MEM_LD && entry->m_mem_req->m_type==MRT_DSTORE)){
  // if(mem_op->mem_type==MEM_LD){

    	entry->req_ops.push_back(mem_op);
//}
   //else
//	entry->req_ops.push_back(NULL);

//	cout << "returning true"; 
    return true;
  }
}



/*******************************************************************/
/*******************************************************************/
/*******************************************************************/
/*******************************************************************/
/*******************************************************************/
/*******************************************************************/
/*****    COMPLETE the following functions               ***********/
/*******************************************************************/
/*******************************************************************/
/*******************************************************************/
/*******************************************************************/
/*******************************************************************/
/*******************************************************************/
/*******************************************************************/
/*******************************************************************/
/*******************************************************************/




/*******************************************************************/
/* send a request from mshr into in_queue 
/*******************************************************************/

void memory_c::send_bus_in_queue() {
 
   if(m_mshr.empty()) return;

 // cout << "\nIN THE FIRST QUEUE"; 

  // cout << "\n SIZE OF MSHR " << m_mshr.size();
     
   list<m_mshr_entry_s *>::iterator ii;

   for(ii=m_mshr.begin();ii!=m_mshr.end();ii++)
	{
	//cout << "mem address" << entry->m_mem_req->m_addr << " mem type is " << entry->m_mem_req->m_type;
	m_mshr_entry_s *entry = (*ii);
        if((entry)->m_mem_req->m_state == MEM_NEW){
	(entry)->m_mem_req->m_state = MEM_DRAM_IN;
        //mem_req_s *temp = ii->m_mem_req;
	//cout << "Pushing in In queue\n";
	dram_in_queue.push_back((entry)->m_mem_req);
	break;
	}

	else
		continue;
	}
   

   /* For Lab #2, you need to fill out this function */ 
  
  // *END**** This is for the TA
} 



/*******************************************************************/
/* search from dram_schedule queue and scheule a request 
/*******************************************************************/

 void memory_c::dram_schedule() {

   	//for(int ii=0;ii<KNOB(KNOB_DRAM_BANK_NUM)->getValue();ii++)
	//	if(dram_bank_sch_queue[ii].empty()) return;

	//list<mem_req_s *>::iterator ii;

	int jj=0;
	for(jj=0;jj < KNOB(KNOB_DRAM_BANK_NUM)->getValue() ;jj++){ 

	list<mem_req_s *>::iterator ii;

	for(ii=dram_bank_sch_queue[jj].begin();ii!=dram_bank_sch_queue[jj].end();ii++)
        {
	mem_req_s *entry = (*ii);

	if((entry)->m_state == MEM_DRAM_IN){
        //dram_bank_sch_queue.push_back((*ii));
//	cout << "\nmem type " << (entry->m_type); 

//	cout << "\naddress of this is " << entry->m_addr;
//	cout << "\ndram page size " << (KNOB(KNOB_DRAM_PAGE_SIZE)->getValue()*1024);

        int64_t row_buffer_id = (((entry)->m_addr)/(KNOB(KNOB_DRAM_PAGE_SIZE)->getValue()*1024));
//	cout << "\nROW BUFFER" << row_buffer_id;

//	cout << "\nstored row buffer is" << dram_bank_open_row[jj];

 	//unsigned int bank_id = (ii->m_addr/(KNOB(KNOB_DRAM_PAGE_SIZE)->getValue()*1024))%(KNOB(KNOB_DRAM_BANK_NUM)->getValue());
//	cout <<"\nvalue of jj " << jj ;

	if (dram_bank_rdy_cycle[jj] < cycle_count) // check if DRAM is idle
	{
//		cout << "\n Bank id is " << jj;
		//dram_bank_open_row for this bank compared wih row_buffer_id.
		if(dram_bank_open_row[jj] == row_buffer_id){ //if same row buffer hit
			(entry)->m_rdy_cycle = cycle_count + KNOB(KNOB_MEM_LATENCY_ROW_HIT)->getValue(); 
			dram_row_buffer_hit_count += 1;
			dram_row_buffer_hit_count_thread[entry->threadid] += 1; 
			}
		else //write back row buffer and read new .
		{
			 dram_bank_open_row[jj] = row_buffer_id ;
			(entry)->m_rdy_cycle = cycle_count + KNOB(KNOB_MEM_LATENCY_ROW_MISS)->getValue(); 
			dram_row_buffer_miss_count += 1;
			dram_row_buffer_miss_count_thread[entry->threadid] += 1;
		}	
//change all these values reflected in the sch_buffer queue so that send_bus_out can know if it is ready.
		//req->row_id = (addr/(KNOB(KNOB_DRAM_PAGE_SIZE)->getValue()*1024));
		dram_bank_rdy_cycle[jj] = (entry)->m_rdy_cycle; //write back the latest values into the corresponding dram bank
		//dram_bank_open_row[jj] = row_buffer_id ; // do the same as above
		(entry)->m_state = MEM_DRAM_SCH;
	break;	//*dii = (*ii);
   	}
	}
	}
	}
/* For Lab #2, you need to fill out this function */ 
   

}
/*******************************************************************/
/*  push_dram_sch_queue()
/*******************************************************************/

void memory_c::push_dram_sch_queue()
{
  
  	if(dram_in_queue.empty()) return;

        list<mem_req_s *>::iterator cii;

        for(cii=dram_in_queue.begin();cii!=dram_in_queue.end();)
        {
	mem_req_s* ii = (*cii);
//	cout << "\nPUSHING IN DRAM ";

	if((ii)->m_state == MEM_DRAM_IN){
//	(ii)->m_state = MEM_DRAM_SCH;
	//mem_req_s* temp = (ii);//get_new_mem_request();
	//insert temp into dram back;
	int temp = (ii)->m_addr/((KNOB(KNOB_DRAM_PAGE_SIZE))->getValue()*1024);
        int bank_id = temp%(KNOB(KNOB_DRAM_BANK_NUM)->getValue());
	//mem_req_s *temp = (ii);
        dram_bank_sch_queue[bank_id].push_back(ii);
	cii = dram_in_queue.erase(cii);
	break;
     	}
	else{
		cii++;
		continue;}
        }

  /* For Lab #2, you need to fill out this function */ 
   
}

/*******************************************************************/
/* send bus_out_queue 
/*******************************************************************/

void memory_c::send_bus_out_queue() 
{


        //list<mem_req_s *>::iterator ii;

	int jj ;
	for(jj=0;jj<KNOB(KNOB_DRAM_BANK_NUM)->getValue();jj++){

	//cout << "\nKNOB BANKS " << KNOB(KNOB_DRAM_BANK_NUM)->getValue();	
	//mem_req_s *dii = (*ii);
 	 list<mem_req_s *>::iterator ii;

	if(dram_bank_sch_queue[jj].empty())
		continue;

	//cout << "size is \n" << dram_bank_sch_queue[jj].size();

        for(ii=(dram_bank_sch_queue[jj]).begin();ii!=(dram_bank_sch_queue[jj]).end();)  //check all enteries and see which one is free.
        {
	//cout << "\nwithin OUT QUEUE of each bank";	
	mem_req_s* dii = (*ii);

        assert(dii != NULL);

//	cout <<"\nREADY CYCLES" << (*ii)->m_rdy_cycle;

        if((dii)->m_rdy_cycle < cycle_count && (dii)->m_state==MEM_DRAM_SCH){
		(dii)->m_state = MEM_DRAM_DONE;

//		cout << (dii)->m_addr << "\nis ready";
		dram_out_queue.push_back(dii); //keep pushing al entries into out queue.

		(dii)->m_state = MEM_DRAM_OUT;

		//dram_bank_rdy_cycle[jj] = 0;
		ii = dram_bank_sch_queue[jj].erase(ii);
	break;
        }
	else
		ii++;
	}
   /* For Lab #2, you need to fill out this function */ 

  
}
}
/*******************************************************************/
/*  fill_queue 
/*******************************************************************/

void memory_c::fill_queue(void) 
{

   /* For Lab #2, you need to fill out this function */ 
  /* CAUTION!: This function is not completed. Please complete this function */ 
  
  if (dram_out_queue.empty()) return; 
  
 
    mem_req_s *req = dram_out_queue.front(); 
    dram_out_queue.pop_front(); 
    
    /* search for matching mshr  entry */
   if(!(req->m_state == MEM_DRAM_OUT))
	return;

//   cout << "\nIN fill queue";
	//req->m_state = MEM_DRAM_OUT;
//   cout << "\nnumber of entries in out queue " << dram_out_queue.size();

    m_mshr_entry_s *entry = search_matching_mshr(req->m_addr); 

    dcache_insert(req->m_addr);
  
  // cout << "\ninserted into dcache " << req->m_addr;
    //in insert insert the physical address , insert the PTE as well.
 //  cout << "\nNumber of entries in req ops " << entry->req_ops.size();
    while(entry->req_ops.size()) {

      Op *w_op = new Op;
	w_op = entry->req_ops.front();
//	if(w_op->mem_type == 1)
//	dcache_insert(w_op->ld_vaddr);
//	if(w_op->mem_type == 2)
//	dcache_insert(w_op->st_vaddr);
//cout << "broadcastin now\n";
        w_op->write_flag = false;
	//if(entry->m_mem_req->m_type == MRT_DSTORE)
	//	w_op = NULL;

      broadcast_rdy_op(w_op);

      entry->req_ops.pop_front(); 
    }

   //dcache_insert(req->m_addr);

   //insert the value into the DCACHE 
    /* The following code will free mshr entry */
 
    list<m_mshr_entry_s *>::iterator mii = search_matching_mshr_itr(req->m_addr); 
    m_mshr.erase(mii); 
  
  free_mshr_entry(entry); 
  
}





/*******************************************************************/
/*  store-load forwarind features, cache addresses, cache load/store types 
/*******************************************************************/


bool memory_c::store_load_forwarding(Op *mem_op,int threadid)
{

m_mshr_entry_s *entry;

if(mem_op->mem_type == 1)
entry = search_matching_mshr(mem_op->ld_vaddr); 

if(mem_op->mem_type == 2)
entry = search_matching_mshr(mem_op->st_vaddr);

 //check the cache block address not the actual address since the whole cache block is brought in.

 if(!entry)
	return false;
 else{
	if(mem_op->mem_type ==1 && entry->m_mem_req->m_type == 2){


	ADDRINT entry_addr = entry->m_mem_req->m_addr;
	entry_addr += entry->m_mem_req->m_size;
        ADDRINT mem_addr = mem_op->ld_vaddr;
	mem_addr += mem_op->mem_read_size;
	if ((entry->m_mem_req->m_addr <= mem_op->ld_vaddr) && (entry_addr >= mem_addr)){ 
	
	store_load_forwarding_count += 1;
	store_load_forwarding_count_thread[threadid] +=1 ;

	return true;}

	return false;}

	if(mem_op->mem_type ==2 && entry->m_mem_req->m_type == 2){


        ADDRINT entry_addr = entry->m_mem_req->m_addr;
        entry_addr += entry->m_mem_req->m_size;
        ADDRINT mem_addr = mem_op->st_vaddr;
        mem_addr += mem_op->mem_write_size;
        if ((entry->m_mem_req->m_addr <= mem_op->st_vaddr) && (entry_addr >= mem_addr)){

      //  store_load_forwarding_count += 1;

        return true;}

        return false;}


	else
	return false;}
  /* For Lab #2, you need to fill out this function */ 

}
