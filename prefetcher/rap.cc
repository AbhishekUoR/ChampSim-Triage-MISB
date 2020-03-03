
#include "rap.h"

//#define DEBUG

#ifdef DEBUG
#define debug_cout cerr << "[RAH] "
#else
#define debug_cout if (0) cerr
#endif
#define SAMPLED_SET(set) true

using namespace std;

RAH::RAH(uint64_t num_sets) :
    num_sets(num_sets),
    index_mask(num_sets-1),
    data_size(RAH_CONFIG_COUNT),
    metadata_size(RAH_CONFIG_COUNT),
    optgen_mytimer(num_sets),
    optgens(NUM_CPUS, vector<vector<OPTgen>>(num_sets, vector<OPTgen>(RAH_CONFIG_COUNT))),
    trigger(0)
{
    for (int core = 0; core < NUM_CPUS; ++core) {
        for (int set = 0; set < num_sets; ++set) {
            for (int config = 0; config < RAH_CONFIG_COUNT; ++config) {
                optgens[core][set][config].init(rah_config_assoc[config]-2);
            }
        }
    }
}

uint64_t RAH::get_traffic(int core, int config)
{
    uint64_t val = 0;
    for (int i = 0; i < num_sets; ++i) {
        debug_cout << "Traffic set " << i << " : " << optgens[core][i][config].get_traffic() << endl;
        val += optgens[core][i][config].get_traffic();
    }
    return val;
}

uint64_t RAH::get_accesses(int core, int config)
{
    uint64_t val = 0;
    for (int i = 0; i < num_sets; ++i) {
        debug_cout << "Accesses set " << i << " : " << optgens[core][i][config].get_num_opt_accesses() << endl;
        val += optgens[core][i][config].get_num_opt_accesses();
    }
    return val;
}

uint64_t RAH::get_hits(int core, int config)
{
    uint64_t val = 0;
    for (int i = 0; i < num_sets; ++i) {
        debug_cout << "Hits set " << i << " : " << optgens[core][i][config].get_num_opt_hits() << endl;
        val += optgens[core][i][config].get_num_opt_hits();
    }
    return val;
}

void RAH::add_access(uint64_t addr, uint64_t pc, int core, bool is_prefetch)
{
    uint64_t set_id = (addr>>6)&index_mask;
    ++trigger;
    debug_cout << "add_access: " << (void*) addr
        << ", " << (void*) pc
        << ", " << core
        << ", " << is_prefetch
        << ", " << set_id
        << endl;
    if(SAMPLED_SET(set_id))
    {
        uint64_t curr_quanta = optgen_mytimer[set_id];
        vector<bool> opt_hit(RAH_CONFIG_COUNT, false);
        signatures[addr] = pc;
        if(optgen_addr_history.find(addr) != optgen_addr_history.end())
        {
            uint64_t last_quanta = optgen_addr_history[addr].last_quanta;
            assert(curr_quanta >= last_quanta);
            uint64_t last_pc = optgen_addr_history[addr].PC;
            for (unsigned l = 0; l < RAH_CONFIG_COUNT; ++l) {
                assert(curr_quanta > last_quanta);
                opt_hit[l] = optgens[core][set_id][l].should_cache(curr_quanta, last_quanta, is_prefetch);
                if (is_prefetch)
                    optgens[core][set_id][l].add_prefetch(curr_quanta);
                else
                    optgens[core][set_id][l].add_access(curr_quanta);
                debug_cout << l << " SHOULD CACHE ADDR: " << hex << addr << ", opt_hit: " << dec << opt_hit[l]
                    << ", curr_quanta: " << curr_quanta << ", last_quanta: " << last_quanta
                    << endl;
            }
        }
        // This is the first time we are seeing this line
        else
        {
            //Initialize a new entry in the sampler
            optgen_addr_history[addr].init(curr_quanta);
            for (unsigned l = 0; l < RAH_CONFIG_COUNT; ++l) {
                if (is_prefetch)
                    optgens[core][set_id][l].add_prefetch(curr_quanta);
                else
                    optgens[core][set_id][l].add_access(curr_quanta);
                //opt_hit[l] = optgens[core][set_id][l].should_cache(curr_quanta, 0, is_prefetch);
            }
        }

        optgen_addr_history[addr].update(optgen_mytimer[set_id], pc, false);
        optgen_mytimer[set_id]++;
    }
}

void RAH::print_stats()
{
    cout << "RAH Triggers: " << trigger << endl;
}

