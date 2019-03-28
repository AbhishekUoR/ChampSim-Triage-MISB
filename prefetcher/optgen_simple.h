//Hawkeye Cache Replacement Tool v2.0
//UT AUSTIN RESEARCH LICENSE (SOURCE CODE)
//The University of Texas at Austin has developed certain software and documentation that it desires to
//make available without charge to anyone for academic, research, experimental or personal use.
//This license is designed to guarantee freedom to use the software for these purposes. If you wish to
//distribute or make other use of the software, you may purchase a license to do so from the University of
//Texas.
///////////////////////////////////////////////
//                                            //
//     Hawkeye [Jain and Lin, ISCA' 16]       //
//     Akanksha Jain, akanksha@cs.utexas.edu  //
//                                            //
///////////////////////////////////////////////


#ifndef OPTGEN_H
#define OPTGEN_H

using namespace std;

#include <iostream>

#include <math.h>
#include <set>
#include <vector>
#include "champsim.h"
#define EPOCH_LENGTH 100000000
struct _EPOCH
{
    uint64_t last_epoch_cycle[NUM_CPUS];
    uint64_t supply[NUM_CPUS];
    uint64_t supply_count[NUM_CPUS];
    uint64_t demand[NUM_CPUS];
    uint64_t demand_count[NUM_CPUS];
    uint64_t supply_total, demand_total;

    void reset()
    {
        for(unsigned int i=0; i<NUM_CPUS; i++)
        {
            last_epoch_cycle[i] = current_core_cycle[i];
            supply[i] = 0;
            demand[i] = 0;
            supply_count[i] = 0;
            demand_count[i] = 0;
        }
        supply_total = 0;
        demand_total = 0;
    }

    bool is_complete()
    {
        bool complete = true;

        for(unsigned int i=0; i<NUM_CPUS; i++)
        {
            uint64_t cycles_elapsed = (current_core_cycle[i] - last_epoch_cycle[i]);
            if(cycles_elapsed < EPOCH_LENGTH)
            {
                complete = false;
                break;
            }
        }

        return complete;
    }

    void update_supply(int rd, uint32_t cpu)
    {
        assert(rd >= 0);
        supply_total += rd;
        supply[cpu] += rd;
        supply_count[cpu]++;
    }

    void update_demand(int rd, uint32_t cpu)
    {
        assert(rd >= 0);
        demand_total += rd;
        demand[cpu] += rd;
        demand_count[cpu]++;
    }


    void new_epoch(uint64_t* threshold)
    {
        uint64_t supply_rd[NUM_CPUS] = {0};
        uint64_t demand_rd[NUM_CPUS] = {0};
        double ratio[NUM_CPUS] = {0.0};
        for(unsigned int i=0; i<NUM_CPUS; i++)
        {
            if(supply_count[i] != 0)
                supply_rd[i] = (double)supply[i]/(double)supply_count[i];
            if(demand_count[i] != 0)
                demand_rd[i] = (double)demand[i]/(double)demand_count[i];

            if(supply_rd[i] != 0)
                ratio[i] = (double)demand_rd[i]/(double)supply_rd[i];
        }

        for(unsigned int i=0; i<NUM_CPUS; i++)
            threshold[i] = (uint64_t)(5*NUM_CPUS*(ratio[i]/2));
        //cout << "Demand/Supply: " << demand_total << " " << supply_total << endl;
        //cout << ratio[0] << " " << threshold[0] << endl;
    }
};


struct ADDR_INFO
{
    uint64_t addr;
    uint32_t last_quanta;
    uint64_t PC; 
    bool prefetched;
    uint32_t lru;
    bool last_prediction;
    bool is_high_cost_predicted; // for Obol
    uint64_t last_miss_cost;
    uint32_t index;
    vector<uint64_t> context;
    bool written;
    bool dp;
    bool pp;
    bool scan;

    void init(unsigned int curr_quanta, bool is_next_high_cost = false)
    {
        last_quanta = 0;
        PC = 0;
        prefetched = false;
        lru = 0;
        is_high_cost_predicted = is_next_high_cost;
        last_miss_cost = 0;
        index = 0;
        context.clear();
        written = false;
        dp = false;
        pp = false;
        scan = false;
    }

    void update(unsigned int curr_quanta, uint64_t _pc, bool prediction, bool is_next_high_cost = false)
    {
        last_quanta = curr_quanta;
        PC = _pc;
        last_prediction = prediction;
        is_high_cost_predicted = is_next_high_cost;
        scan = false;
    }

    void mark_prefetch()
    {
        prefetched = true;
    }

    void set_dp()
    {
        pp= false;
        dp = true;
    }

    void set_pp()
    {
        pp = true;
        dp = false;
    }
};

struct OPTgen
{
    vector<unsigned int> access_history;
    vector<unsigned int> liveness_history;
    vector<vector<unsigned int> > per_core_liveness_history;
    vector<unsigned int> liveness_history_stable;

    uint64_t num_cache;
    uint64_t num_dont_cache;
    uint64_t access;
    uint64_t prefetch_count;
    uint64_t prefetch_cachehit;
    uint64_t per_core_num_cache[NUM_CPUS];
    uint64_t per_core_access[NUM_CPUS];
    uint64_t per_core_prefetches[NUM_CPUS];
    uint64_t per_core_prefetch_cachehit[NUM_CPUS];

    uint64_t CACHE_SIZE;

    void init(uint64_t size)
    {
        num_cache = 0;
        num_dont_cache = 0;
        access = 0;
        CACHE_SIZE = size;
        prefetch_count = 0;
        prefetch_cachehit = 0;
        for(unsigned int i=0; i<NUM_CPUS; i++) {
            per_core_access[i] = 0;
            per_core_prefetches[i] = 0;
            per_core_num_cache[i] = 0;
            per_core_prefetch_cachehit[i] = 0;
        }

        per_core_liveness_history.resize(NUM_CPUS);
    }

    void add_access(uint64_t curr_quanta, uint64_t last_quanta, uint32_t cpu)
    {
        assert(last_quanta < access_history.size());
        assert(access_history[last_quanta] > 0);
        access_history[last_quanta]--;

        add_access(curr_quanta, cpu);
    }

    void add_prefetch(uint64_t curr_quanta, uint64_t last_quanta, uint32_t cpu)
    {
        assert(last_quanta < access_history.size());
        assert(access_history[last_quanta] > 0);
        access_history[last_quanta]--;

        add_prefetch(curr_quanta, cpu);
    }


    void add_access(uint64_t curr_quanta, uint32_t cpu)
    {
        access++;
        per_core_access[cpu]++;
        liveness_history.resize(curr_quanta+1);
        access_history.resize(curr_quanta+1);
        per_core_liveness_history[cpu].resize(curr_quanta+1);
        liveness_history_stable.resize(curr_quanta+1);
        // TODO: initialize liveness with 0 or 1?
        liveness_history[curr_quanta] = 0;
        per_core_liveness_history[cpu][curr_quanta] = 0;
        liveness_history_stable[curr_quanta] = 0;
        access_history[curr_quanta] = 1;
    }

    void add_prefetch(uint64_t curr_quanta, uint32_t cpu)
    {
        per_core_prefetches[cpu]++;
        liveness_history.resize(curr_quanta+1);
        per_core_liveness_history[cpu].resize(curr_quanta+1);
        liveness_history_stable.resize(curr_quanta+1);
        prefetch_count++;
        liveness_history[curr_quanta] = 0;
        per_core_liveness_history[cpu][curr_quanta] = 0;
        liveness_history_stable[curr_quanta] = 0;
        access_history.resize(curr_quanta+1);
        access_history[curr_quanta] = 1;
    }

    bool should_cache(uint64_t curr_quanta, uint64_t last_quanta, bool prefetch, uint32_t cpu)
    {
        bool is_cache = true;

        if(per_core_liveness_history[cpu].size() <= curr_quanta)
           per_core_liveness_history[cpu].resize(curr_quanta+1);

        assert(curr_quanta <= liveness_history.size());
        assert(curr_quanta <= per_core_liveness_history[cpu].size());
        unsigned int i = last_quanta;
        while (i != curr_quanta)
        {
            if(liveness_history[i] >= CACHE_SIZE)
            {
                is_cache = false;
                break;
            }

            i++;
        }

        //if ((is_cache) && (last_quanta != curr_quanta))
        if ((is_cache))
        {
            i = last_quanta;
            while (i != curr_quanta)
            {
                liveness_history[i]++;
                per_core_liveness_history[cpu][i]++;
                liveness_history_stable[i]++;
                i++;
            }
            assert(i == curr_quanta);
        }

        if(!prefetch)
        {
            if (is_cache) {
                num_cache++;
                per_core_num_cache[cpu]++;
            }
            else num_dont_cache++;
        }
        else
        {
            if(is_cache) {
                prefetch_cachehit++;
                per_core_prefetch_cachehit[cpu]++;
            }
        }

        return is_cache;    
    }

    bool should_cache_probe(uint64_t curr_quanta, uint64_t last_quanta)
    {
        bool is_cache = true;

        unsigned int i = last_quanta;
        while (i != curr_quanta)
        {
            if(liveness_history[i] >= CACHE_SIZE)
            {
                is_cache = false;
                break;
            }

            i++;
        }

        return is_cache;    
    }

    bool should_cache_tentative(uint64_t curr_quanta, uint64_t last_quanta)
    {
        bool is_cache = true;

        unsigned int i = last_quanta;
        while (i != curr_quanta)
        {
            if(liveness_history[i] >= CACHE_SIZE)
            {
                is_cache = false;
                break;
            }

            i++;
        }

        //if ((is_cache) && (last_quanta != curr_quanta))
        if ((is_cache))
        {
            i = last_quanta;
            while (i != curr_quanta)
            {
                liveness_history[i]++;
                i++;
            }
            assert(i == curr_quanta);
        }

        return is_cache;    
    }

    void revert_to_checkpoint()
    {
        assert(liveness_history.size() == liveness_history_stable.size());
        for(unsigned int i=0; i < liveness_history.size(); i++)
            liveness_history[i] = liveness_history_stable[i];
    }

    uint64_t get_num_opt_accesses()
    {
        //assert((num_cache+num_dont_cache) == access);
        return access;
    }

    uint64_t get_num_opt_hits()
    {
        return num_cache;

        uint64_t num_opt_misses = access - num_cache;
        return num_opt_misses;
    }

    uint64_t get_traffic()
    {
        return (prefetch_count - prefetch_cachehit + access - num_cache);
    }

    uint64_t get_traffic(uint32_t cpu)
    {
        return (per_core_prefetches[cpu] - per_core_prefetch_cachehit[cpu] + per_core_access[cpu] - per_core_num_cache[cpu]);
    }

    double get_num_opt_hits(unsigned int core_id)
    {
        return per_core_num_cache[core_id];
    }


    void print()
    {
        cout << "Liveness Info " << liveness_history.size() << endl;        
        for(unsigned int i=0 ; i < liveness_history.size(); i++)
            cout << liveness_history[i] << ", ";

        cout << endl << endl;

        cout << "Liveness Stable Info " << liveness_history_stable.size() << endl;        
        for(unsigned int i=0 ; i < liveness_history_stable.size(); i++)
            cout << liveness_history_stable[i] << ", ";

        cout << endl << endl;
    }

    void print_per_core()
    {
        for(unsigned int k=0; k<NUM_CPUS; k++)
        {
            cout << "Liveness Info CPU " << k << ": " << per_core_liveness_history[k].size() << endl;        
            for(unsigned int i=0 ; i < per_core_liveness_history[k].size(); i++)
                cout << per_core_liveness_history[k][i] << ", ";

            cout << endl << endl;
        }

    }

    bool is_full(uint32_t index)
    {
        if(index >= liveness_history.size())
            return false;

        assert(liveness_history[index] <= CACHE_SIZE);
        return (liveness_history[index] == CACHE_SIZE);
    }

    vector<unsigned int> get_liveness_vector()
    {
        return liveness_history;
    }

    vector<unsigned int> get_liveness_vector(uint32_t cpu)
    {
        return per_core_liveness_history[cpu];
    }
};

#endif
