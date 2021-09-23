#ifndef WORKER_MATMUL_H
#define WORKER_MATMUL_H

#include "MWWorker.h"
#include "Task-Matmul.h"

class Worker_Matmul : public MWWorker 
{

 public:
    Worker_Matmul();
    ~Worker_Matmul();

    MWReturn unpack_init_data( void );
    
    void execute_task( MWTask * );

 private:
 	int num_rowsA;
 	int num_rowsB;
 	int num_colsB;
 	int **A;
	int **B;
};

#endif
