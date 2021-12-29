#ifndef TASK_MATMUL_H
#define TASK_MATMUL_H

#include <stdio.h>
#include "MWTask.h"

class Task_Matmul : public MWTask {

 public:
    
    Task_Matmul();

    Task_Matmul( int startRow, int endRow, int nCols );

    ~Task_Matmul();

    void pack_work( void );
    
    void unpack_work( void );
    
    void pack_results( void );
    
    void unpack_results( void );

    void printself( int level = 60 );

    void write_ckpt_info( FILE *fp );
    void read_ckpt_info( FILE *fp );

    int startRow;
    int endRow;
    int nCols;
    int **results;
};

#endif
