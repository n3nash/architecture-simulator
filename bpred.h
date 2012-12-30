//Author: Prashant J Nair, Georgia Tech
//Simulation of Branch Predictors

/****** Header file for branch prediction structures ********/

#ifndef __BPRED_H
#define __BPRED_H
#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>

typedef enum bpred_type_enum{
  BPRED_NOTTAKEN, 
  BPRED_TAKEN,
  BPRED_BIMODAL,
  BPRED_GSHARE,
  BPRED_PERCEPTRON 
}bpred_type;

typedef struct bpred bpred;

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

struct bpred{
  unsigned int  ghr[4]; // global history register
  unsigned int *pht; // pattern history table

  int **perceptron_array;  //array of perceptrons
  int *history_vector;  //history buffer for perceptron
  signed int ctr_bits; 		    //so that the weights in the perceptrons are within %ctr_bits.
  unsigned int table_entries;
  int threshold; 
  int percep_hist_len;

  int y_out;

  bpred_type type; // type of branch predictor

  unsigned int hist_len; // history length or the history buffer for perceptron length
  unsigned int pht_entries; // entries in pht table or in the perceptron array


  long int mispred; // A counter to count mispredictions
  long int okpred; // A counter to count correct predictions
  long int mispred_thread[4];
  long int okpred_thread[4];
};


/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

bpred *bpred_new(bpred_type type, int , int , int, int, int);
int    bpred_access(bpred *b, unsigned int pc,unsigned int threadid);
void   bpred_update(bpred *b, unsigned int pc, 
		    int pred_dir, int resolve_dir,unsigned int threadid);

// bimodal predictor
void   bpred_bimodal_init(bpred *b);
int    bpred_bimodal_access(bpred *b, unsigned int pc);
void   bpred_bimodal_update(bpred *b, unsigned int pc, 
			    int pred_dir, int resolve_dir);


// gshare predictor
void   bpred_gshare_init(bpred *b);
int    bpred_gshare_access(bpred *b, unsigned int pc,unsigned int threadid);
void   bpred_gshare_update(bpred *b, unsigned int pc, 
			   int pred_dir, int resolve_dir,unsigned int threadid);

//perceptron predictor
void   bpred_perceptron_init(bpred *b);
int    bpred_perceptron_access(bpred *b, unsigned int pc,unsigned int threadid);
void   bpred_perceptron_update(bpred *b, unsigned int pc,
                           int pred_dir, int resolve_dir,unsigned int threadid);


/***********************************************************/
#endif
