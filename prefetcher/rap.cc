
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
    optgen_mytimer(RAH_CONFIG_COUNT, vector<uint64_t>( num_sets)),
    optgen_addr_history(RAH_CONFIG_COUNT),
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

void RAH::set_prefetchers(TriageBase** prefetcher_array)
{
    prefetchers.assign(prefetcher_array, prefetcher_array+NUM_CPUS);
}

uint64_t RAH::get_traffic(int core, int config) const
{
    uint64_t val = 0;
    for (int i = 0; i < num_sets; ++i) {
//        debug_cout << "Traffic set " << i << " : " << optgens[core][i][config].get_traffic() << endl;
        val += optgens[core][i][config].get_traffic();
    }
    return val;
}

uint64_t RAH::get_accesses(int core, int config) const
{
    uint64_t val = 0;
    for (int i = 0; i < num_sets; ++i) {
//        debug_cout << "Accesses set " << i << " : " << optgens[core][i][config].get_num_opt_accesses() << endl;
        val += optgens[core][i][config].get_num_opt_accesses();
    }
    return val;
}

uint64_t RAH::get_hits(int core, int config) const
{
    uint64_t val = 0;
    for (int i = 0; i < num_sets; ++i) {
//        debug_cout << "Hits set " << i << " : " << optgens[core][i][config].get_num_opt_hits() << endl;
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
        for (unsigned l = 0; l < RAH_CONFIG_COUNT; ++l) {
            if (is_prefetch && prefetchers[core]->should_skip_prefetch(rah_pref_assoc[l]))
                continue;
            uint64_t curr_quanta = optgen_mytimer[l][set_id];
            vector<bool> opt_hit(RAH_CONFIG_COUNT, false);
            if(optgen_addr_history[l].find(addr) != optgen_addr_history[l].end())
            {
                uint64_t last_quanta = optgen_addr_history[l][addr].last_quanta;
                uint64_t last_pc = optgen_addr_history[l][addr].PC;
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
            // This is the first time we are seeing this line
            else
            {
                //Initialize a new entry in the sampler
                optgen_addr_history[l][addr].init(curr_quanta);
                if (is_prefetch)
                    optgens[core][set_id][l].add_prefetch(curr_quanta);
                else
                    optgens[core][set_id][l].add_access(curr_quanta);
                //opt_hit[l] = optgens[core][set_id][l].should_cache(curr_quanta, 0, is_prefetch);
            }

            optgen_addr_history[l][addr].update(optgen_mytimer[l][set_id], pc, false);
            optgen_mytimer[l][set_id]++;
        }
    }
}

int RAH::get_best_assoc(int core)
{
    int best_config = 3, best_hit_count = 0;
    for (int config = 3; config < RAH_CONFIG_COUNT; ++config) {
        uint64_t hit_count = get_hits(core, config);
        debug_cout << "CORE " << core << " CONFIG " << config
            << " HIT_COUNT " << hit_count << endl;
        if (hit_count > best_hit_count) {
            best_config = config;
            best_hit_count = hit_count;
        }
    }
    // XXX: Only works for rah_pref_assoc => 8,8,8,8,4,0
    debug_cout << "Choose Best Config: " << best_config << endl;
    debug_cout << "RC " << best_config << " HIT_RATE: " << double(get_hits(core,3))/get_accesses(core,3) << ' ' 
            <<  double(get_hits(core,4))/get_accesses(core,4) << ' '
            <<  double(get_hits(core,5))/get_accesses(core,5) << ' ' << endl;
    assert(best_config >= 3);

    return 3;
//    return rah_pref_assoc[best_config];
}

void RAH::print_stats()
{
    cout << "RAH Triggers: " << trigger << endl;
}

