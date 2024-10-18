#ifndef SIM_CACHE_H
#define SIM_CACHE_H

typedef 
struct {
   uint32_t BLOCKSIZE;
   uint32_t L1_SIZE;
   uint32_t L1_ASSOC;
   uint32_t L2_SIZE;
   uint32_t L2_ASSOC;
   uint32_t PREF_N;
   uint32_t PREF_M;
} cache_params_t;

typedef 
struct {
   uint32_t VALID_BIT;
   uint32_t DIRTY_BIT;
   uint32_t TAG;
   uint32_t ADDR;
} cache_set_t;

typedef struct {
    uint32_t* TAG;      // Pointer to a 2D array of uint32_t
    int LRU_COUNTER;     // Pointer to a 1D array of integers
    uint32_t VALID;       // Validity flag
} stream_buf_t;




// Put additional data structures here as per your requirement.

#endif
