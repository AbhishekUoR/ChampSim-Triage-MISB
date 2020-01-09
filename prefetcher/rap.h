
#include <assert.h>
#include <stdint.h>
#include <iostream>
#include <vector>

#include "champsim.h"
#include "cache.h"
#include "optgen_simple.h"

#define RAH_CONFIG_COUNT 6
const int rah_config_assoc[] = {1,2,4,8,12,16};

// RAH stands for Resource Allocation Handler
class RAH
{
    uint64_t num_sets, index_mask;

    // Indexed by config_no
    std::vector<uint64_t> data_size;
    std::vector<uint64_t> metadata_size;

    // optgens[core][set_id][config]
    std::vector<uint64_t> optgen_mytimer;
    std::map<uint64_t, ADDR_INFO> optgen_addr_history;
    std::map<uint64_t, uint64_t> signatures;
    std::vector<std::vector<std::vector<OPTgen>>> optgens;

    // Stats
    int trigger = 0;
    public:
        RAH(uint64_t num_sets);
        uint64_t get_traffic(int core, int config);
        uint64_t get_hits(int core, int config);
        uint64_t get_accesses(int core, int config);
        void add_access(uint64_t addr, uint64_t pc, int core, bool is_prefetch);
        void print_stats();
};

