#ifndef WORKER_NQUEENS_H
#define WORKER_NQUEENS_H

#include "MWWorker.h"
#include "Task-nQueens.h"

class Worker_nQueens : public MWWorker 
{

 public:
    Worker_nQueens();
    ~Worker_nQueens();

    MWReturn unpack_init_data( void );
    
    void execute_task( MWTask * );

    int* search_subspace ( int *perm, int N, int givenPerm );

    int* isCorrectPermutation ( int *perm, int N );


 private:
 	int N;
	int partition_factor;
};

#endif
