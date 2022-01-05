
#include "mipssim.h"

uint32_t cache_type = 0;

#define BLOCK_SIZE 16   //bytes
#define BLOCK_SIZE_W 4   //words
#define OFFSET_BITS 4   //lg base2 (16) = 4
#define WORD_SIZE_BITS 32
#define BLOCKS_PER_SET 2

struct block_struct{
    int valid_bit;
    uint32_t block_tag;
    uint32_t data[BLOCK_SIZE_W];
};

struct set_struct{
    struct block_struct block[BLOCKS_PER_SET];
};

struct dmf_cache{
    struct block_struct* block;
}dmf_cache;

struct sa_cache{
    struct set_struct* set;
}sa_cache;



int num_of_blocks, num_of_sets, index_bits, tag_bits;

void memory_state_init(struct architectural_state *arch_state_ptr) {
    arch_state_ptr->memory = (uint32_t *) malloc(sizeof(uint32_t) * MEMORY_WORD_NUM);
    memset(arch_state_ptr->memory, 0, sizeof(uint32_t) * MEMORY_WORD_NUM);
    if (cache_size == 0) {
        // CACHE DISABLED
        memory_stats_init(arch_state_ptr, 0); // WARNING: we initialize for no cache 0
    } else {
        // CACHE ENABLED
        //assert(0); /// @students: remove assert and initialize cache
        /// @students: memory_stats_init(arch_state_ptr, X); <-- fill # of tag bits for cache 'X' correctly
        
        switch(cache_type) {
        case CACHE_TYPE_DIRECT: // direct mapped
            num_of_blocks = cache_size / BLOCK_SIZE;
            index_bits = log2((double) num_of_blocks);
            printf("Number of bits for index: %u \n", index_bits);
            tag_bits = WORD_SIZE_BITS - OFFSET_BITS - index_bits;
            printf("Number of bits for tag: %u \n", tag_bits);
            memory_stats_init(arch_state_ptr, tag_bits);
            
            dmf_cache.block = (struct block_struct *) calloc(num_of_blocks, sizeof(struct block_struct));
            
            printf("Memory initialized for Direct-mapped cache. \n");    
            break;

        case CACHE_TYPE_FULLY_ASSOC: // fully associative
            tag_bits = WORD_SIZE_BITS - OFFSET_BITS;
            index_bits = 0;
            memory_stats_init(arch_state_ptr, tag_bits);
            
            dmf_cache.block = (struct block_struct *) calloc(num_of_blocks, sizeof(struct block_struct));
            
            break;

        case CACHE_TYPE_2_WAY: // 2-way associative
            num_of_blocks = cache_size / BLOCK_SIZE;
            num_of_sets = num_of_blocks / BLOCKS_PER_SET;
            index_bits = log2((double) num_of_sets);
            tag_bits = WORD_SIZE_BITS - OFFSET_BITS - index_bits;
            memory_stats_init(arch_state_ptr, tag_bits);

            sa_cache.set = (struct set_struct *) calloc(num_of_sets, sizeof(struct set_struct));
            
            //set.block = (struct block_struct *) calloc(num_of_blocks, sizeof(struct block_struct));
            
            //v_arr_set_asc = calloc(num_of_sets, sizeof(int*));
            //for (int i=0; i<num_of_sets; i++)
            //    v_arr_set_asc[i] = calloc(BLOCKS_PER_SET, sizeof(int));
            
            //t_arr_set_asc = calloc(num_of_sets, sizeof(uint32_t*));
            //for (int i=0; i<num_of_sets; i++)                                       //same as v_arr but each entry has many bits so uint32
            //    t_arr_set_asc[i] = calloc(BLOCKS_PER_SET, sizeof(uint32_t));
            
            //d_arr_set_asc = calloc(num_of_sets, sizeof(uint32_t**));
            //for (int i=0; i<num_of_sets; i++) {
            //    d_arr_set_asc[i] = calloc(BLOCKS_PER_SET, sizeof(uint32_t*));
            //    for (int j=0; j<BLOCKS_PER_SET; j++) {
            //        d_arr_set_asc[i][j] = calloc(BLOCK_SIZE_W, sizeof(uint32_t));
            //    }    
            //d_arr = (uint32_t *) malloc(num_of_sets * BLOCKS_PER_SET * BLOCK_SIZE_W * sizeof(uint32_t));   //3-D array. Cache is an array (num of sets) 
            //}                                                                                               //of 2 arrays (2 blocks per set) of arrays 
            //lru_arr = calloc(num_of_sets, sizeof(int*));                                                                                            ///(1 array for each 32-bit word, ie. 4 arrays).
            //for (int i=0; i<num_of_sets; i++)
            //    lru_arr[i] = calloc(BLOCKS_PER_SET, sizeof(int));           
            break;
        }
    }
}

// returns data on memory[address / 4]
int memory_read(int address){
    arch_state.mem_stats.lw_total++;
    check_address_is_word_aligned(address);

    if (cache_size == 0) {
        // CACHE DISABLED
        return (int) arch_state.memory[address / 4];
    } else {
        // CACHE ENABLED
        //assert(0); /// @students: Remove assert(0); and implement Memory hierarchy w/ cache
        /// @students: your implementation must properly increment: arch_state_ptr->mem_stats.lw_cache_hits
        
    

        //uint32_t tag = address >> (WORD_SIZE_BITS - tag_bits);
        //idx = ((address << tag_bits) >> (WORD_SIZE_BITS - index_bits));
        //word_offset = (address << (WORD_SIZE_BITS - OFFSET_BITS)) >> (WORD_SIZE_BITS - OFFSET_BITS +2);
        uint32_t tag_idx_conc = (address>>OFFSET_BITS)<<2;
        int word_offset = get_piece_of_a_word(address, 2, 2);
        int idx = get_piece_of_a_word(address, OFFSET_BITS, index_bits);
        uint32_t tag = get_piece_of_a_word(address,(OFFSET_BITS + index_bits), tag_bits);
        //uint32_t tag_idx_conc = get_piece_of_a_word(address, OFFSET_BITS, tag_bits + index_bits) << 2;
        int t = 0;
        int tt = 0; 

        printf("tag in decimal = %d \n", tag);
        printf("index in decimal = %d \n", idx);
        printf("word offset in decimal = %d \n", word_offset);

        switch(cache_type) {
        case CACHE_TYPE_DIRECT: // direct mapped
            if (dmf_cache.block[idx].valid_bit == 1 && dmf_cache.block[idx].block_tag == tag) {
                arch_state.mem_stats.lw_cache_hits++;
                return (int) dmf_cache.block[idx].data[word_offset];
                   
            } else {
                for (int i=0; i<BLOCK_SIZE_W; i++) {
                    int adrs = (tag_idx_conc | i);
                    dmf_cache.block[idx].data[i] = (int) arch_state.memory[ adrs ];
                }    
                dmf_cache.block[idx].valid_bit = 1;
                dmf_cache.block[idx].block_tag = tag;  
                return (int) arch_state.memory[address / 4];  
            }
            printf("Performing a memory read on a Direct-mapped cache. \n");
            break;
        case CACHE_TYPE_FULLY_ASSOC: // fully associative
            
            for (int i=0; i<num_of_blocks; i++) {
                if (dmf_cache.block[i].valid_bit == 1 && dmf_cache.block[i].block_tag == tag){
                    arch_state.mem_stats.lw_cache_hits++;
                    t = 1;
                    return (int) dmf_cache.block[i].data[word_offset];
                } 
            }   
            if (t == 0){    
                uint32_t padded_tag = tag << 2;
                for(int i=0; i<num_of_blocks; i++) {
                    if (dmf_cache.block[i].valid_bit == 0 && dmf_cache.block[i].block_tag == 0) {
                        for (int j=0; j<BLOCK_SIZE_W; j++) 
                            dmf_cache.block[i].data[j] = (int) arch_state.memory[(padded_tag | j)];
                        dmf_cache.block[i].valid_bit = 1;
                        dmf_cache.block[i].block_tag = tag;
                        tt = 1;   
                    }
                }        
                return (int) arch_state.memory[address / 4];
            }  
            
            if (tt == 0 && t == 0){

            }            
            
            break;
        
        case CACHE_TYPE_2_WAY: // 2-way associative
        
            for (int i=0; i<BLOCKS_PER_SET; i++) {
                if (sa_cache.set[idx].block[i].valid_bit == 1 && sa_cache.set[idx].block[i].block_tag == tag) {
                    arch_state.mem_stats.lw_cache_hits++;
                    t = 1;
                    return (int) sa_cache.set[idx].block[i].data[word_offset];
                } 
            }           
                    //for (int j=0; j<BLOCKS_PER_SET; j++)
                    //    lru_arr[idx][j] = 0;
                    //lru_arr[idx][i] = 1;         // block with lru value 0 is the least recently accessed.   
                    
            if (t == 0){                     //block is not in the cache so we must fetch it from the memory.
                
                for (int i=0; i<BLOCKS_PER_SET; i++) {
                    if (sa_cache.set[idx].block[i].valid_bit == 0 && sa_cache.set[idx].block[i].block_tag == 0) {
                        for (int j=0; j<BLOCK_SIZE_W; j++)
                            sa_cache.set[idx].block[i].data[j] = (int) arch_state.memory[(tag_idx_conc | j)];
                        sa_cache.set[idx].block[i].valid_bit  = 1;
                        sa_cache.set[idx].block[i].block_tag = tag;
                        tt = 1;   
                    }    
                }       
                return (int) arch_state.memory[address / 4];            
            }    
            if (tt == 0 && t == 0){                   //no block is free in the set so we use LRU replacement policy.
                //for (int i=0; i<BLOCKS_PER_SET; i++)
                //    if (lru_arr[idx][i] == 0) {
                //        for (int j=0; j<BLOCK_SIZE_W; j++)
                //            d_arr_set_asc[idx][i][j] = (int) arch_state.memory[(tag_idx_conc | i)];
                //        v_arr_set_asc[idx][i] = 1;
                //        t_arr_set_asc[idx][i] = tag;    
            }    
                      
            break;
        }
    }
    return 0;
}

// writes data on memory[address / 4]
void memory_write(int address, int write_data) {
    arch_state.mem_stats.sw_total++;
    check_address_is_word_aligned(address);

    if (cache_size == 0) {
        // CACHE DISABLED
        arch_state.memory[address / 4] = (uint32_t) write_data;
    } else {
        // CACHE ENABLED
        //assert(0); /// @students: Remove assert(0); and implement Memory hierarchy w/ cache 
        /// @students: your implementation must properly increment: arch_state_ptr->mem_stats.sw_cache_hits
        

        uint32_t tag = address >> (WORD_SIZE_BITS - tag_bits);
        int idx = ((address << tag_bits) >> (WORD_SIZE_BITS - index_bits));
        int word_offset = (address << (WORD_SIZE_BITS - OFFSET_BITS)) >> (WORD_SIZE_BITS - OFFSET_BITS +2);        
        int t = 0;
        

        switch(cache_type) {
        case CACHE_TYPE_DIRECT: // direct mapped
            if (dmf_cache.block[idx].valid_bit == 1 && dmf_cache.block[idx].block_tag == tag){
                arch_state.mem_stats.sw_cache_hits++;
                dmf_cache.block[idx].data[word_offset] = (uint32_t) write_data;
                arch_state.memory[address / 4] = (uint32_t) write_data;
            } else {
                arch_state.memory[address / 4] = (uint32_t) write_data;
            }    
            break;
        case CACHE_TYPE_FULLY_ASSOC: // fully associative
            for (int i=0; i<num_of_blocks; i++)
                if (dmf_cache.block[i].valid_bit == 1 && dmf_cache.block[i].block_tag == tag){
                    arch_state.mem_stats.sw_cache_hits++; 
                    dmf_cache.block[i].data[word_offset] = (uint32_t) write_data;
                    arch_state.memory[address / 4] = (uint32_t) write_data;
                    t = 1;              
                }       
            if (t == 0) {   //miss
                arch_state.memory[address / 4] = (uint32_t) write_data;    
            }    
            break;
        case CACHE_TYPE_2_WAY: // 2-way associative
            for (int i=0; i<BLOCKS_PER_SET; i++)
                if (sa_cache.set[idx].block[i].valid_bit == 1 && sa_cache.set[idx].block[i].block_tag == tag) {
                    arch_state.mem_stats.sw_cache_hits++;
                    sa_cache.set[idx].block[i].data[word_offset] = (uint32_t) write_data;
                    arch_state.memory[address / 4] = (uint32_t) write_data;
                    t = 1;    
                }
            if (t == 0) {
                arch_state.memory[address / 4] = (uint32_t) write_data;
            }    
            break;
        }
    }
}
