#include <assert.h>
#include "bpred.h"
#include <iostream>
#include <math.h>

using namespace std;

//Needs to add knobs for history bits

#define BPRED_PHT_CTR_MAX  3
#define BPRED_PHT_CTR_INIT 2

#define BPRED_SAT_INC(X,MAX)  (X<MAX)? (X+1): MAX
#define BPRED_SAT_DEC(X)      X?       (X-1): 0

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

bpred * bpred_new(bpred_type type, int hist_len ,int percep_hist_len, int table_entries, int ctr_bits , int threshold){
  bpred *b = (bpred *) calloc(1, sizeof(bpred));
  
  b->type = type;
  b->percep_hist_len = percep_hist_len + 1;
  b->hist_len = hist_len;
  b->pht_entries = (1<<hist_len);
  b->table_entries = table_entries;
  b->ctr_bits = ctr_bits;
  b->threshold = threshold;
  b->y_out = 0;

  switch(type){

  case BPRED_NOTTAKEN:
    // nothing to do
    break;

  case BPRED_TAKEN:
    // nothing to do
    break;

  case BPRED_BIMODAL:
    bpred_bimodal_init(b); 
    break;

  case BPRED_GSHARE:
    bpred_gshare_init(b); 
    break;
  
  case BPRED_PERCEPTRON:
    bpred_perceptron_init(b);
    break;

  default: 
    assert(0);
    return NULL;
  }

return b;

}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

int bpred_access(bpred *b, unsigned int pc,unsigned int threadid){

  switch(b->type){

  case BPRED_NOTTAKEN:
    return 0;

  case BPRED_TAKEN:
    return 1;

  case BPRED_BIMODAL:
    return bpred_bimodal_access(b, pc);

  case BPRED_GSHARE:
    return bpred_gshare_access(b, pc,threadid);

  case BPRED_PERCEPTRON:
    return bpred_perceptron_access(b,pc,threadid);

  default: 
    assert(0);
  }

}


//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void bpred_update(bpred *b, unsigned int pc, 
		    int pred_dir, int resolve_dir,unsigned int threadid){

  // update the stats
  if(pred_dir == resolve_dir){
    b->okpred++;
    b->okpred_thread[threadid]++;
  }else{
    b->mispred++;
    b->mispred_thread[threadid]++;
  }

  // update the predictor

  switch(b->type){
    
  case BPRED_NOTTAKEN:
    // nothing to do
    break; 

  case BPRED_TAKEN:
    // nothing to do
    break;

  case BPRED_BIMODAL:
    bpred_bimodal_update(b, pc, pred_dir, resolve_dir);
    break;

  case BPRED_GSHARE:
    bpred_gshare_update(b, pc, pred_dir, resolve_dir,threadid);
    break;

  case BPRED_PERCEPTRON:
    bpred_perceptron_update(b, pc, pred_dir, resolve_dir, threadid);
    break;

  default: 
    assert(0);
  }


}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void  bpred_bimodal_init(bpred *b){

  b->pht = (unsigned int *) calloc(b->pht_entries, sizeof(unsigned int));

  for(int ii=0; ii< b->pht_entries; ii++){
    b->pht[ii]=BPRED_PHT_CTR_INIT; 
  }
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

int   bpred_bimodal_access(bpred *b, unsigned int pc){
  int pht_index = pc % (b->pht_entries);
  int pht_ctr = b->pht[pht_index];

  if(pht_ctr > BPRED_PHT_CTR_MAX/2){
    return 1; // Taken
  }else{
    return 0; // Not-Taken
  }
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void  bpred_bimodal_update(bpred *b, unsigned int pc, 
			    int pred_dir, int resolve_dir){
  int pht_index = pc % (b->pht_entries);
  int pht_ctr = b->pht[pht_index];
  int new_pht_ctr;

  if(resolve_dir==1){
    new_pht_ctr = BPRED_SAT_INC(pht_ctr, BPRED_PHT_CTR_MAX);
  }else{
    new_pht_ctr = BPRED_SAT_DEC(pht_ctr);
  }

  b->pht[pht_index] = new_pht_ctr;  
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void  bpred_gshare_init(bpred *b){
  b->pht = (unsigned int *) calloc(b->pht_entries, sizeof(unsigned int));

  b->ghr[0] = 0;
  b->ghr[1] = 0;
  b->ghr[2] = 0;
  b->ghr[3] = 0;

  for(int ii=0; ii< b->pht_entries; ii++){
    b->pht[ii]=BPRED_PHT_CTR_INIT;
 //   cout << "\n Value of initialized pht is " << b->pht[ii];
  }

}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

int bpred_gshare_access(bpred *b, unsigned int pc,unsigned int threadid){

 // unsigned int mask=0;
 // int temp = b->hist_len;
 // while(temp>0)
//	{
//	mask = mask | 1;
//	mask <<= mask;
//	temp = temp - 1;
//	}
  int pht_index = (pc^b->ghr[threadid])%b->pht_entries;
 //  pht_index = pht_index & mask;
  int taken,correctly_predicted=0;
  if (b->pht[pht_index] >= 2)
	taken = 1;
  else
	taken = 0;

  //int pht_ctr = b->pht[pht_index];

  /*if(actual_dir == taken)
	correctly_predicted = 1;
  else
	correctly_predicted = 0;*/

  return taken;
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void  bpred_gshare_update(bpred *b, unsigned int pc, 
			    int pred_dir, int resolve_dir,unsigned int threadid){


 //   unsigned int mask=0;
 // int temp = b->hist_len;
 // while(temp>0)
  //      {
  //      mask = mask | 1;
  //      mask <<= mask;
   //     temp = temp - 1;
   //     }

  int pht_index = (pc^b->ghr[threadid])% b->pht_entries;
 // pht_index = pht_index & mask;
 
//  cout << "\npht_index is" << pht_index;
  int pht_ctr = b->pht[pht_index];
//  cout << "\npht_ctr is" << pht_ctr;
  /*if(pred_dir == resolve_dir)
	correct++;
  else
	correct--;*/
// cout << "\n resolve dir value" << resolve_dir;

  if(resolve_dir){
	pht_ctr = BPRED_SAT_INC(pht_ctr,BPRED_PHT_CTR_MAX);
	b->pht[pht_index] = pht_ctr;}
  else
	{
	pht_ctr = BPRED_SAT_DEC(pht_ctr);
	b->pht[pht_index] = pht_ctr;
	}
//cout << "\nvalue of pht counter" << b->pht[pht_index];

   b->ghr[threadid] = ((b->ghr[threadid]<<1) | resolve_dir)%b->pht_entries;
//cout << "\nValue of ghr after update" << b->ghr;

}


/* array implementation of perceptron */

void bpred_perceptron_init(bpred *b) {

  b->perceptron_array = (int **) calloc(b->table_entries, sizeof(int*));
  for ( int i = 0 ; i < b->table_entries ; i++ )
	b->perceptron_array[i] = (int *) calloc(b->percep_hist_len , sizeof(int));

  b->history_vector = (int *) calloc(b->percep_hist_len , sizeof(int));

  b->history_vector[0] = 1;


  for(int ii=0; ii< b->table_entries ; ii++) {
	for(int jj=0 ; jj < b->percep_hist_len ; jj++)
    		b->perceptron_array[ii][jj] = 0; //was 1 earlier
  }

}

int bpred_perceptron_access(bpred *b, unsigned int pc,unsigned int threadid) {
  int taken = 0;

  int index = (pc) % b->table_entries;  //change index calculation

  for( int ii=1 ; ii < b->percep_hist_len ; ii++ ) {
  	taken = taken + (b->history_vector[ii] * b->perceptron_array[index][ii]);
  }
  taken = taken + b->perceptron_array[index][0] * 1;  //since x0 = 1 always
  b->y_out = taken;


  if(taken >=0)
	return 1;
  else
	return 0;

 return 0;
}


void bpred_perceptron_update(bpred *b, unsigned int pc,
                            int pred_dir, int resolve_dir,unsigned int threadid) { 

  int temp , ii;

  int resolve_temp=0 , y_out = 0;

  if(b->y_out < 0)
	y_out = -1;
  else
        y_out = 1;
 
  if(resolve_dir == 0)
	resolve_temp = -1;
  else
	resolve_temp = 1;

  int index = (pc) % b->table_entries;

  if(y_out!=resolve_temp || abs(b->y_out) <= b->threshold)
  for ( int ii=0; ii < b->percep_hist_len ; ii++ ) {
	b->perceptron_array[index][ii] = b->perceptron_array[index][ii] + resolve_temp * b->history_vector[ii];

	if (b->perceptron_array[index][ii] < 0) {
		temp = (-1)*b->perceptron_array[index][ii];
		temp = temp % (1<<(b->ctr_bits-1));
		b->perceptron_array[index][ii] = (-1)*temp;
		}
	else
		b->perceptron_array[index][ii] =  b->perceptron_array[index][ii] % (1<<(b->ctr_bits-1));
	}

  for (int ii=1; ii < b->percep_hist_len - 1 ; ii++ )
                b->history_vector[ii] = b->history_vector[ii+1];
  b->history_vector[b->percep_hist_len-1] = resolve_temp;


/*  for ( ii=b->percep_hist_len-1; ii >= 2 ; ii-- )
		b->history_vector[ii] = b->history_vector[ii-1];
  b->history_vector[1] = resolve_temp;
*/

}

/*bit wise implementation */
/*
void  bpred_perceptron_init(bpred *b){
  b->perceptron_array = (unsigned int *) calloc(b->perceptron_tbentry, sizeof(unsigned int));

  b->ghr[0] = 0;
  b->ghr[1] = 0;
  b->ghr[2] = 0;
  b->ghr[3] = 0;

  for(int jj=0 ; jj < b->perceptron_tbentry; jj++)
    b->perceptron_array[jj] = 0;
 //   cout << "\n Value of initialized pht is " << b->pht[ii];
  }

}*/


//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

