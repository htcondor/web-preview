#ifndef TASK_MULT_H
#define TASK_MULT_H

#include <stdio.h>
#include "MWTask.h"

class Task_Mult : public MWTask {

 public:
    
    /** Construct with parameters.
        @param first The first number in the pair of starting "seed" numbers
        @param second The second of the "seed" numbers
        @param seqlen The length of the sequence to make 
    */
    Task_Fib( int len, int br, int **Aarray, int *Barray );

    /// Default Destructor
    ~Task_Fib();

    void pack_work( void );
    
    void unpack_work( void );
    
    /// Packing and Unpacking routines
    void pack_results( void );
    void unpack_results( void );

    void write_ckpt_info( FILE *fp );
    void read_ckpt_info( FILE *fp );

        /// the length of the array
    int length;
    	/// the breadth of the array
    int breadth;
        /// the first array.
    int **array1;
        /// the second array.
    int **array2;
        /// The results
    int *results;
};

#endif

