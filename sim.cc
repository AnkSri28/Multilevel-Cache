#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <cmath>
#include <vector>
#include "sim.h"
#include "cache.cpp"

/*  "argc" holds the number of command-line arguments.
    "argv[]" holds the arguments themselves.

    Example:
    ./sim 32 8192 4 262144 8 3 10 gcc_trace.txt
    argc = 9
    argv[0] = "./sim"
    argv[1] = "32"&& lru_counter[i][k] > keyLru) {
                tags[k + 1] = tags[k]; // Move TAG
                k--;
            }
            tags[k + 1] = key; 
    argv[2] = "8192"
    ... and so on
*/
// Function to sort TAGs based on LRU counters and return sorted TAGs
cache_set_t** sort_cache_lru(int** lru_counter, cache_set_t** lx_cache, uint32_t sets, uint32_t ways) {
    // Allocate memory for sorted cache sets
    cache_set_t** sorted_cache_sets = new cache_set_t*[sets];
    for (uint32_t i = 0; i < sets; i++) {
        sorted_cache_sets[i] = new cache_set_t[ways]; // Allocate space for each set
    }

    for (uint32_t i = 0; i < sets; i++) {
        // Create a temporary array to hold cache sets for sorting
        cache_set_t* temp_cache_sets = new cache_set_t[ways];
        for (uint32_t j = 0; j < ways; j++) {
            temp_cache_sets[j] = lx_cache[i][j]; // Copy cache sets
        }

        // Simple selection sort to sort cache sets based on LRU counters
        for (uint32_t j = 0; j < ways - 1; j++) {
            for (uint32_t k = j + 1; k < ways; k++) {
                // Swap if the current LRU counter is greater than the next
                if (lru_counter[i][j] > lru_counter[i][k]) {
                    // Swap cache sets
                    std::swap(temp_cache_sets[j], temp_cache_sets[k]);
                    // Swap LRU counters as well to maintain association
                    std::swap(lru_counter[i][j], lru_counter[i][k]);
                }
            }
        }

        // Store sorted cache sets back into the sorted_cache_sets array
        for (uint32_t j = 0; j < ways; j++) {
            sorted_cache_sets[i][j] = temp_cache_sets[j];
        }

        delete[] temp_cache_sets; // Free temporary cache sets array
    }

    return sorted_cache_sets; // Return the sorted cache sets
}


int main (int argc, char *argv[]) {
   FILE *fp;			// File pointer.
   char *trace_file;		// This variable holds the trace file name.
   cache_params_t params;	// Look at the sim.h header file for the definition of struct cache_params_t.
   char rw;			// This variable holds the request's type (read or write) obtained from the trace.
   uint32_t addr;		// This variable holds the request's address obtained from the trace.
				// The header file <inttypes.h> above defines signed and unsigned integers of various sizes in a machine-agnostic way.  "uint32_t" is an unsigned integer of 32 bits.
   ///////////////////////////////////////////////////////////////////////////////////////////////////
   uint32_t l1_set;
   uint32_t l1_index;
   uint32_t l1_tag;
   uint32_t l1_way;
   uint32_t block_offset;
   uint32_t l2_set;
   uint32_t l2_index;
   uint32_t l2_tag;
   uint32_t l2_way;  
   bool status; 
   char dirty;
   uint32_t perf_n;
   uint32_t perf_m;
   ///////////////////////////////////////////////////////////////////////////////////////////////////
   uint32_t assoc[l1_way];
   uint32_t** sortedLRUs;
   ///////////////////////////////////////////////////////////////////////////////////////////////////
   // Exit with an error if the number of command-line arguments is incorrect.
   if (argc != 9) {
      printf("Error: Expected 8 command-line arguments but was provided %d.\n", (argc - 1));
      exit(EXIT_FAILURE);
   }
    
   // "atoi()" (included by <stdlib.h>) converts a string (char *) to an integer (int).
   params.BLOCKSIZE = (uint32_t) atoi(argv[1]);
   params.L1_SIZE   = (uint32_t) atoi(argv[2]);
   params.L1_ASSOC  = (uint32_t) atoi(argv[3]);
   params.L2_SIZE   = (uint32_t) atoi(argv[4]);
   params.L2_ASSOC  = (uint32_t) atoi(argv[5]);
   params.PREF_N    = (uint32_t) atoi(argv[6]);
   params.PREF_M    = (uint32_t) atoi(argv[7]);
   trace_file       = argv[8];

   // Open the trace file for reading.
   fp = fopen(trace_file, "r");
   if (fp == (FILE *) NULL) {
      // Exit with an error if file open failed.
      printf("Error: Unable to open file %s\n", trace_file);
      exit(EXIT_FAILURE);
   }
    
   // Print simulator configuration.
   printf("===== Simulator configuration =====\n");
   printf("BLOCKSIZE:  %u\n", params.BLOCKSIZE);
   printf("L1_SIZE:    %u\n", params.L1_SIZE);
   printf("L1_ASSOC:   %u\n", params.L1_ASSOC);
   printf("L2_SIZE:    %u\n", params.L2_SIZE);
   printf("L2_ASSOC:   %u\n", params.L2_ASSOC);
   printf("PREF_N:     %u\n", params.PREF_N);
   printf("PREF_M:     %u\n", params.PREF_M);
   printf("trace_file: %s\n", trace_file);
   printf("\n");

   /////////////////////////////////////////////////////////////////////////////////////////
   perf_n = params.PREF_N;
   perf_m = params.PREF_M;
   /////////////////////////////////////////////////////////////////////////////////////////
   //CACHE_PARAMETERS
   /////////////////////////////////////////////////////////////////////////////////////////
   l1_set = params.L1_SIZE/(params.L1_ASSOC * params.BLOCKSIZE);
   l1_index = static_cast<uint32_t>(std::log2(l1_set));
   block_offset = static_cast<uint32_t>(std::log2(params.BLOCKSIZE));
   l1_tag = 32 - l1_index - block_offset;
   l1_way = params.L1_ASSOC;
   //printf("l1_set=%u\n",l1_set);
   //printf("l1_index=%u\n",l1_index);
   //printf("l1_tag=%u\n",l1_tag);
   //printf("l1_way=%u\n",l1_way);

   //printf("block_offset=%u\n",block_offset);
   ////////////////////////////////////////////////////////////////////////////////////////////
   if (params.L2_SIZE > 0)
   {
      l2_set = params.L2_SIZE/(params.L2_ASSOC * params.BLOCKSIZE);
      l2_index = static_cast<uint32_t>(std::log2(l2_set));
      l2_tag = 32 - l2_index - block_offset;
      l2_way = params.L2_ASSOC;
      //printf("l2_set=%u\n",l2_set);
      //printf("l2_index=%u\n",l2_index);
      //printf("l2_tag=%u\n",l2_tag);
      //printf("l2_way=%u\n",l2_way);
   }
   else {
      l2_set = 0;
      l2_index = 0;
      l2_tag = 0;
      l2_way = 0;   
      //printf("l2_set=%u\n",l2_set);
      //printf("l2_index=%u\n",l2_index);
      //printf("l2_tag=%u\n",l2_tag);
      //printf("l2_way=%u\n",l2_way);   
   }
   cache l2Cache(nullptr, rw, l2_way, l2_set, addr, l2_index, l2_tag, block_offset, perf_n,  perf_m);
   cache l1Cache(&l2Cache, rw, l1_way, l1_set, addr, l1_index, l1_tag, block_offset, perf_n, perf_m);
   l1Cache.setup();
   l2Cache.setup();
   ////////////////////////////////////////////////////////////////////////////////////////////
   // Read requests from the trace file and echo them back.
   while (fscanf(fp, "%c %x\n", &rw, &addr) == 2) {	// Stay in the loop if fscanf() successfully parsed two tokens as specified.     
      if (rw == 'r' || rw == 'w'){        
         status = l1Cache.request_fn(rw, addr);
         //printf("I am Here file access\n");
      }
      else {
         printf("Error: Unknown request type %c.\n", rw);
	      exit(EXIT_FAILURE);
      }

      ///////////////////////////////////////////////////////
      // Issue the request to the L1 cache instance here.
      ///////////////////////////////////////////////////////
      
    }
   cache_set_t** l1_cache_sorted = sort_cache_lru(l1Cache.lru_counter, l1Cache.lx_cache, l1_set, l1_way);
   printf("===== L1 contents =====\n");
   for (uint32_t i = 0; i < l1_set; i++) {
      printf("set      %d:   ", i);
      for (uint32_t j = 0; j < l1_way; ++j) {
            dirty = ' ';
            if(l1_cache_sorted[i][j].DIRTY_BIT == 1){
                dirty = 'D';
            }
            //printf("set      %d:   %x %x %c\n", i, l1Cache.lx_cache[i][j].TAG, l1Cache.lx_cache[i][j].ADDR, dirty);
            printf("%x %c ", l1_cache_sorted[i][j].TAG, dirty);
      }
      printf("\n");
   }
   if (params.L2_SIZE > 0){
        printf("\n");
        cache_set_t** l2_cache_sorted = sort_cache_lru(l2Cache.lru_counter, l2Cache.lx_cache, l2_set, l2_way);
        printf("===== L2 contents =====\n");
        for (uint32_t i = 0; i < l2_set; i++) {
            printf("set      %d:   ", i);
            for (uint32_t j = 0; j < l2_way; ++j) {
                    dirty = ' ';
                    if(l2_cache_sorted[i][j].DIRTY_BIT == 1){
                        dirty = 'D';
                    }
                    //printf("set      %d:   %x %x %c\n", i, l1Cache.lx_cache[i][j].TAG, l1Cache.lx_cache[i][j].ADDR, dirty);
                    printf("%x %c ", l2_cache_sorted[i][j].TAG, dirty);
            }
            printf("\n");
        }
   }
   if((perf_n > 0 || perf_m > 0) && params.L2_SIZE > 0){
    printf("\n===== Stream Buffer(s) contents =====\n");
    for (uint32_t i = 0; i < perf_n; ++i) {
        for (uint32_t j = 0; j < perf_m; ++j) {
            printf("%x\t",l2Cache.sbuf[i].TAG[j]); 
            //printf("LRU=%d\t",l1Cache.sbuf[i].LRU_COUNTER); 
        }   
        printf("\n");
    }
   } 
   if ((perf_n > 0 || perf_m) > 0 && params.L2_SIZE == 0){
   printf("\n===== Stream Buffer(s) contents =====\n");
    for (uint32_t i = 0; i < perf_n; ++i) {
        for (uint32_t j = 0; j < perf_m; ++j) {
            printf("%x\t",l1Cache.sbuf[i].TAG[j]); 
            //printf("LRU=%d\t",l1Cache.sbuf[i].LRU_COUNTER); 
        }   
        printf("\n");
    }
   } 
///////////////////////////////////////////////////////////////////////////////////////
    uint32_t mem_l1;
    uint32_t mem_l2;
    double total_accesses_l1 = l1Cache.lx_reads + l1Cache.lx_writes;
    double l1_miss_rate = (total_accesses_l1 > 0) ? 
                      static_cast<double>(l1Cache.lx_read_misses + l1Cache.lx_write_misses) / total_accesses_l1 
                      : 0.0; // Avoid division by zero
    double total_accesses_l2 = l2Cache.lx_reads;// + l2Cache.lx_writes;
    double l2_miss_rate = (total_accesses_l2 > 0) ? 
                      static_cast<double>(l2Cache.lx_read_misses) / total_accesses_l2 
                      : 0.0; // Avoid division by zero
    if(perf_n > 0 || perf_m > 0 && params.L2_SIZE == 0){
        mem_l1 = l1Cache.lx_read_misses + l1Cache.lx_write_misses + l1Cache.lx_writebacks + l1Cache.lx_prefetches;
    } else if(perf_n > 0 || perf_m > 0 && params.L2_SIZE > 0){
        mem_l2 = l2Cache.lx_read_misses + l2Cache.lx_write_misses + l2Cache.lx_writebacks + l2Cache.lx_prefetches;
    }
///////////////////////////////////////////////////////////////////////////////////////
   printf("\n ===== Measurements =====\n");
   printf("a. L1 reads:                   %d\n",l1Cache.lx_reads);
   printf("b. L1 read misses:             %d\n",l1Cache.lx_read_misses);
   printf("c. L1 writes:                  %d\n",l1Cache.lx_writes);
   printf("d. L1 write misses:            %d\n",l1Cache.lx_write_misses);
   printf("e. L1 miss rate:               %.4f\n",l1_miss_rate);
   printf("f. L1 writebacks:              %d\n",l1Cache.lx_writebacks);
   printf("g. L1 prefetches:              %d\n",l1Cache.lx_prefetches);
   printf("h. L2 reads (demand):          %d\n",l2Cache.lx_reads);
   printf("i. L2 read misses (demand):    %d\n",l2Cache.lx_read_misses);
   printf("j. L2 reads (prefetch):        %d\n",0);
   printf("k. L2 read misses (prefetch):  %d\n",0);
   printf("l. L2 writes:                  %d\n",l2Cache.lx_writes);
   printf("m. L2 write misses:            %d\n",l2Cache.lx_write_misses);
   printf("n. L2 miss rate:               %.4f\n",l2_miss_rate);
   printf("o. L2 writebacks:              %d\n",l2Cache.lx_writebacks);
   printf("p. L2 prefetches:              %d\n",l2Cache.lx_prefetches);
   if(perf_n > 0 || perf_m > 0 && params.L2_SIZE == 0){
    printf("q. memory traffic:             %d\n",mem_l1);
   }  else if(perf_n > 0 || perf_m > 0 && params.L2_SIZE > 0){
    printf("q. memory traffic:             %d\n",mem_l2);
   }  else {
    printf("q. memory traffic:             %d\n",l1Cache.mem_access_cnt + l2Cache.mem_access_cnt);
   }
    return(0);
}