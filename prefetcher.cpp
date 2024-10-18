#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "sim.h"
// Debug switch
#define DEBUG 0


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
    public:

        // Constructor definition
        cache(cache* nextCache, char req, uint32_t w, uint32_t s, uint32_t a,
          uint32_t idx, uint32_t tag, uint32_t blockOff)
        : next_cache(nextCache), request(req), ways(w), sets(s),
          addr(a), index(idx), Tag(tag), block_offset(blockOff) {
    }