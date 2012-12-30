#include "common.h"
#include "sim.h"
#include "trace.h" 
#include "cache.h"  /**** NEW-LAB2*/ 
#include "memory.h" // NEW-LAB2 
#include <stdlib.h>
#include "bpred.h"
#include "vmem.h"
#include <cmath>

#include <ctype.h> /* Library for useful character operations */
/*******************************************************************/
/* Simulator frame */ 
/*******************************************************************/

#define VMEM_PTE_SIZE 8

#define HW_MAX_THREAD 4

bool run_a_cycle(memory_c *m ); // please modify run_a_cycle function argument  /** NEW-LAB2 */ 
void init_structures(memory_c *m); // please modify init_structures function argument  /** NEW-LAB2 */ 

void init_regfile(void);

int total_cf_count=0;

/* uop_pool related variables */ 

uint32_t free_op_num;
int free_count=0;
uint32_t active_op_num; 
Op *op_pool; 
Op *op_pool_free_head = NULL; 

/* simulator related local functions */ 

bool icache_access(ADDRINT addr); /** please change uint32_t to ADDRINT NEW-LAB2 */  
bool dcache_access(ADDRINT addr); /** please change uint32_t to ADDRINT NEW-LAB2 */   
void  init_latches(void);

#include "knob.h"
#include "all_knobs.h"

// knob variables
KnobsContainer *g_knobsContainer; /* < knob container > */
all_knobs_c    *g_knobs; /* < all knob variables > */

gzFile g_stream[HW_MAX_THREAD];

void init_knobs(int argc, char** argv)
{
  // Create the knob managing class
  g_knobsContainer = new KnobsContainer();

  // Get a reference to the actual knobs for this component instance
  g_knobs = g_knobsContainer->getAllKnobs();

  // apply the supplied command line switches
  char* pInvalidArgument = NULL;
  g_knobsContainer->applyComandLineArguments(argc, argv, &pInvalidArgument);

  g_knobs->display();
}

void read_trace_file(void) {
  g_stream[0] = gzopen((KNOB(KNOB_TRACE_FILE)->getValue()).c_str(), "r");

  if ((KNOB(KNOB_RUN_THREAD_NUM)->getValue())<2) return;    /** NEW-LAB4 **/
   g_stream[1] = gzopen((KNOB(KNOB_TRACE_FILE2)->getValue()).c_str(), "r"); /** NEW-LAB4 **/
   if ((KNOB(KNOB_RUN_THREAD_NUM)->getValue())<3) return;  /** NEW-LAB4 **/
   g_stream[2] = gzopen((KNOB(KNOB_TRACE_FILE3)->getValue()).c_str(), "r"); /** NEW-LAB4 **/
   if ((KNOB(KNOB_RUN_THREAD_NUM)->getValue())<4) return;  /** NEW-LAB4 **/
   g_stream[3] = gzopen((KNOB(KNOB_TRACE_FILE4)->getValue()).c_str(), "r"); /** NEW-LAB4 **/
}
// simulator main function is called from outside of this file 

void simulator_main(int argc, char** argv) 
{
  init_knobs(argc, argv);

  // trace driven simulation 
  read_trace_file();

  /** NEW-LAB2 */ /* just note: passing main memory pointers is hack to mix up c++ objects and c-style code */  /* Not recommended at all */ 
  memory_c *main_memory = new memory_c();  // /** NEW-LAB2 */ 


  init_structures(main_memory);  // please modify run_a_cycle function argument  /** NEW-LAB2 */ 
  run_a_cycle(main_memory); // please modify run_a_cycle function argument  /** NEW-LAB2 */ 


}
int op_latency[NUM_OP_TYPE]; 

void init_op_latency(void)
{
  op_latency[OP_INV]   = 1; 
  op_latency[OP_NOP]   = 1; 
  op_latency[OP_CF]    = 1; 
  op_latency[OP_CMOV]  = 1; 
  op_latency[OP_LDA]   = 1;
  op_latency[OP_LD]    = 1; 
  op_latency[OP_ST]    = 1; 
  op_latency[OP_IADD]  = 1; 
  op_latency[OP_IMUL]  = 2; 
  op_latency[OP_IDIV]  = 4; 
  op_latency[OP_ICMP]  = 2; 
  op_latency[OP_LOGIC] = 1; 
  op_latency[OP_SHIFT] = 2; 
  op_latency[OP_BYTE]  = 1; 
  op_latency[OP_MM]    = 2; 
  op_latency[OP_FMEM]  = 2; 
  op_latency[OP_FCF]   = 1; 
  op_latency[OP_FCVT]  = 4; 
  op_latency[OP_FADD]  = 2; 
  op_latency[OP_FMUL]  = 4; 
  op_latency[OP_FDIV]  = 16; 
  op_latency[OP_FCMP]  = 2; 
  op_latency[OP_FBIT]  = 2; 
  op_latency[OP_FCMP]  = 2; 
}

void init_op(Op *op)
{
  op->num_src               = 0; 
  op->src[0]                = -1; 
  op->src[1]                = -1;
  op->dst                   = -1; 
  op->opcode                = 0; 
  op->is_fp                 = false;
  op->cf_type               = NOT_CF;
  op->mem_type              = NOT_MEM;
  op->write_flag             = 0;
  op->inst_size             = 0;
  op->ld_vaddr              = 0;
  op->st_vaddr              = 0;
  op->instruction_addr      = 0;
  op->branch_target         = 0;
  op->actually_taken        = 0;
  op->mem_read_size         = 0;
  op->mem_write_size        = 0;
  op->valid                 = FALSE;
  op->thread_id 	    = 0; 
  /* you might add more features here */ 
}


void init_op_pool(void)
{
  /* initialize op pool */ 
  op_pool = new Op [1024];
  free_op_num = 1024; 
  active_op_num = 0; 
  uint32_t op_pool_entries = 0; 
  int ii;
  for (ii = 0; ii < 1023; ii++) {

    op_pool[ii].op_pool_next = &op_pool[ii+1]; 
    op_pool[ii].op_pool_id   = op_pool_entries++; 
    init_op(&op_pool[ii]); 
  }
  op_pool[ii].op_pool_next = op_pool_free_head; 
  op_pool[ii].op_pool_id   = op_pool_entries++;
  init_op(&op_pool[ii]); 
  op_pool_free_head = &op_pool[0]; 
}


Op *get_free_op(void)
{
  /* return a free op from op pool */ 

  if (op_pool_free_head == NULL || (free_op_num == 1)) {
    std::cout <<"ERROR! OP_POOL SIZE is too small!! " << endl; 
    std::cout <<"please check free_op function " << endl; 
    assert(1); 
    exit(1);
  }

  free_op_num--;
  assert(free_op_num); 

  Op *new_op = op_pool_free_head; 
  op_pool_free_head = new_op->op_pool_next;
  //cout <<"\nValid bit " << new_op->valid;
 
  assert(!new_op->valid); 
  init_op(new_op);
  active_op_num++; 
  return new_op; 
}

void free_op(Op *op)
{
  free_op_num++;
  active_op_num--;
  free_count++; 
  //cout << "In free Op\n";
  op->valid = FALSE; 
  //cout << "Value of op valid bit " << op->valid;
  op->op_pool_next = op_pool_free_head;
  op_pool_free_head = op; 
}



/*******************************************************************/
/*  Data structure */
/*******************************************************************/

typedef struct pipeline_latch_struct {
  Op *op; /* you must update this data structure. */
  bool op_valid;
  bool used; 
   /* you might add more data structures. But you should complete the above data elements */ 
}pipeline_latch; 


typedef struct Reg_element_struct{
  bool valid;
  int count;
  // data is not needed 
  /* you might add more data structures. But you should complete the above data elements */ 
}REG_element; 

REG_element register_file[HW_MAX_THREAD][NUM_REG];

int stall_for_cycle,hit_miss_load = -1 , hit_miss_store = -1; 
bool dcache_lat_stall=false,mshr_visited = false , mshr_full = false;

/*******************************************************************/
/* These are the functions you'll have to write.  */ 
/*******************************************************************/

void FE_stage();
void ID_stage();
void EX_stage(); 
void MEM_stage(memory_c *main_memory); // please modify MEM_stage function argument  /** NEW-LAB2 */ 
void WB_stage(); 

/*******************************************************************/
/*  These are the variables you'll have to write.  */ 
/*******************************************************************/

bool br_stall[HW_MAX_THREAD] = {0};

uint64_t retired_instruction_thread[HW_MAX_THREAD] = {0};       // NEW for LAB4
uint64_t dcache_miss_count_thread[HW_MAX_THREAD] = {0};         // NEW for LAB4
uint64_t dcache_hit_count_thread[HW_MAX_THREAD] = {0};  // NEW for LAB4
uint64_t data_hazard_count_thread[HW_MAX_THREAD] = {0};         // NEW for LAB4
uint64_t control_hazard_count_thread[HW_MAX_THREAD] = {0};      // NEW for LAB4
uint64_t store_load_forwarding_count_thread[HW_MAX_THREAD] = {0};       // NEW for LAB4
uint64_t bpred_mispred_count_thread[HW_MAX_THREAD] = {0};       // NEW for LAB4
uint64_t bpred_okpred_count_thread[HW_MAX_THREAD] = {0};        // NEW for LAB4
uint64_t dtlb_hit_count_thread[HW_MAX_THREAD] = {0};    // NEW for LAB4
uint64_t dtlb_miss_count_thread[HW_MAX_THREAD] = {0};   // NEW for LAB4
uint64_t dram_row_buffer_hit_count_thread[HW_MAX_THREAD] = {0};
uint64_t dram_row_buffer_miss_count_thread[HW_MAX_THREAD] = {0};
uint64_t cycle_count_thread[HW_MAX_THREAD] = {0};

bool sim_end_condition = FALSE;     /* please complete the condition. */ 
UINT64 retired_instruction = 0;    /* number of retired instruction. (only correct instructions) */ 
UINT64 cycle_count = 0;            /* total number of cycles */ 
UINT64 data_hazard_count = 0;  
UINT64 control_hazard_count = 0; 
UINT64 icache_miss_count = 0;      /* total number of icache misses. for Lab #2 and Lab #3 */ 
UINT64 dcache_hit_count = 0;      /* total number of dcache  misses. for Lab #2 and Lab #3 */ 
UINT64 dcache_miss_count = 0;      /* total number of dcache  misses. for Lab #2 and Lab #3 */ 
UINT64 l2_cache_miss_count = 0;    /* total number of L2 cache  misses. for Lab #2 and Lab #3 */  
UINT64 dram_row_buffer_hit_count = 0; /* total number of dram row buffer hit. for Lab #2 and Lab #3 */   // NEW-LAB2
UINT64 dram_row_buffer_miss_count = 0; /* total number of dram row buffer hit. for Lab #2 and Lab #3 */   // NEW-LAB2
UINT64 store_load_forwarding_count = 0;  /* total number of store load forwarding for Lab #2 and Lab #3 */  // NEW-LAB2

//lab5 mcpat counters

UINT64 total_floating_operations;
UINT64 total_integer_operations;
UINT64 total_branch_instructions;
UINT64 total_load_instructions;
UINT64 total_store_instructions;

UINT64 total_scheduler_accesses;
UINT64 reads_integer_operations;
UINT64 reads_floating_operations;
UINT64 total_multiply_instructions;
UINT64 total_dcache_reads;
UINT64 total_dcache_writes;
UINT64 total_memory_accesses;

UINT64 writes_integer_operations;
UINT64 writes_floating_operations;


int trace_visited = 0;
bool flag = true,set_release = false;
int EX_latency_countdown = 0;
bool control_stall[HW_MAX_THREAD] = {0};
bool data_stall[HW_MAX_THREAD] = {0};
bool read_trace_error = false;
int mshr_entries = 0,found=0;
int ii=-1;

list<Op*>retire_list;
vector<pipeline_latch*>fetch_list;

UINT64 fetch_arbiter_FEStage = 0;
UINT64 fetch_arbiter_IDStage = 0;

uint64_t bpred_mispred_count = 0;  /* total number of branch mispredictions */  // NEW-LAB3
uint64_t bpred_okpred_count = 0;   /* total number of correct branch predictions */  // NEW-LAB3
uint64_t dtlb_hit_count = 0;       /* total number of DTLB hits */ // NEW-LAB3
uint64_t dtlb_miss_count = 0;      /* total number of DTLB miss */ // NEW-LAB3

bpred *branchpred; // NEW-LAB3 (student need to initialize)
tlb *dtlb;        // NEW-LAB3 (student need to intialize)
Op* tlb_op;

bool control_stall_mispred = false,mshr_full_tlb = false,first_cycle = false;
uint64_t vaddress = 0;
bool stall_for_tlb = false;
bool tlb_received = false;;

bool stall_for_memory = false;

bool visited_tlb = false,dcache_lat_stall_2=false;
bool pte = false,update_once = false,found_pte=false;
uint64_t pfn=0,vpn=0,actual_physical_address=0;

int hit_miss=-1;
int taken=0;

pipeline_latch *MEM_latch;  
pipeline_latch *EX_latch;
pipeline_latch *ID_latch;
pipeline_latch *FE_latch;
//pipeline_latch *fetch_list;
UINT64 ld_st_buffer[LD_ST_BUFFER_SIZE];   /* this structure is deprecated. do not use */ 
UINT64 next_pc; 

Cache *data_cache = new Cache();  // NEW-LAB2 

/*******************************************************************/
/*  Print messages  */
/*******************************************************************/

/*void print_stats() {
  std::ofstream out((KNOB(KNOB_OUTPUT_FILE)->getValue()).c_str());
  // Do not modify this function. This messages will be used for grading
  out << "Total instruction: " << retired_instruction << endl; 
  out << "Total cycles: " << cycle_count << endl; 
  float ipc = (cycle_count ? ((float)retired_instruction/(float)cycle_count): 0 );
  out << "IPC: " << ipc << endl; 
  out << "Total I-cache miss: " << icache_miss_count << endl; 
  out << "Total D-cache hit: " << dcache_hit_count << endl; 
  out << "Total D-cache miss: " << dcache_miss_count << endl; 
  out << "Total L2-cache miss: " << l2_cache_miss_count << endl; 
  out << "Total data hazard: " << data_hazard_count << endl;
  out << "Total control hazard : " << control_hazard_count << endl; 
  out << "Total DRAM ROW BUFFER Hit: " << dram_row_buffer_hit_count << endl;  // NEW-LAB2
  out << "Total DRAM ROW BUFFER Miss: "<< dram_row_buffer_miss_count << endl;  // NEW-LAB2 
  out <<" Total Store-load forwarding: " << store_load_forwarding_count << endl;  // NEW-LAB2 

  out.close();
}*/

void print_stats() {
  std::ofstream out((KNOB(KNOB_OUTPUT_FILE)->getValue()).c_str());

  std::ofstream out_mcpat(("power_counter.txt"));
  /* Do not modify this function. This messages will be used for grading */
  out << "Total instruction: " << retired_instruction << endl;
  out << "Total cycles: " << cycle_count << endl;
  float ipc = (cycle_count ? ((float)retired_instruction/(float)cycle_count): 0 );
  out << "IPC: " << ipc << endl;
  out << "Total I-cache miss: " << icache_miss_count << endl;
  out << "Total D-cache miss: " << dcache_miss_count << endl;
  out << "Total D-cache hit: " << dcache_hit_count << endl;
  out << "Total data hazard: " << data_hazard_count << endl;
  out << "Total control hazard : " << control_hazard_count << endl;
  out << "Total DRAM ROW BUFFER Hit: " << dram_row_buffer_hit_count << endl;
  out << "Total DRAM ROW BUFFER Miss: "<< dram_row_buffer_miss_count << endl;
  out <<" Total Store-load forwarding: " << store_load_forwarding_count << endl;

  // new for LAB3
  out << "Total Branch Predictor Mispredictions: " << bpred_mispred_count << endl;
  out << "Total Branch Predictor OK predictions: " << bpred_okpred_count << endl;
  out << "Total DTLB Hit: " << dtlb_hit_count << endl;
  out << "Total DTLB Miss: " << dtlb_miss_count << endl;

  total_memory_accesses = dcache_miss_count;

  out << endl << endl << endl;

    out_mcpat << "TOTAL INSTRUCTIONS " << retired_instruction << endl;
    out_mcpat << "TOTAL SIMULATION CYCLES " << cycle_count << endl;
    
    out_mcpat << "TOTAL FLOATING OPERATIONS " << total_floating_operations << endl;
    out_mcpat << "TOTAL INTEGER OPERATIONS " << total_integer_operations << endl;
    out_mcpat << "TOTAL BRANCH INSTRUCTIONS " << total_branch_instructions << endl;
    out_mcpat << "TOTAL LOAD OPERATIONS " << total_load_instructions << endl;
    out_mcpat << "TOTAL STORE OPERATIONS " << total_store_instructions << endl;
    out_mcpat << "TOTAL ICACHE ACCESSES " << retired_instruction << endl;
    out_mcpat << "TOTAL MISPREDICTIONS " << bpred_mispred_count << endl;

    out_mcpat << "TOTAL SCHEDULER ACCESSES " << total_scheduler_accesses << endl;
    out_mcpat << "REGISTER READS INTEGER OPERATIONS " << reads_integer_operations << endl;
    out_mcpat << "REGISTER READS FLOATING OPERATIONS " << reads_floating_operations << endl;
    out_mcpat << "TOTAL MULTIPLY INSTRUCTIONS " << total_multiply_instructions << endl;
    out_mcpat << "TOTAL DCACHE READS " << total_dcache_reads << endl;
    out_mcpat << "TOTAL DCACHE WRITES " << total_dcache_writes << endl;
    out_mcpat << "TOTAL TLB ACCESSES " << total_dcache_reads + total_dcache_writes << endl;
    out_mcpat << "TOTAL MEMORY ACCESSES " << total_memory_accesses << endl;
    out_mcpat << "REGISTER WRITES INTEGER OPERATIONS " << writes_integer_operations << endl;
    out_mcpat << "REGISTER WRITES FLOATING OPERATIONS " << writes_floating_operations << endl;


  for (int ii = 0; ii < (KNOB(KNOB_RUN_THREAD_NUM)->getValue()); ii++ ) {
    out << "THREAD instruction: " << retired_instruction_thread[ii] << " Thread id: " << ii << endl;
    float thread_ipc = (cycle_count ? ((float)retired_instruction_thread[ii]/(float)cycle_count): 0 );
    out << "THREAD IPC: " << thread_ipc << endl;
  //  out << "cycles : " << cycle_count_thread[ii] << endl;
    out << "THREAD D-cache miss: " << dcache_miss_count_thread[ii] << " Thread id: " << ii << endl;
    out << "THREAD D-cache hit: " << dcache_hit_count_thread[ii] << " Thread id: " << ii << endl;
    out << "THREAD data hazard: " << data_hazard_count_thread[ii] << " Thread id: " << ii <<   endl;
    out << "THREAD control hazard : " << control_hazard_count_thread[ii] << " Thread id: " << ii << endl;
    out << "THREAD Store-load forwarding: " << store_load_forwarding_count_thread[ii] << " Thread id: " << ii << endl;
    out << "THREAD Branch Predictor Mispredictions: " << bpred_mispred_count_thread[ii] << " Thread id: " << ii << endl;
    out << "THREAD Branch Predictor OK predictions: " << bpred_okpred_count_thread[ii] << " Thread id: " << ii << endl;
    out << "THREAD DTLB Hit: " << dtlb_hit_count_thread[ii] << " Thread id: " << ii << endl;
    out << "THREAD DTLB Miss: " << dtlb_miss_count_thread[ii] << " Thread id: " << ii << endl;

    

    
 //   out << "THREAD ROW BUFFER Hit: " << dram_row_buffer_hit_count_thread[ii] << " Thread id: " << ii << endl;
   // out << "THREAD ROW BUFFER Miss: " << dram_row_buffer_miss_count_thread[ii] << " Thread id: " << ii << endl;
  }

  out.close();
  out_mcpat.close();
}


/*******************************************************************/
/*  Support Functions  */ 
/*******************************************************************/


int get_op(Op *op)
{
  static UINT64 unique_count = 1; 
  static UINT64 fetch_arbiter; 
  
  Trace_op trace_op; 
  bool success = FALSE; 
  // read trace 
  // fill out op info 
  // return FALSE if the end of trace
  int read_size;
  int fetch_id = -1;
  bool data_stall_true = false; 
  bool br_stall_fail = false; 
  // read trace 
  // fill out op info 
  // return FALSE if the end of trace
//  cout << "THreads " << KNOB(KNOB_RUN_THREAD_NUM)->getValue(); 
 /* for (int jj = 0; jj < (KNOB(KNOB_RUN_THREAD_NUM)->getValue()); jj++) {
    fetch_id = (fetch_arbiter++%(KNOB(KNOB_RUN_THREAD_NUM)->getValue()));
//	cout << "in get -OP \n";

    if (control_stall[fetch_id]) {
    br_stall_fail = true;
//	cout << "control stall \n" ;
    continue; 
    }

    if(data_stall[fetch_id]) {
    data_stall_true = true;
//	cout << "data stall \n";
    continue;
    }*/
    fetch_id = op->thread_id;

    read_size = gzread(g_stream[fetch_id], &trace_op, sizeof(Trace_op));
    success = read_size>0;
//	        if((uint8_t)op->opcode && (op->opcode < 255) && (op->opcode > 0))
                op->valid  = TRUE;  
  //      else
    //            op->valid = FALSE;

    if(read_size!=sizeof(Trace_op) && read_size>0) {

/*	if(KNOB(KNOB_RUN_THREAD_NUM)->getValue() - trace_visited > 1){
	if(fetch_id == 0){
		g_stream[0] = gzopen((KNOB(KNOB_TRACE_FILE)->getValue()).c_str(), "r");
		trace_visited++;}
   	if(fetch_id == 1){
	g_stream[1] = gzopen((KNOB(KNOB_TRACE_FILE2)->getValue()).c_str(), "r");
	trace_visited++;
	}}
*/
//	if (gzwrite(g_stream[fetch_id], trace_op,sizeof(Trace_op)) != sizeof(Trace_op))
  //  		cout << "Error when writting instruction " << inst_count << endl;	

    //  cycle_count_thread [ fetch_id ] = cycle_count ; 
      printf( "ERROR!! gzread reads corrupted op! @cycle:%d\n", cycle_count);
      success = false;
    }

    /* copy trace structure to op */ 
    if (success) {

	if (KNOB(KNOB_PRINT_INST)->getValue())
  	dprint_trace(&trace_op); 
	copy_trace_op(&trace_op, op); 

	op->inst_id  = unique_count++;
//	if((uint8_t)op->opcode && (op->opcode < 255) && op->opcode > 0)
		op->valid  = TRUE; 
//	else
//		op->valid = FALSE;
	op->thread_id = fetch_id; 
	    return success;  // get op so return 
    }
    
	// if not success and go to another trace. 
//}
if (br_stall_fail) return -1; 
if (data_stall_true) return -1;
return success; 
}


/* return op execution cycle latency */ 

int get_op_latency (Op *op) 
{
  assert (op->opcode < NUM_OP_TYPE); 
  return op_latency[op->opcode];
}

/* Print out all the register values */ 
void dump_reg() {
  for (int ii = 0; ii < NUM_REG; ii++) {
    //std::cout << cycle_count << ":register[" << ii  << "]: V:" << register_file[ii].valid << endl; 
  }
}

void print_pipeline() {
  std::cout << "--------------------------------------------" << endl; 
  std::cout <<"cycle count : " << dec << cycle_count << " retired_instruction : " << retired_instruction << endl; 
  std::cout << (int)cycle_count << " FE: " ;
  int i=0;
  for(i=0;i<HW_MAX_THREAD;i++){
  if (fetch_list[i]->op_valid) {
    Op *op = FE_latch->op;
    cout << fetch_list[i]->op->inst_id; 
    cout << "/" << "t" << i;
    //cout << (int)op->inst_id ;
  }
  else {
    cout <<"####";
  }}
  std::cout << " ID: " ;
  if (ID_latch->op_valid) {
    Op *op = ID_latch->op; 
    cout << (int)op->inst_id ;
    cout << "/" << "t" << ID_latch->op->thread_id;
  }
  else {
    cout <<"####";
  }
  std::cout << " EX: " ;
  if (EX_latch->op_valid) {
    Op *op = EX_latch->op; 
    cout << (int)op->inst_id ;
    cout << "/" << "t" << EX_latch->op->thread_id;
  }
  else {
    cout <<"####";
  }

 
  std::cout << " MEM: " ;
  if(!retire_list.empty())
	{
	list<Op*>::iterator loop;
	for(loop=retire_list.begin();loop!=retire_list.end();loop++)
		{
		Op* temp = (*loop);
                if(temp!=NULL && temp->write_flag){
			cout << temp->inst_id;
        	 }
		}
	}
  if (MEM_latch->op_valid) {
    Op *op = MEM_latch->op; 
    cout << (int)op->inst_id ;
    cout << "/" << "t" << MEM_latch->op->thread_id;
  }
  else {
    cout <<"####";
  }
  cout << endl; 
  //  dump_reg();   
  std::cout << "--------------------------------------------" << endl; 
}


void print_heartbeat()
{
  static uint64_t last_cycle_thread[HW_MAX_THREAD] ;
  static uint64_t last_inst_count_thread[HW_MAX_THREAD];

  for (int ii = 0; ii < (KNOB(KNOB_RUN_THREAD_NUM)->getValue()); ii++ ) {

    float temp_ipc = float(retired_instruction_thread[ii] - last_inst_count_thread[ii]) /(float)(cycle_count-last_cycle_thread[ii]) ;
    float ipc = float(retired_instruction_thread[ii]) /(float)(cycle_count) ;
    /* Do not modify this function. This messages will be used for grading */
    cout <<"**Heartbeat** cycle_count: " << cycle_count << " inst:" << retired_instruction_thread[ii] << " IPC: " << temp_ipc << " Overall IPC: " << ipc << " thread_id " << ii << endl;
    last_cycle_thread[ii] = cycle_count;
    last_inst_count_thread[ii] = retired_instruction_thread[ii];
  }
}

/*******************************************************************/
/*                                                                 */
/*******************************************************************/

bool run_a_cycle(memory_c *main_memory){   // please modify run_a_cycle function argument  /** NEW-LAB2 */ 

  for (;;) { 
    if (((KNOB(KNOB_MAX_SIM_COUNT)->getValue() && (cycle_count >= KNOB(KNOB_MAX_SIM_COUNT)->getValue())) || 
      (KNOB(KNOB_MAX_INST_COUNT)->getValue() && (retired_instruction >= KNOB(KNOB_MAX_INST_COUNT)->getValue())) ||  (sim_end_condition))) { 
        // please complete sim_end_condition 
        // finish the simulation 
        print_heartbeat(); 
        print_stats();
        return TRUE; 
    }
    cycle_count++; 
    if (!(cycle_count%5000)) {
      print_heartbeat(); 
    }

    main_memory->run_a_cycle();          // *NEW-LAB2 
    
    WB_stage(); 
    MEM_stage(main_memory);  // please modify MEM_stage function argument  /** NEW-LAB2 */ 
    EX_stage();
    ID_stage();
    FE_stage(); 
    if (KNOB(KNOB_PRINT_PIPE_FREQ)->getValue() && !(cycle_count%KNOB(KNOB_PRINT_PIPE_FREQ)->getValue())) print_pipeline();
  }
  return TRUE; 
}


/*******************************************************************/
/* Complete the following fuctions.  */
/* You can add new data structures and also new elements to Op, Pipeline_latch data structure */ 
/*******************************************************************/

void init_structures(memory_c *main_memory) // please modify init_structures function argument  /** NEW-LAB2 */ 
{
  init_op_pool(); 
  init_op_latency();
  init_regfile();
  /* please initialize other data stucturs */ 
  /* you must complete the function */
  stall_for_cycle = KNOB(KNOB_DCACHE_LATENCY)->getValue();

  main_memory->init_mem();

  branchpred = bpred_new((bpred_type)KNOB(KNOB_BPRED_TYPE)->getValue(),KNOB(KNOB_BPRED_HIST_LEN)->getValue(),KNOB(KNOB_PERCEP_HIST_LEN)->getValue(),KNOB(KNOB_PERCEP_TABLE_ENTRY)->getValue(), KNOB(KNOB_PERCEP_CTR_BITS)->getValue(), KNOB(KNOB_PERCEP_THRESH_OVD)->getValue());

  cout << "perceptron length" << KNOB(KNOB_PERCEP_HIST_LEN)->getValue(); 
  dtlb = tlb_new(KNOB(KNOB_TLB_ENTRIES)->getValue()); 

  init_latches();
   cache_init(data_cache,KNOB(KNOB_DCACHE_SIZE)->getValue(), 64,KNOB(KNOB_DCACHE_WAY)->getValue() , "lru_cache");
}

void WB_stage() {

//cout << "Number of freed instruction " << free_count <<"Number of retired " <<retired_instruction;
	
    if(!retire_list.empty()){
	//cout << "list size has increases \n";
	//if(set_release){
	list<Op*>::iterator kk;
	int i=0;
	int size = retire_list.size();
	while(i < size)
	//for(kk=retire_list.begin();kk!=retire_list.end();kk++)
        {
		if(retire_list.front() && retire_list.front()->write_flag){
			//cout << "Trying to retire\n";
			    if( retire_list.front()->dst!= -1 && !retire_list.front()->is_tlb) {
      register_file[retire_list.front()->thread_id][retire_list.front()->dst ].count--;
      if( register_file[retire_list.front()->thread_id][ retire_list.front()->dst ].count==0 ) {
        register_file[retire_list.front()->thread_id][ retire_list.front()->dst ].valid = true;
        if(data_stall[retire_list.front()->thread_id])
          data_stall[retire_list.front()->thread_id] = false;
	}
	if(control_stall[retire_list.front()->thread_id])
	  control_stall[retire_list.front()->thread_id] = false;
//	if(data_stall)
//		data_stall = false;
    }		
	//	cout << "\nMem Type of retiring instruction" << retire_list.front()->mem_type <<"and its address is" << retire_list.front()->ld_vaddr;
//		if(retire_list.front()->mem_type==MEM_LD && retire_list.front()->write_flag)	
		

		if(!retire_list.front()->is_tlb){

         /*       if(retire_list.front()->mem_type == 1)
			total_load_instructions += 1;
		else if(retire_list.front()->mem_type == 2)
			total_store_instructions += 1;
*/
		retired_instruction++;
		retired_instruction_thread[retire_list.front()->thread_id]++;
		//cout << "writeback\n";
		}
		else{
        //        cout <<"came back from memory \n";
                //do we need to stall here again to access pte from dcache and then insert into tlb??
                //uint64_t pfn_2 = vmem_vpn_to_pfn(vpn,0);
//		  cout << "got tlb back\n";
		//  cout << "TLB OP\n";
		  stall_for_tlb=false;
		  stall_for_memory = false;
		  retire_list.front()->is_tlb = false;
		  tlb_received = true;}


//		cout << "freeeing instruction in retire list\n";
//		retire_list.front()->op_valid = false;
		free_op(retire_list.front());

		retire_list.front() = NULL;
		//kk = retire_list.erase(kk);
                retire_list.pop_front();
		flag = true;}
		i++;
                }
		//set_release = false;
	//	}
	//	set_release = true;
		}
	if(!retire_list.empty())
		{
		list<Op*>::iterator ii;
		for(ii=retire_list.begin();ii!=retire_list.end();ii++){
	//	cout << "iterating through retire list\n";
		Op* temp = (*ii);
		if(temp!=NULL){
//		if(temp->is_tlb){
//		uint64_t  vpn = (vaddress/KNOB(KNOB_VMEM_PAGE_SIZE)->getValue());
  //              cout <<"came back from memory \n";
                //do we need to stall here again to access pte from dcache and then insert into tlb??
    //            uint64_t pfn_2 = vmem_vpn_to_pfn(vpn,0);
//			tlb_install(dtlb,vpn,0,pfn_2);}
	//		cout <<" setting values in retire list to true\n" ;
			temp->write_flag = true;}
		}}

  if( MEM_latch->op_valid == true && !dcache_lat_stall && !stall_for_memory && !stall_for_tlb && !tlb_received) {
int i=0;
//	for(i=0;i<HW_MAX_THREAD;i++)
//		cout << "\ndata stall for thread " << i << "is " << data_stall[i];

 // cout << "\n writeback for thread id " << MEM_latch->op->thread_id;

    if((MEM_latch->op->opcode == OP_CF) && control_stall[MEM_latch->op->thread_id] && KNOB(KNOB_USE_BPRED)->getValue() == 0){
	control_stall[MEM_latch->op->thread_id] = false;
    //	cout << "releasing control stall \n";
//	br_stall[MEM_latch->op->thread_id] = false;
	}
     else if(MEM_latch->op->cf_type == CF_CBR && control_stall[MEM_latch->op->thread_id] && MEM_latch->op->mispredicted && KNOB(KNOB_USE_BPRED)->getValue() == 1){
        control_stall[MEM_latch->op->thread_id] = false;
//	br_stall[MEM_latch->op->thread_id] = false;
	}
//	if(control_stall[MEM_latch->op->thread_id])
//		control_stall[MEM_latch->op->thread_id] = false;
  	
    if( MEM_latch->op->dst!= -1 ) {
      register_file[MEM_latch->op->thread_id][ MEM_latch->op->dst ].count--;
      if( register_file[MEM_latch->op->thread_id][ MEM_latch->op->dst ].count==0 ) {
        register_file[MEM_latch->op->thread_id][ MEM_latch->op->dst ].valid = true;
        if(data_stall[MEM_latch->op->thread_id]){
	//	cout << "\nreleasing dependency";
          data_stall[MEM_latch->op->thread_id] = false;}
      } 
    }

//	cout << "\nretired instrucions " << retired_instruction << "cycle count " << cycle_count;
    retired_instruction_thread[MEM_latch->op->thread_id]++;
	
    retired_instruction++;
  
 /*   if(MEM_latch->op->mem_type == 1)
              total_load_instructions += 1;
    else if(MEM_latch->op->mem_type == 2)
              total_store_instructions += 1;
*/

    MEM_latch->op->mispredicted = false;
    MEM_latch->op_valid = false;
 //  cout << "\nFreeing_memtype "<< MEM_latch->op->mem_type << "\nWith mem address " << MEM_latch->op->st_vaddr<< MEM_latch->op->ld_vaddr ;
    free_op(MEM_latch->op);
  //  cout << "retired instruction \n";
    //MEM_latch->op = NULL;
  }
}

void MEM_stage(memory_c *main_memory)
{ // please modify MEM_stage function argument  /** NEW-LAB2 */

//  cout << "came here in memory stage\n";

  MEM_latch->op = EX_latch->op;
  mshr_entries = main_memory->m_mshr.size();
  

  //cout << "\nsize of MSHR" << main_memory->m_mshr.size();
  //cout << "\ndstall value" << dcache_lat_stall ;
  //cout <<"\nVAlue of valid bit" << MEM_latch->op_valid;
  if(MEM_latch->op!=NULL){
 // cout << "MEM TYPE\n";
//  cout << "MEM_latch type" << MEM_latch->op->mem_type;
//	cout << "\n Value of stall for memory " << stall_for_memory;
}

  if(!dcache_lat_stall){
  MEM_latch->op_valid = EX_latch->op_valid;
   }


   if(stall_for_memory)
	return;

  if(MEM_latch->op!=NULL){

  // 	cout << "\nValue of control stall " << control_stall[0];
//	cout << "\nValue of MEM_latch op" << MEM_latch->op;
//	cout << "\nValue ofvalid bit " << MEM_latch->op_valid;

//	cout << "\n Value of stall_For_tlb " << stall_for_tlb;
  //                            cout << "\nValue of stall for memory " << stall_for_memory;
    //                          cout << "\nValue of op valid " << MEM_latch->op_valid;
      //                        cout << "\nValue of hit miss" << hit_miss;
        //                      cout << "\nValue of Dcache stall " << dcache_lat_stall;
          //                    cout << "\nValue of tlb recevied " << tlb_received;

//	cout << "\nValue of EX_latch op" << EX_latch->op;

//	uint64_t vpn = (MEM_latch->op->ld_vaddr/KNOB(KNOB_VPN_PAGE_SIZE)->getValue());  address is ld / st

	//int pte = vmem_get_pteaddr(vpn,0);

	if(KNOB(KNOB_ENABLE_VMEM)->getValue() && !dcache_lat_stall){
	//int actual_physical_address = pfn<<log(virutal_addr % page_size) + virtual_adddr %page_size;
	if(MEM_latch->op->mem_type == 1)
		vaddress = MEM_latch->op->ld_vaddr;
	else if(MEM_latch->op->mem_type == 2)
		vaddress = MEM_latch->op->st_vaddr; 
	else 
		return;
	//	vaddress = MEM_latch->op->instruction_addr;

//	cout << "\nvaddresss is" << vaddress;

	vpn = (uint64_t)(vaddress/KNOB(KNOB_VMEM_PAGE_SIZE)->getValue());
//	cout << "\nvpn is " << vpn;

	//int hit_miss;

	if(!tlb_received && !dcache_lat_stall && !stall_for_memory){
//		cout << "came in tlb check\n";
		if(!mshr_full_tlb){
//		MEM_latch->op_valid = false;
//		cout << "came in tlb\n";
		if(!visited_tlb){
//			cout << "\nthread id in tlb " << MEM_latch->op->thread_id;
			hit_miss = tlb_access(dtlb,vpn,MEM_latch->op->thread_id,&pfn);
//			cout << "\n Value of hit inside TLB access" << hit_miss;
			if(hit_miss){
			hit_miss = -1;
//			cout << "\nTLB hit";
			uint64_t temp_pfn = pfn;
			actual_physical_address = (uint64_t)((temp_pfn<<(uint64_t)log2(KNOB(KNOB_VMEM_PAGE_SIZE)->getValue())) + (vaddress%KNOB(KNOB_VMEM_PAGE_SIZE)->getValue()));
//			cout << "\nactual address is" << actual_physical_address;
//			actual_physical_address = (uint64_t)((temp_pfn*KNOB(KNOB_VMEM_PAGE_SIZE)->getValue())+(vaddress%KNOB(KNOB_VMEM_PAGE_SIZE)->getValue()));
			dtlb_hit_count_thread[MEM_latch->op->thread_id] += 1;
			dtlb_hit_count +=1;}
			else
				visited_tlb = true;
			pfn=0;
//			cout << "\n visited tlb with exiting value of visited_tlb =" << visited_tlb;
			}
		if(!hit_miss){
			stall_for_tlb = true;
			//dcache_lat_stall_2 = true;
			//if(!update_once){
		//	  	int thread = (int)MEM_latch->op->thread_id;	
		//		uint64_t thread2 = MEM_latch->op->thread_id;
				dtlb_miss_count_thread[MEM_latch->op->thread_id] = dtlb->s_miss_thread[MEM_latch->op->thread_id];//??;
				dtlb_miss_count = dtlb->s_miss;
				//update_once = true;
				//}
//			cout <<"\nmissed in tlb , trying pte in dcache";
//			cout << "\nvalue of stall_for_cycle is " << stall_for_cycle;
		//	MEM_latch->op_valid = false;

			if(stall_for_cycle == 1 && !pte){
			if(dcache_access(vmem_get_pteaddr(vpn,MEM_latch->op->thread_id))){
				dcache_hit_count_thread[MEM_latch->op->thread_id] += 1;
				dcache_hit_count += 1;
				first_cycle = true;
//				cout << "should find pte\n";
				found_pte = true;}
				else{
					dcache_miss_count += 1;
					dcache_miss_count_thread[MEM_latch->op->thread_id] += 1;
					found_pte = false;
					}
				stall_for_cycle = KNOB(KNOB_DCACHE_LATENCY)->getValue();
				stall_for_tlb = false;
				pte = true;}
			else if (stall_for_cycle > 1 && !pte){
				stall_for_cycle-=1;
				return;}

			if(found_pte){
//				cout << "\nfound pte in dcache";
				//dcache_lat_stall_2 = true;
		//		MEM_latch->op_valid = true;
			//	if(stall_for_cycle == 1 && pte){
				//	dcache_lat_stall_2 = false;
				//	if(first_cycle){
					uint64_t pfn_2 = vmem_vpn_to_pfn(vpn,MEM_latch->op->thread_id);
					tlb_install(dtlb,vpn,MEM_latch->op->thread_id,pfn_2);
				//		first_cycle = false;
				//		return;
				//		}
					//tlb_access(dtlb,vpn,0,&pfn); //do we need this
					uint64_t temp_pfn = pfn_2;
					stall_for_cycle = KNOB(KNOB_DCACHE_LATENCY)->getValue();
					actual_physical_address = (uint64_t)((temp_pfn<<(uint64_t)log2(KNOB(KNOB_VMEM_PAGE_SIZE)->getValue())) + vaddress%KNOB(KNOB_VMEM_PAGE_SIZE)->getValue());
//					actual_physical_address = (uint64_t)((temp_pfn*KNOB(KNOB_VMEM_PAGE_SIZE)->getValue())+(vaddress%KNOB(KNOB_VMEM_PAGE_SIZE)->getValue()));
					update_once = false;
					visited_tlb = false;
					found_pte = false;
					stall_for_tlb = false;
					hit_miss = -1;
					pte=false;
				//	}
				//else if(stall_for_cycle > 1 && pte){
				//	stall_for_cycle -= 1;
				//	return; }
				}
			else{
				hit_miss = -1;
				//dcache_miss_count += 1;
//				cout << "\nMissed pte in dcache";	
				tlb_op = get_free_op();
				tlb_op->is_tlb = true;
				tlb_op->thread_id = MEM_latch->op->thread_id;
				tlb_op->mem_type = MEM_LD;
				tlb_op->mem_read_size = VMEM_PTE_SIZE;
				tlb_op->ld_vaddr = vmem_get_pteaddr(vpn,MEM_latch->op->thread_id);
				//MEM_latch->op_valid = false;

				if(main_memory->store_load_forwarding(tlb_op,MEM_latch->op->thread_id))
				{
					visited_tlb = false;
//					cout << "\ntried store load forwarding";
					stall_for_tlb = false;
					stall_for_memory = false;
					//MEM_latch->op_valid = false;
					uint64_t pfn_2 = vmem_vpn_to_pfn(vpn,MEM_latch->op->thread_id);
                                       // tlb_install(dtlb,vpn,0,pfn_2);
                                       // tlb_access(dtlb,vpn,0,&pfn); //do we need this
                                        uint64_t temp_pfn = pfn_2;
                                        actual_physical_address = (uint64_t)(temp_pfn<<(uint64_t)log2(KNOB(KNOB_VMEM_PAGE_SIZE)->getValue())) + vaddress%KNOB(KNOB_VMEM_PAGE_SIZE)->getValue();
//					actual_physical_address = (uint64_t)((temp_pfn*KNOB(KNOB_VMEM_PAGE_SIZE)->getValue())+(vaddress%KNOB(KNOB_VMEM_PAGE_SIZE)->getValue()));
					tlb_op->is_tlb = false;
					free_op(tlb_op);
					pte = false;
				}
				else if(main_memory->check_piggyback(tlb_op)){
					stall_for_memory = true;
//					cout << "\ntried piggyback";
					}
					
				else{
					if(main_memory->m_mshr.size() >= KNOB(KNOB_MSHR_SIZE)->getValue()){
						//MEM_latch->op_valid =false;
						mshr_full_tlb = true;
//						cout << "\nMshr GREATER";
						//stall_for_memory = true;
						return;}
					main_memory->insert_mshr(tlb_op);
					stall_for_memory = true;
//					cout << "\n inserted into MSHR";
				}
//				cout << "\n Value of visited_tlb " << visited_tlb;
//				cout << "\n Value of stall_For_tlb " << stall_for_tlb;
//				cout << "\nValue of stall for memory " << stall_for_memory;
//				cout << "\nValue of op valid " << MEM_latch->op_valid;
//				cout << "\nValue of hit miss" << hit_miss;
//				cout << "\nValue of Dcache stall " << dcache_lat_stall;
//				cout << "\nValue of tlb recevied " << tlb_received;

				 }
			}
			}//mshr_full
			else
				{
				if(stall_for_cycle > 1){
					stall_for_cycle -=1;
					return;}
				else{
					stall_for_cycle = KNOB(KNOB_DCACHE_LATENCY)->getValue();
					 if(main_memory->m_mshr.size() >= KNOB(KNOB_MSHR_SIZE)->getValue())
						return;

					mshr_full_tlb = false;
					main_memory->insert_mshr(tlb_op);
					stall_for_memory = true;
				}}
		}
				
	
	else if (tlb_received && !dcache_lat_stall && !stall_for_memory)
		{
		visited_tlb = false;

		vpn = (vaddress/KNOB(KNOB_VMEM_PAGE_SIZE)->getValue());
//		cout << "\nVPN is " << vpn;
//		cout <<"came back from memory \n";
		//do we need to stall here again to access pte from dcache and then insert into tlb??
		uint64_t pfn_2 = vmem_vpn_to_pfn(vpn,MEM_latch->op->thread_id);
//		cout << "the pfn being installed \n" << pfn_2;
		tlb_install(dtlb,vpn,MEM_latch->op->thread_id,pfn_2);
//		cout << "\n inserted PFN is " << pfn_2;
		tlb_access(dtlb,vpn,MEM_latch->op->thread_id,&pfn);

		//cout << "\naccessed pfn is " << pfn;
		uint64_t temp_pfn = pfn;
		//check dcache_for_actual value (the physical address generated from pfn)
		actual_physical_address = (uint64_t)((temp_pfn<<(uint64_t)log2(KNOB(KNOB_VMEM_PAGE_SIZE)->getValue())) + vaddress%KNOB(KNOB_VMEM_PAGE_SIZE)->getValue());
//		actual_physical_address = (uint64_t)((temp_pfn*KNOB(KNOB_VMEM_PAGE_SIZE)->getValue())+(vaddress%KNOB(KNOB_VMEM_PAGE_SIZE)->getValue()));
		tlb_received = false;
		pte = false;
		pfn = 0;
		//MEM_latch->op_valid = true;
//		cout << "\n actual address is " << actual_physical_address;
		}
	}


	

     if(MEM_latch->op->mem_type==1 && !stall_for_memory)
        {

	if(KNOB(KNOB_ENABLE_VMEM)->getValue())
	MEM_latch->op->ld_vaddr = actual_physical_address;
/*	if(mshr_entries >= KNOB(KNOB_MSHR_SIZE)->getValue())
		{
		dcache_lat_stall = true;
		MEM_latch->op_valid = false;
		return;
		}*/
//	cout << "\n addres is" << MEM_latch->op->ld_vaddr;
        //cout << "\n Value of MEM type is" << MEM_latch->op->mem_type;
	if(!mshr_visited) {
	  
 
       // now use the generated physical_address everhwhere.
//	cout << "LOAD Executing dcache access memory with physical address\n";

	//access the TLB..if hit then dcache stall with the got PTE //update TLB hit
	//else stall to acess PTE .. if PTE miss , put PTE in MSHR to be updated in TLB ..update TLB miss..to put into TLB generate a new OP which will not be broadcasted since it is not a real instructiuon..stall everything till TLB MSHR entry comes back with PTE..thus no piggybacking since we are stalling anyways.but this instrucion can piggyback on other instrutions.

        //then stall again to access the data using the PTE (dcache). put the value in TLB.
//if(not_waiting_for_address_transalation){}

	dcache_lat_stall = true;
        if(stall_for_cycle == 1){
                hit_miss_load = dcache_access(MEM_latch->op->ld_vaddr);
                MEM_latch->op_valid = EX_latch->op_valid;
                stall_for_cycle=KNOB(KNOB_DCACHE_LATENCY)->getValue();
		dcache_lat_stall = false;}
        else{
		stall_for_cycle -= 1;
		MEM_latch->op_valid = false;
                return;}
	if(hit_miss_load == 1)
	{
		dcache_hit_count += 1;
		dcache_hit_count_thread[MEM_latch->op->thread_id] += 1;
//		dcache_insert(MEM_latch->op->ld_vaddr);
//		cout << "\nCACHE HIT!";
	}
	if(hit_miss_load == 0){
	//	cout << "\nCache miss in lab2 stage";
		dcache_miss_count += 1;
		dcache_miss_count_thread[MEM_latch->op->thread_id] += 1;
                if(main_memory->store_load_forwarding(MEM_latch->op,MEM_latch->op->thread_id)){
//			cout << "\nstore load forwarded";
			MEM_latch->op_valid = EX_latch->op_valid;
			MEM_latch->op = EX_latch->op;
			hit_miss_load = -1;
			return;}
		//	cout <<"\nValue of MEM type just before insertion "<<MEM_latch->op->mem_type;

                        if(!main_memory->check_piggyback(MEM_latch->op)){
                        if(main_memory->m_mshr.size() >= KNOB(KNOB_MSHR_SIZE)->getValue())
                                {
				mshr_full = true;
				mshr_visited = true;
				dcache_lat_stall = true;
				hit_miss_store = -1;
//				cout << "MSHR FULL \n";
                                return;}
			main_memory->insert_mshr(MEM_latch->op);
	//		cout << "\nInserted into MSHR with OP " << MEM_latch->op->ld_vaddr;
                        //MEM_latch->op = NULL;
                        MEM_latch->op_valid = false;
                        }
			else
			{
//			cout << "\nPiggybacking";
//			cout << "\nActual address is" << MEM_latch->op->ld_vaddr;
			//MEM_latch->op = NULL;
			MEM_latch->op_valid = false;
//			cout << "In Load if condition, setting NULL\n";
			}}
		hit_miss_store = -1;
		}
	else
	{
	
	     if(stall_for_cycle > 1){
                //hit_miss_load = dcache_access(MEM_latch->op->ld_vaddr);
                //MEM_latch->op_valid = EX_latch->op_valid;
                //stall_for_cycle=KNOB(KNOB_DCACHE_LATENCY)->getValue();
		stall_for_cycle -= 1;
                dcache_lat_stall = true;
		return;}
		else
		{
	     	if(mshr_visited && main_memory->m_mshr.size() >= KNOB(KNOB_MSHR_SIZE)->getValue()){
		dcache_lat_stall = true;
		stall_for_cycle = KNOB(KNOB_DCACHE_LATENCY)->getValue();
                return;
		}
	     else{
		/*int hit_miss = dcache_access(MEM_latch->op->ld_vaddr);
		if(hit_miss)
		{
		dcache_miss_count -= 1;
		dcache_hit_count += 1;
		MEM_latch->op_valid = EX_latch->op_valid;
		dcache_lat_stall = false;
		mshr_visited = false;
		hit_miss_store =-1;
		return;
		}*/
        	main_memory->insert_mshr(MEM_latch->op);
        	//MEM_latch->op = NULL;
        	MEM_latch->op_valid = false;
        	dcache_lat_stall = false;
		mshr_visited = false;
		hit_miss_store = -1;
		stall_for_cycle = KNOB(KNOB_DCACHE_LATENCY)->getValue();
       		 }
		}

	}
	}
   if(MEM_latch->op!=NULL && MEM_latch->op->mem_type==2 && !stall_for_memory)
        {
	if(KNOB(KNOB_ENABLE_VMEM)->getValue())
	MEM_latch->op->st_vaddr = actual_physical_address;
	    /*    if(mshr_entries >= KNOB(KNOB_MSHR_SIZE)->getValue())
                {
                dcache_lat_stall = true;
                MEM_latch->op_valid = false;
                return;
                }*/
  //      cout << "CAME IN STORE";
	if(!mshr_visited){
//	cout << "came in store condition of MEM_latch\n";
//	cout << "\naddress of this instruction is" << MEM_latch->op->st_vaddr;
        dcache_lat_stall = true;
        if(stall_for_cycle == 1){
                hit_miss_store = dcache_access(MEM_latch->op->st_vaddr);
	//	if(hit_miss_store)
	//		dcache_insert(MEM_latch->op->st_vaddr);
		MEM_latch->op_valid = EX_latch->op_valid;
                stall_for_cycle=KNOB(KNOB_DCACHE_LATENCY)->getValue();
		dcache_lat_stall = false;
	//	cout << "STALL" << stall_for_cycle;
		}
        else{
		MEM_latch->op_valid = false;
		stall_for_cycle -= 1;
                return;}
	if(hit_miss_store==1){
		dcache_hit_count += 1;
		dcache_hit_count_thread[MEM_latch->op->thread_id] += 1;
		MEM_latch->op_valid = EX_latch->op_valid;
		//MEM_latch->op = EX_latch->op;
		hit_miss_store = -1;
		return;
//		cout <<"\nCACHE HIT IN STORE!";
		}
	if(hit_miss_store == 0){
			dcache_miss_count += 1;
			dcache_miss_count_thread[MEM_latch->op->thread_id] += 1;
		        if(main_memory->store_load_forwarding(MEM_latch->op,MEM_latch->op->thread_id)){
                        MEM_latch->op_valid = EX_latch->op_valid;
                        MEM_latch->op = EX_latch->op;
                        hit_miss_load = -1;
                        return;}
                  if(!main_memory->check_piggyback(MEM_latch->op)){
			  if(main_memory->m_mshr.size() >= KNOB(KNOB_MSHR_SIZE)->getValue())
                                {
                                mshr_full = true;
                                mshr_visited = true;
                                dcache_lat_stall = true;
				hit_miss_store = -1;
//				cout << "MSHR FULL \n";
                                return;}
                        main_memory->insert_mshr(MEM_latch->op);
//			cout << "\ninsert into MSHR in store" ;
			MEM_latch->op_valid = false;  }
			MEM_latch->op_valid = false;
			hit_miss_store = -1;
				}
	}
	else
	{
	        if(stall_for_cycle > 1){
                //hit_miss_load = dcache_access(MEM_latch->op->ld_vaddr);
                //MEM_latch->op_valid = EX_latch->op_valid;
                //stall_for_cycle=KNOB(KNOB_DCACHE_LATENCY)->getValue();
                stall_for_cycle -= 1;
                dcache_lat_stall = true;
                return;}
		else
		{
	     if(mshr_visited && main_memory->m_mshr.size() >= KNOB(KNOB_MSHR_SIZE)->getValue())
		{dcache_lat_stall = true;
		stall_for_cycle = KNOB(KNOB_DCACHE_LATENCY)->getValue();
                return;}
             else{
		/*int hit_miss = dcache_access(MEM_latch->op->st_vaddr);
                if(hit_miss)
                {
		dcache_miss_count -= 1;
		dcache_hit_count += 1;
		dcache_insert(MEM_latch->op->st_vaddr);
                MEM_latch->op_valid = EX_latch->op_valid;
                dcache_lat_stall = false;
                mshr_visited = false;
                hit_miss_store =-1;
                return;
                }*/
                main_memory->insert_mshr(MEM_latch->op);
                dcache_lat_stall = false;
		mshr_visited = false;
		hit_miss_store = -1;
		MEM_latch->op_valid = false;
		stall_for_cycle = KNOB(KNOB_DCACHE_LATENCY)->getValue();
                 }

	}}

}	
}

//cout <<"\nMSHR size" << main_memory->m_mshr.size();
//cout <<"\nValue of cache stall" << dcache_lat_stall ;
//cout <<"\nValue of daa stall" << data_stall;
//cout <<"\nValue of control stall" << control_stall;
//cout <<"\nValue of FE latch" << FE_latch->op_valid;
//cout <<"Value of FE latch op"<< FE_latch->op;
//cout <<"\nValue of ID latch" <<ID_latch->op_valid;
//cout<<"\nValue of Ex latch" << EX_latch->op_valid;
//cout<<"\nvalue of mem latches " << MEM_latch->op_valid; 	//mshr_visited = false;
}


void EX_stage() {
	if(dcache_lat_stall || stall_for_tlb || stall_for_memory)
		return;
//   cout << "coming here in EX STAGE\n";
  //cout <<"\nValue of EX latch and valid " << EX_latch->op << EX_latch->op_valid;

  if( ID_latch->op_valid ) {

   //cout << "\nID is valid";
   //cout << "\nValue of countdown " << EX_latency_countdown;
  //  if(KNOB(KNOB_USE_BPRED)->getValue())     //update should probably be shifted to FE stage
   // bpred_update(branchpred,ID_latch->op->instruction_addr,taken,ID_latch->op->actually_taken);

  //  if(control_stall_mispred)
//	control_stall_mispred = false;

  // cout << "execution123\n";

    if(EX_latency_countdown >0 )
    	EX_latency_countdown--;    
    else //EX_latency_countdown==0
      EX_latency_countdown = get_op_latency(ID_latch->op)-1;

    if( EX_latency_countdown==0 ) {
      EX_latch->op = ID_latch->op;
      EX_latch->op_valid = ID_latch->op_valid;
  //    cout << "\nOP TYPE " << EX_latch->op->opcode << "with thread id " << EX_latch->op->thread_id;
  //    cout << "\nIn execution stage , the control stall is " << control_stall[ID_latch->op->thread_id];
    }
    else{
      EX_latch->op = NULL;
      EX_latch->op_valid = false;   	
    }
  }
  else {
    EX_latch->op = ID_latch->op;
    EX_latch->op_valid = ID_latch->op_valid;
  }

}

void ID_stage() {
	if(dcache_lat_stall || stall_for_tlb || stall_for_memory)
		return;
 // cout << "Executing ID latch \n";
//	cout << "\nop_valid od ID latch is" << ID_latch->op_valid;
  int fetch_id = 0;
	int tt;
   int increase_scheduler = 0;
//	cout << (KNOB(KNOB_RUN_THREAD_NUM)->getValue()); 
  for (tt = 0; tt < (KNOB(KNOB_RUN_THREAD_NUM)->getValue()); tt++) {
    fetch_id = (fetch_arbiter_IDStage++%(KNOB(KNOB_RUN_THREAD_NUM)->getValue()));
//	cout << "\nThread is in ID stage " << fetch_id;
//	cout << "\ndata stall is " << data_stall[fetch_id];
/*	if( fetch_list[fetch_id]->op!=NULL && !increase_scheduler ) {
	total_scheduler_accesses += 1;
	increase_scheduler = 1;}*/
	
	if(data_stall[fetch_id] || fetch_list[fetch_id]->op==NULL)
		continue;
	else
		break;}
//	cout << "\nvalue of tt " << tt;

	if(tt >= KNOB(KNOB_RUN_THREAD_NUM)->getValue() && EX_latency_countdown!=0){
		ID_latch->op_valid = ID_latch->op_valid;
		ID_latch->op = ID_latch->op;
		return;}

	else if(tt >= KNOB(KNOB_RUN_THREAD_NUM)->getValue() && EX_latency_countdown==0){
                ID_latch->op_valid = false;
                ID_latch->op = NULL;
                return;}

  if( fetch_list[fetch_id]->op_valid ) {

	total_scheduler_accesses += 1;
//cout << "\nThread id in ID " << fetch_id;
    /* Checking for any source data hazard */
    if( EX_latency_countdown==0) {

      /* Checking for any source data hazard */  
  // return FALSE if the end of trace
      for (int ii = 0; ii < fetch_list[fetch_id]->op->num_src; ii++) {
        if( register_file[fetch_id][ fetch_list[fetch_id]->op->src[ii] ].valid == false ) {
          data_hazard_count++;  
          data_stall[fetch_id] = true;
//	  cout << "\nHitting data stalls for thread id" << fetch_id ;
	  break;
	}
	/*else{
	data_stall[fetch_id] = false;
	found = 1;
	}*/
	}
//	if(data_stall[fetch_id])
//		continue;
      if( fetch_list[fetch_id]->op->opcode == OP_CF ) {
        control_hazard_count++;
	control_hazard_count_thread[fetch_id]++;
//	if(KNOB(KNOB_USE_BPRED)->getValue() == 0)
 //       	control_stall = true;
      }      
    }

    /* If no data hazard, then contiune */
    if(EX_latency_countdown!=0) {				//execution stall
      ID_latch->op = ID_latch->op;  
      ID_latch->op_valid = ID_latch->op_valid; 
      fetch_list[fetch_id]->op = fetch_list[fetch_id]->op;
      fetch_list[fetch_id]->op_valid = fetch_list[fetch_id]->op_valid ;
    //  if(!data_stall[fetch_id])
      //	break;
    }
    else if(!data_stall[fetch_id] ) {      //no data stall
      ID_latch->op = fetch_list[fetch_id]->op;  
      ID_latch->op_valid = fetch_list[fetch_id]->op_valid;
      if( fetch_list[fetch_id]->op->dst != -1 ) {
        register_file[fetch_id][ fetch_list[fetch_id]->op->dst ].valid = false; 
        register_file[fetch_id][ fetch_list[fetch_id]->op->dst ].count++;
//	break;
      }
	found = 0;
	fetch_list[fetch_id]->used = true;
	fetch_list[fetch_id]->op = NULL;
	fetch_list[fetch_id]->op_valid = false;
//	cout << "OP PASSED\n";
    }
    else {
//	cout << "\n ID Becoming null";
      ID_latch->op = NULL;  
      ID_latch->op_valid = false; 
      fetch_list[fetch_id]->op = fetch_list[fetch_id]->op;
      fetch_list[fetch_id]->op_valid = fetch_list[fetch_id]->op_valid ;

//	continue;
    }

/*	if(control_stall[fetch_id])
        {
        fetch_list[fetch_id]->used = true;
        fetch_list[fetch_id]->op = NULL;
        fetch_list[fetch_id]->op_valid = false;
        }*/
  //return;
  }
  else {
    if(EX_latency_countdown==0) {
      ID_latch->op = fetch_list[fetch_id]->op;  
      ID_latch->op_valid = fetch_list[fetch_id]->op_valid;
      fetch_list[fetch_id]->op = NULL;
      fetch_list[fetch_id]->op_valid = false;
      fetch_list[fetch_id]->used = true;
 //	cout << "BAD OP PASSED \n";
//	cout << "\nNever";
//	if(!data_stall[fetch_id])
    }

/*	if(control_stall[fetch_id])
        {
        fetch_list[fetch_id]->used = true;
        fetch_list[fetch_id]->op = NULL;
        fetch_list[fetch_id]->op_valid = false;
        }*/
//return;
  }
/*if(control_stall[fetch_id])
	{
	fetch_list[fetch_id]->used = true;
	fetch_list[fetch_id]->op = NULL;
	fetch_list[fetch_id]->op_valid = false;
	}*/
//if(!data_stall[fetch_id])
//	break; 
}



void FE_stage() {
//	if(dcache_lat_stall || stall_for_tlb || stall_for_memory)  //lab4
//		return;
int fetch_arbiter = -1;

//cout << " Executing FE stage \n";
//cout <<"\ndata stall " << data_stall << "\ncontrol stall" << control_stall ;  
 for( int tt=0 ; tt < KNOB(KNOB_RUN_THREAD_NUM)->getValue() ; tt ++) {
 int ii = (fetch_arbiter_FEStage++%(KNOB(KNOB_RUN_THREAD_NUM)->getValue()));

 //cout << "\nValue of thread is and fetch_arbiter" << ii <<" " << fetch_arbiter_FEStage ;

 /*if( (EX_latency_countdown!=0) || (data_stall[ii]) ) {  //Execution stall
      fetch_list[ii]->op = fetch_list[ii]->op;
      fetch_list[ii]->op_valid = fetch_list[ii]->op_valid;
//	ii = -1;
	cout << "coming in data stall \n";
      continue; 
  }
  else if(control_stall[ii] || fetch_list[ii]->op!=NULL) {//|| control_stall_mispred[ii] ) {	//Control stall
     // fetch_list[ii]->op = NULL;//fetch_list[ii]->op;
     // fetch_list[ii]->op_valid = false;//fetch_list[ii]->op_valid;   
//	fetch_list[ii]->used = false;
//	cout << "coming in control stall \n";
//	ii = -1; 
      continue;
  }*/
 //cout << "\nvalue of control stall "<< control_stall[0];
  if(fetch_list[ii]->op!=NULL || control_stall[ii] )//|| data_stall[ii])// || dcache_lat_stall || stall_for_tlb || stall_for_memory)
	continue;

  else{
    Op *op = get_free_op();
    op->thread_id = ii;
  //  cout << "comingto fetch but not able to \n";

    if( get_op(op) && op->valid && op->opcode && fetch_list[op->thread_id]){  
//	cout << "\nopcode is" << (int)op->opcode << "valid bit"<< op->valid;
//	cout << "GEtting new OP without propagating\n";
//	cout << "STALL value \n" << dcache_lat_stall;
//	cout << "NEW OP\n";
	ii = op->thread_id;
    //  cout << "filling in value\n";
      fetch_list[ii]->op = op;
      fetch_list[ii]->op_valid = true;
      fetch_list[ii]->used = false;

	if(op->is_fp) {
		total_floating_operations += 1;
		if(op->num_src > 0)
			reads_floating_operations += op->num_src;
		if(op->dst != -1 )
			writes_floating_operations += 1;
		}
	else {
/*		if(op->num_src > 0)
                        reads_integer_operations += op->num_src;
                if(op->dst != -1)
                        writes_integer_operations += 1;*/

 		if(op->opcode == OP_IADD || op->opcode == OP_IMUL || op->opcode == OP_IDIV || op->opcode == OP_ICMP)
		total_integer_operations += 1; 
		if(op->num_src > 0)
                        reads_integer_operations += op->num_src;
                if(op->dst != -1)
                        writes_integer_operations += 1;}

        if(op->opcode == OP_IMUL || op->opcode == OP_MM)
		total_multiply_instructions += 1;

	if(op->opcode == OP_CF)
		total_branch_instructions += 1;

	if(op->opcode == OP_LD)
		total_load_instructions += 1;
 	if(op->opcode == OP_ST)
		total_store_instructions += 1;

        //if(op->opcode == 

//	cout << "thread is \n" << fetch_list[ii]->op->thread_id;

      //FE_latch->op = op;
      //FE_latch->op_valid = true;

      if(KNOB(KNOB_USE_BPRED)->getValue()==1)
	{
	if(fetch_list[ii]->op->cf_type == CF_CBR)
	{
	//cout << "in prediction\n";
	//cout << "\nvalue of actuall taken" << FE_latch->op->actually_taken;
	taken = bpred_access(branchpred , fetch_list[ii]->op->instruction_addr,ii);

	//taken = bpred_access(branchpred , fetch_list[ii]->op->instruction_addr,ii,512, 0 , 8);
	//cout << "\nvalue of predicted taken" << taken;
		
	if(taken != fetch_list[ii]->op->actually_taken){
	//	bpred_mispred_count += 1;
		//control_stall_mispred = true;
		//cout << "incorrect predicted \n";
		fetch_list[ii]->op->mispredicted = true;
		//br_stall[ii] = true;  //added lab4
		control_stall[ii] = true;}
	else
		fetch_list[op->thread_id]->op->mispredicted = false;

//	bpred_update(branchpred,fetch_list[ii]->op->instruction_addr,taken,fetch_list[ii]->op->actually_taken,ii);

	bpred_update(branchpred , fetch_list[ii]->op->instruction_addr , taken , fetch_list[ii]->op->actually_taken , ii);

	bpred_mispred_count = branchpred->mispred ;
	bpred_mispred_count_thread[ii] = branchpred->mispred_thread[ii];
	bpred_okpred_count =  branchpred->okpred;
	bpred_okpred_count_thread[ii] = branchpred->okpred_thread[ii];
	}
	}

	 else if(KNOB(KNOB_USE_BPRED)->getValue() == 0 && fetch_list[op->thread_id]->op->opcode == OP_CF ){
                 control_stall[ii] = true;
//		cout << "turning on control stall\n";
		 //br_stall[op->thread_id] = true;
		}
//	cout <<"MEme type of instruction " < FE_latch->op->mem_type ;  
    } 
    else {
      if((fetch_list[0]->op_valid==false) && !fetch_list[1]->op_valid && !fetch_list[2]->op_valid && !fetch_list[3]->op_valid && (ID_latch->op_valid==false) && (EX_latch->op_valid==false) && (MEM_latch->op_valid==false) )
		if(mshr_entries<1 && retire_list.size() < 1) 
    	  		sim_end_condition = true;
    	//fetch_list[op->thread_id]->op = NULL;
//      cout << "\nSim end condition "<< sim_end_condition;
    //  fetch_list[op->thread_id]->op_valid = false;
//	fetch_list[op->thread_id]->op = NULL;
//	if(fetch_list[ii]!=NULL)
//	free_op(fetch_list[op->thread_id]->op); 
 	op->thread_id = -1;
      free_op(op);    
    }
  return;
  }

	}
}


void  init_latches()
{
  MEM_latch = new pipeline_latch();
  EX_latch = new pipeline_latch();
  ID_latch = new pipeline_latch();
  FE_latch = new pipeline_latch();

  for(int i=0;i<HW_MAX_THREAD;i++){
	pipeline_latch *temp = new pipeline_latch();
	temp->op= NULL;
	temp->op_valid = false;
	temp->used = false;
	fetch_list.push_back(temp);}
  //fetch_list = new pipeline_latch[4];
 
  MEM_latch->op = NULL;
  EX_latch->op = NULL;
  ID_latch->op = NULL;
  FE_latch->op = NULL;

  /* you must set valid value correctly  */ 
  MEM_latch->op_valid = false;
  EX_latch->op_valid = false;
  ID_latch->op_valid = false;
  FE_latch->op_valid = false;

  retire_list.clear();

}

void init_regfile() {

for(int tt = 0 ; tt < HW_MAX_THREAD ; tt++) {

  for (int ii = 0; ii < NUM_REG; ii++) {

   register_file[tt][ii].valid = true;

   register_file[tt][ii].count = 0;

  }

}
}

bool icache_access(ADDRINT addr) {   /** please change uint32_t to ADDRINT NEW-LAB2 */ 

  /* For Lab #1, you assume that all I-cache hit */     
  bool hit = FALSE; 
  if (KNOB(KNOB_PERFECT_ICACHE)->getValue()) hit = TRUE; 
  return hit; 
}


bool dcache_access(ADDRINT addr) { /** please change uint32_t to ADDRINT NEW-LAB2 */ 
  /* For Lab #1, you assume that all D-cache hit */     
  /* For Lab #2, you need to connect cache here */   // NEW-LAB2

  total_dcache_reads += 1;
  return cache_access(data_cache,addr); 
  bool hit = FALSE;
  if (KNOB(KNOB_PERFECT_DCACHE)->getValue()) hit = TRUE; 
  return hit; 
}



// NEW-LAB2 
void dcache_insert(ADDRINT addr)  // NEW-LAB2 
{                                 // NEW-LAB2 
  /* dcache insert function */   // NEW-LAB2 
  total_dcache_writes += 1;
  cache_insert(data_cache, addr) ;   // NEW-LAB2 
 
}                                       // NEW-LAB2 

void broadcast_rdy_op(Op* op)             // NEW-LAB2 
{                                          // NEW-LAB2 
  /* you must complete the function */     // NEW-LAB2 
  // mem ops are done.  move the op into WB stage   // NEW-LAB2
  // if(flag)
//	retire_list.clear();
  // flag=false;
//  cout <<"\nInstructions broadcasted " << op->mem_type << "address of load " << op->ld_vaddr << "address of store " << op->st_vaddr;
  //if(op->write_flag)
  retire_list.push_back(op); 
}      // NEW-LAB2 



/* utility functions that you might want to implement */     // NEW-LAB2 
int get_dram_row_id(ADDRINT addr)    // NEW-LAB2 
{  // NEW-LAB2 
 // NEW-LAB2 
/* utility functions that you might want to implement */     // NEW-LAB2 
/* if you want to use it, you should find the right math! */     // NEW-LAB2 
/* pleaes carefull with that DRAM_PAGE_SIZE UNIT !!! */     // NEW-LAB2 
  // addr >> 6;   // NEW-LAB2 
  return 2;   // NEW-LAB2 
}  // NEW-LAB2 

int get_dram_bank_id(ADDRINT addr)  // NEW-LAB2 
{  // NEW-LAB2 
 // NEW-LAB2 
/* utility functions that you might want to implement */     // NEW-LAB2 
/* if you want to use it, you should find the right math! */     // NEW-LAB2 
UINT64 KNOB_DRAM_PAGE_SIZE= KNOB (KNOB_DRAM_PAGE_SIZE)->getValue ();
UINT64 KNOB_DRAM_BANK_NUM = KNOB (KNOB_DRAM_BANK_NUM)->getValue ();

  // (addr >> 6);   // NEW-LAB2 
  return 1;   // NEW-LAB2 
}  // NEW-LAB2 


