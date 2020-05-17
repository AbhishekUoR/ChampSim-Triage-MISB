
#include <assert.h>
#include <stdint.h>
#include <iostream>
#include <vector>

#include "champsim.h"
#include "cache.h"
#include "optgen_simple.h"
#include "triage.h"

#define RAH_CONFIG_COUNT 6
const int rah_config_assoc[] = {1,2,4,8,12,16};
const int rah_pref_assoc[] = {8,8,8,8,4,0};
//const int rah_pref_assoc[] = {8,8,8,8,8,8};

// RAH stands for Resource Allocation Handler
class RAH
{
    vector<TriageBase*> prefetchers;

    uint64_t num_sets, index_mask;

    // Indexed by config_no
    std::vector<uint64_t> data_size;
    std::vector<uint64_t> metadata_size;

    // optgen_mytimer[config][set_id]
    std::vector<std::vector<uint64_t>> optgen_mytimer;
    // optgen_addr_history[config][addr]->ADDR_INFO
    std::vector<std::map<uint64_t, ADDR_INFO>> optgen_addr_history;
    // optgens[core][set_id][config]
    std::vector<std::vector<std::vector<OPTgen>>> optgens;

    // Stats
    int trigger = 0;
    public:
        RAH(uint64_t num_sets);
        void set_prefetchers(TriageBase** prefetcher_array);
        uint64_t get_traffic(int core, int config) const;
        uint64_t get_hits(int core, int config) const;
        uint64_t get_accesses(int core, int config) const;
        void add_access(uint64_t addr, uint64_t pc, int core, bool is_prefetch);
        void print_stats();
};

