#ifndef TASK_NQUEENS_H
#define TASK_NQUEENS_H

#include <stdio.h>
#include "MWTask.h"

class Task_nQueens : public MWTask {

 public:
    
    Task_nQueens();

    Task_nQueens( int N, int num );

    ~Task_nQueens();

    void pack_work( void );
    
    void unpack_work( void );
    
    void pack_results( void );
    
    void unpack_results( void );

    void printself( int level = 60 );

    void write_ckpt_info( FILE *fp );
    void read_ckpt_info( FILE *fp );

    int N;
    int num;
    int *results;
};

#endif
