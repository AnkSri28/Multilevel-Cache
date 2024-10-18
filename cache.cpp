#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <algorithm>

#include "sim.h"
// Debug switch
#define DEBUG 0
#define DEBUG_P 1


class cache{
    private:
        //void processStruct(const cache_set_t& Cache_set_t);
        cache* next_cache;
        char request;
        uint32_t ways;
        uint32_t sets;
        uint32_t addr;
        uint32_t index;
        uint32_t Tag;
        uint32_t block_offset;   
        uint32_t perf_n;//no of sbufs
        uint32_t perf_m;// no of blocks    
    public:
        uint32_t location_in_way;
        /////////USED WHEN REPLACEMENT IS NEEDED////////////
        char req_lx_next;
        uint32_t eject_address;
        ////////////////////////////////////////////////////
        bool hit = false;
        bool miss = false;
        uint32_t hit_tag_way;
        ////////////////////////////////////////////////////
        uint32_t lx_reads=0;
        uint32_t lx_read_misses=0;
        uint32_t lx_writes=0;
        uint32_t lx_write_misses=0;
        float lx_miss_rate=0;
        uint32_t lx_writebacks=0;
        uint32_t lx_prefetches=0;
        uint32_t mem_access_cnt=0;
        ////////////////////////////////////////////////////
        uint32_t prefetch_tag;
        bool prefetch_hit;
        ////////////////////////////////////////////////////

        // Constructor definition
        cache(cache* nextCache, char req, uint32_t w, uint32_t s, uint32_t a,
          uint32_t idx, uint32_t tag, uint32_t blockOff, uint32_t no_sbuf, uint32_t sbuf_size )
        : next_cache(nextCache), request(req), ways(w), sets(s),
          addr(a), index(idx), Tag(tag), block_offset(blockOff), perf_n(no_sbuf), perf_m(sbuf_size)  {
    }
        // Step 1: Allocate an array of pointers to cache_set_t
        cache_set_t** lx_cache = new cache_set_t*[sets];
        int** lru_counter = new int*[sets];
        //int* lru_counter_sbuf = new int[perf_n];
        stream_buf_t* sbuf = new stream_buf_t[perf_n];
        //stream_buf_blk_t* tag_array = new stream_buf_blk_t[perf_m]

    void setup(){
        // Step 2: Allocate each row as an array of cache_set_t
        //printf("Hello\n");
        for (uint32_t i = 0; i < sets; i++) {
            lx_cache[i] = new cache_set_t[ways];
            lru_counter[i] = new int[ways];
        }
        for (uint32_t i = 0; i < sets; i++) {
            for (uint32_t j = 0; j < ways; ++j) {
                lx_cache[i][j].VALID_BIT = 0;
                lx_cache[i][j].DIRTY_BIT = 0;
                lx_cache[i][j].TAG = 0;
                lx_cache[i][j].ADDR = 0;
                lru_counter[i][j] = j; 
                //printf("lru_count[%d][%d]=%d\n",i,j,lru_counter[i][j]);
            }
        }
    for (uint32_t i = 0; i < perf_n; ++i) {
        sbuf[i].VALID = 0; // Initialize VALID
        sbuf[i].LRU_COUNTER = i; // Allocate LRU_COUNTER for each stream_buf_t

        // Allocate TAG for each stream_buf_t (2D array of uint32_t)
        sbuf[i].TAG = new uint32_t[perf_m]; // Allocate an array of pointers
        for (uint32_t j = 0; j < perf_m; ++j) {
            sbuf[i].TAG[j] = 0; // Initialize each TAG element to 0
        }   
    }   
    }
    


bool request_fn(char request, uint32_t addr) {
    uint32_t idx;
    uint32_t tag;
    bool status = false;

    // Calculate index and tag from the address
    uint32_t block_offset_mask = (1 << block_offset) - 1;
    uint32_t block_offset_bits = addr & block_offset_mask;

    uint32_t idx_mask = (1 << index) - 1;
    idx = (addr >> block_offset) & idx_mask;

    tag = addr >> (index + block_offset);
    prefetch_tag = addr >> block_offset;

    // Debugging output
    if (DEBUG) {
        printf("================================================\n");
        printf("Address: %x, Index: %u, Block Offset: %u, Tag: %x\n", addr, idx, block_offset_bits, tag);
    }

    cache_set_t* tag_row = lx_cache[idx];

    // Check for hit in the current cache set
    for (uint32_t i = 0; i < ways; i++) {
        if (tag == tag_row[i].TAG && tag_row[i].VALID_BIT) {
            if (DEBUG) printf("Hit in L1 cache at index %u, way %u\n", idx, i);
            hit = true;
            hit_tag_way = i;

            // LRU update
            location_in_way = lru_counter_update(idx, ways, hit_tag_way, hit);
            if (request == 'w') {
                lx_writes++;
                tag_row[location_in_way].DIRTY_BIT = 1;
                tag_row[location_in_way].ADDR = addr;
                tag_row[location_in_way].VALID_BIT = 1;

                if (DEBUG) {
                    printf("Write hit: TAG=%x ADDR=%x DIRTY=%d\n", tag_row[location_in_way].TAG, tag_row[location_in_way].ADDR, tag_row[location_in_way].DIRTY_BIT);
                }
            } else {
                lx_reads++;
                tag_row[location_in_way].ADDR = addr;
                tag_row[location_in_way].VALID_BIT = 1;

                if (DEBUG) {
                    printf("Read hit: TAG=%x ADDR=%x DIRTY=%d\n", tag_row[location_in_way].TAG, tag_row[location_in_way].ADDR, tag_row[location_in_way].DIRTY_BIT);
                }
            }
            if (perf_n > 0 && perf_m > 0 && next_cache != nullptr && next_cache->sets == 0 && next_cache->ways == 0) {
                for(uint32_t j=0;j<perf_n;j++){
                    prefetch_hit = stream_buf_check(j,prefetch_tag); // Check the Stream buffer for a hit
                    if(prefetch_hit == true){
                        //printf("CACHE HIT and SBUF HIT\n");
                        return true;                 
                    }
                }
            }
            //printf("CACHE HIT and SBUF MISS\n");
        return true;
        }
    }
    // Miss handling
    if (next_cache != nullptr && (next_cache->sets > 0) && (next_cache->ways > 0)) { 
        location_in_way = lru_counter_update(idx, ways); // Update LRU
        if (tag_row[location_in_way].DIRTY_BIT == 1 && tag != tag_row[location_in_way].TAG) {
            if (DEBUG) printf("TRYING to EVICT\n");
            if (DEBUG) {
                printf("EVICT: TAG=%x ADDR=%x DIRTY=%d\n", tag_row[location_in_way].TAG, tag_row[location_in_way].ADDR, tag_row[location_in_way].DIRTY_BIT);
            }
            lx_writebacks++;
            req_lx_next = 'w';
            eject_address = tag_row[location_in_way].ADDR;

            bool l2_hit = next_cache->request_fn(req_lx_next, eject_address);
            if (DEBUG) {
                printf("L2 Miss Handling: Ejecting Address=%x, L2 Hit=%d\n", eject_address, l2_hit);
            }
            //mem_access_cnt++;
            tag_row[location_in_way].ADDR = addr;
            tag_row[location_in_way].DIRTY_BIT = 0; // Reset dirty after eviction
        }


        // Handle the read request to L2
        if (DEBUG) printf("TRYING to READ from L2\n");
        req_lx_next = 'r';
        eject_address = addr;
        bool l2_read = next_cache->request_fn(req_lx_next, eject_address);
        if (DEBUG) printf("ALREADY READ from L2\n");

        if (DEBUG) {
            printf("Requesting L2: Address=%x, Read Result=%d\n", addr, l2_read);
        }

        if (request == 'w') {   
            lx_writes++;
            lx_write_misses++;
            tag_row[location_in_way].DIRTY_BIT = 1;
            tag_row[location_in_way].TAG = tag; // Update L1
            tag_row[location_in_way].VALID_BIT = 1;
            tag_row[location_in_way].ADDR = addr;

            if (DEBUG) {
                printf("Write to L1: TAG=%x, ADDR=%x, DIRTY=1\n", tag, addr);
            }
        } else {
            lx_read_misses++;
            lx_reads++;
            tag_row[location_in_way].DIRTY_BIT = 0; // Reset dirty on read
            tag_row[location_in_way].TAG = tag; // Update L1
            tag_row[location_in_way].VALID_BIT = 1;  
            tag_row[location_in_way].ADDR = addr;

            if (DEBUG) {
                printf("Read to L1: TAG=%x, ADDR=%x, DIRTY=0\n", tag, addr);
            }
        }
        return true; // Return after handling L2
    } else {
        //////////////////////////////////////PREFETCH/////////////////////////////////////////////// 
        if(perf_n > 0 && perf_m > 0){
            for(uint32_t j=0;j<perf_n;j++){
                //printf("Checking %d\n",j);
                prefetch_hit = stream_buf_check(j,prefetch_tag); // Check the Stream buffer for a hit
                //printf("prefetch_hit = %d\n",prefetch_hit);
                if(prefetch_hit == true){
                    break;
                }
            }
                if(prefetch_hit == true){
                    //printf("CACHE MISS and SBUF HIT\n");
                    location_in_way = lru_counter_update(idx, ways); // LRU update
                    if (request == 'w') {
                        if(tag_row[location_in_way].DIRTY_BIT == 1){
                            lx_writebacks++;
                            mem_access_cnt++;
                        }
                        lx_writes++;
                        //lx_write_misses++;
                        tag_row[location_in_way].DIRTY_BIT = 1;
                        tag_row[location_in_way].VALID_BIT = 1;
                        tag_row[location_in_way].ADDR = addr;
                        tag_row[location_in_way].TAG = tag;

                    if (DEBUG_P) {
                        printf("Write to Memory: TAG=%x, ADDR=%x\n", tag, addr);
                    }
                    } else {
                        if(tag_row[location_in_way].DIRTY_BIT == 1){
                            lx_writebacks++;
                            mem_access_cnt++;
                        }
                        lx_reads++;
                        //lx_read_misses++;
                        tag_row[location_in_way].DIRTY_BIT = 0; // Reset dirty on read
                        tag_row[location_in_way].VALID_BIT = 1;
                        tag_row[location_in_way].ADDR = addr;
                        tag_row[location_in_way].TAG = tag;

                    if (DEBUG_P) {
                        printf("Read to Memory: TAG=%x, ADDR=%x\n", tag, addr);                                 
                    }
                }
            return true;
            }
            else{
                //printf("CACHE MISS and SBUF MISS\n");
                stream_buf_update(prefetch_tag);
                    location_in_way = lru_counter_update(idx, ways); // LRU update
                    if (request == 'w') {
                        if(tag_row[location_in_way].DIRTY_BIT == 1){
                            lx_writebacks++;
                            mem_access_cnt++;
                        }
                        lx_writes++;
                        lx_write_misses++;
                        tag_row[location_in_way].DIRTY_BIT = 1;
                        tag_row[location_in_way].VALID_BIT = 1;
                        tag_row[location_in_way].ADDR = addr;
                        tag_row[location_in_way].TAG = tag;

                    if (DEBUG_P) {
                        printf("Write to Memory: TAG=%x, ADDR=%x\n", tag, addr);
                    }
                    } else {
                        if(tag_row[location_in_way].DIRTY_BIT == 1){
                            lx_writebacks++;
                            mem_access_cnt++;
                        }
                        lx_reads++;
                        lx_read_misses++;
                        tag_row[location_in_way].DIRTY_BIT = 0; // Reset dirty on read
                        tag_row[location_in_way].VALID_BIT = 1;
                        tag_row[location_in_way].ADDR = addr;
                        tag_row[location_in_way].TAG = tag;

                    if (DEBUG_P) {
                        printf("Read to Memory: TAG=%x, ADDR=%x\n", tag, addr);                                 
                    }
                }
            return true;
            }
        }
        //////////////////////////////////////PREFETCH///////////////////////////////////////////////
        // If both L1 and L2 miss, handle memory access
         // Increment memory access count
        location_in_way = lru_counter_update(idx, ways); // LRU update
        if (request == 'w') {
            if(tag_row[location_in_way].DIRTY_BIT == 1){
                lx_writebacks++;
                mem_access_cnt++;
            }
            lx_writes++;
            lx_write_misses++;
            mem_access_cnt++;
            tag_row[location_in_way].DIRTY_BIT = 1;
            tag_row[location_in_way].VALID_BIT = 1;
            tag_row[location_in_way].ADDR = addr;
            tag_row[location_in_way].TAG = tag;

            if (DEBUG) {
                printf("Write to Memory: TAG=%x, ADDR=%x\n", tag, addr);
            }
        } else {
            if(tag_row[location_in_way].DIRTY_BIT == 1){
                lx_writebacks++;
                mem_access_cnt++;
            }
            lx_reads++;
            lx_read_misses++;
            mem_access_cnt++;
            tag_row[location_in_way].DIRTY_BIT = 0; // Reset dirty on read
            tag_row[location_in_way].VALID_BIT = 1;
            tag_row[location_in_way].ADDR = addr;
            tag_row[location_in_way].TAG = tag;

            if (DEBUG) {
                printf("Read to Memory: TAG=%x, ADDR=%x\n", tag, addr);
            }
        }
    }

    return status; // Final return status
}


uint32_t lru_counter_update(uint32_t index, uint32_t way, uint32_t hit_way = 0, bool hit = false) {
    // Initialize location for the least recently used (LRU)
    uint32_t location_in_way = 0;
    // If there is a hit
    if (hit) {
        // Update the hit way to be the most recently used (MRU)
        uint32_t hit_lru = lru_counter[index][hit_way];
        /*if (DEBUG) {
            printf("UPDATE_LRU=%d, lru_counter[%d][%d]=%d\n", hit_lru, index, hit_way, lru_counter[index][hit_way]);
        }*/

        // Set the hit way's counter to 0 (most recently used)
        lru_counter[index][hit_way] = 0; 

        // Update other counters
        for (uint32_t i = 0; i < way; i++) {
            if (i != hit_way) {
                if (lru_counter[index][i] < hit_lru) {
                    lru_counter[index][i]++; // Increment the counter for other ways
                }
                
                /*if (DEBUG) {
                    printf("After HIT: lru_counter[%d][%d]=%d\n", index, i, lru_counter[index][i]);
                }*/
            }
        }
        
        return hit_way; // Return the hit way
    } else {
        // Update for a miss: find the least recently used (LRU) way
        for (uint32_t i = 0; i < way; i++) {
            lru_counter[index][i]++;
        }
        // Identify the LRU way
        for (uint32_t j = 0; j < way; j++) {
            if (lru_counter[index][j] == way) {
                lru_counter[index][j] = 0; 
                location_in_way = j;
            }
        }
        // Debug print for all LRU counters
        /*if (DEBUG) {
            for (uint32_t j = 0; j < way; j++) {
                printf("LRU[%d][%d]=%d\n", index, j, lru_counter[index][j]);
            }
        }*/       
        return location_in_way; // Return the location of the replaced way
    }
}
//////////////////////////////////PREFETCH////////////////////////////////////////////////////////////////////////////
uint32_t stream_buf_update(uint32_t prefetch_tag){ // Function to update the prefetcher after miss
    uint32_t i=perf_n-1;
    sort_stream_buf(sbuf, perf_n);
    for (uint32_t k = 0; k < perf_m; k++) {
        sbuf[i].TAG[k] = prefetch_tag + k + 1; //stride
        if (DEBUG_P) {
            printf("MISS_SBUF=%d, lru_counter_sbuf[%d]=%x\n", i,  sbuf[i].LRU_COUNTER, sbuf[i].TAG[k]);
        }
        mem_access_cnt++;
        lx_prefetches++;
    }
    stream_buf_lru_update(i);
    sort_stream_buf(sbuf, perf_n);
    return 0;
}

uint32_t stream_buf_update_cnt(uint32_t prefetch_tag){ // Function to update the prefetcher after miss
    uint32_t i=perf_n-1;
    for (uint32_t k = 0; k < perf_m; k++) {
        mem_access_cnt++;
        lx_prefetches++;
    }
    return 0;
}

bool stream_buf_check(uint32_t i, uint32_t tag) { // Check the Stream buffer for a hit
    uint32_t hit_tag_perf_m; 
    uint32_t offset;
    
    // Sort the stream buffer before checking
    sort_stream_buf(sbuf, perf_n);
    
    // Retrieve the last tag for incrementing
    uint32_t tag_inc = sbuf[i].TAG[perf_m - 1];

    for (uint32_t j = 0; j < perf_m; j++) {
        if (DEBUG_P) {
            printf("BUF%d Checking TAG[%d]: %x for %x\n", i, j, sbuf[i].TAG[j], tag);
        }
        
        if (sbuf[i].TAG[j] == tag) {
            hit_tag_perf_m = j;
            offset = perf_m - hit_tag_perf_m;

            // Shift elements up to overwrite those above 'tag'
            for (uint32_t k = 0; k < offset; k++) {
                if (hit_tag_perf_m + k < perf_m) {
                    sbuf[i].TAG[k] = sbuf[i].TAG[hit_tag_perf_m + 1 + k];
                    if (DEBUG_P) {
                        printf("Shifting TAG[%d] to TAG[%d]: %x\n", hit_tag_perf_m + k, k, sbuf[i].TAG[k]);
                    }
                }
            }
            for (uint32_t k = offset - 1; k < perf_m; k++) {
                tag_inc = tag_inc + 1;
                sbuf[i].TAG[k] = tag_inc;
                if (DEBUG_P) {
                    printf("Filling TAG[%d] with: %x\n", k, sbuf[i].TAG[k]);
                }
                mem_access_cnt++;
                lx_prefetches++;
            }
            stream_buf_lru_update(i);
            sort_stream_buf(sbuf, perf_n);
            return true; // Hit found
        } else {
            if (DEBUG_P) {
                printf("Miss for TAG[%d]: %x (searched TAG: %x)\n", j, sbuf[i].TAG[j], tag);
            }
        }
    }
    
    //stream_buf_update(tag);
    sort_stream_buf(sbuf, perf_n);
    return false; // No hit found
}


void stream_buf_lru_update(uint32_t lru){ // Update the lru counter for the prefetcher used in other functions
    uint32_t hit_lru = sbuf[lru].LRU_COUNTER;
    if (DEBUG_P) {
        printf("HIT_LRU=%x, lru_counter_sbuf[%d]\n", hit_lru,  sbuf[lru].LRU_COUNTER);
    }

    // Set the hit way's counter to 0 (most recently used)
     sbuf[lru].LRU_COUNTER = 0; 

    // Update other counters
    for (uint32_t i = 0; i < perf_n; i++) {
        if (i != hit_lru) {
            if ( sbuf[lru].LRU_COUNTER < hit_lru) {
                 sbuf[i].LRU_COUNTER++; // Increment the counter for other ways
            }            
            if (DEBUG_P) {
                for (uint32_t j = 0; j < perf_m; j++) {
                    printf("After HIT: lru_counter_sbuf[%d]=%x, tag=%x\n", i,  sbuf[i].LRU_COUNTER, sbuf[i].TAG[j]);
                }
            }
        }
    }
}

void sort_stream_buf(stream_buf_t* sbuf, uint32_t perf_n) {
    //if (DEBUG_P) {
    //    printf("Sorting stream_buf_t based on LRU_COUNTER:\n");
    //}

    for (uint32_t i = 0; i < perf_n - 1; ++i) {
        for (uint32_t j = i + 1; j < perf_n; ++j) {
            // Print current TAG and LRU_COUNTER values before comparison
            /*if (DEBUG_P) {
                printf("Comparing TAG[%d]: %x (LRU_COUNTER: %d) with TAG[%d]: %x (LRU_COUNTER: %d)\n", 
                       i, sbuf[i].TAG[0], sbuf[i].LRU_COUNTER, 
                       j, sbuf[j].TAG[0], sbuf[j].LRU_COUNTER);
            }*/

            // Compare LRU_COUNTERs
            if (sbuf[i].LRU_COUNTER > sbuf[j].LRU_COUNTER) {
                // Print the swap action
                if (DEBUG_P) {
                    printf("Swapping index %d and index %d\n", i, j);
                }
                
                // Swap stream_buf_t elements
                std::swap(sbuf[i], sbuf[j]);
            }
        }
    }

    if (DEBUG_P) {
        printf("Sorting complete. Sorted LRU_COUNTERs:\n");
        for (uint32_t i = 0; i < perf_n; ++i) {
            printf("TAG[%d]: %x, LRU_COUNTER: %d\n", i, sbuf[i].TAG[0], sbuf[i].LRU_COUNTER);
        }
    }
}

};