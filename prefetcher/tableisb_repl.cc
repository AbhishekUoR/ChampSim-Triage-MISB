
#include <assert.h>
#include <stdint.h>
#include <iostream>

#include "tableisb_onchip.h"

using namespace std;

//#define DEBUG

#ifdef DEBUG
#define debug_cout cerr << "[TABLEISB_REPL] "
#else
#define debug_cout if (0) cerr
#endif


TableISBRepl::TableISBRepl(std::vector<std::map<uint64_t, TableISBOnchipEntry> >* entry_list)
    : entry_list(entry_list)
{
}

TableISBRepl* TableISBRepl::create_repl(
        std::vector<std::map<uint64_t, TableISBOnchipEntry> >* entry_list,
        TableISBReplType repl_type, uint64_t assoc)
{
    TableISBRepl *repl;
    switch (repl_type) {
        case TABLEISB_REPL_LRU:
            repl = new TableISBReplLRU(entry_list);
            break;
        case TABLEISB_REPL_HAWKEYE:
            repl = new TableISBReplHawkeye(entry_list, assoc);
            break;
        case TABLEISB_REPL_PERFECT:
            repl = new TableISBReplPerfect(entry_list);
            break;
        default:
            cerr << "Unknown repl type: " << repl_type <<endl;
            assert(0);
    }

    return repl;
}

TableISBReplLRU::TableISBReplLRU(std::vector<std::map<uint64_t, TableISBOnchipEntry> >* entry_list)
    : TableISBRepl(entry_list)
{
}

void TableISBReplLRU::addEntry(uint64_t set_id, uint64_t addr, uint64_t pc)
{
    map<uint64_t, TableISBOnchipEntry>& entry_map = (*entry_list)[set_id];
    map<uint64_t, TableISBOnchipEntry>::iterator it = entry_map.find(addr);
    assert(it != entry_map.end());
    // Lower RRPV is more recently used
    for (map<uint64_t, TableISBOnchipEntry>::iterator jt = entry_map.begin();
            jt != entry_map.end(); ++jt) {
        ++jt->second.rrpv;
    }
    it->second.rrpv = 0;

    debug_cout << "ReplLRU addEntry: set_id: " << hex << set_id
        << ", addr: " << addr << endl;
}

uint64_t TableISBReplLRU::pickVictim(uint64_t set_id)
{
    unsigned max_ts = 0;
    map<uint64_t, TableISBOnchipEntry>& entry_map = (*entry_list)[set_id];
    map<uint64_t, TableISBOnchipEntry>::iterator max_it = entry_map.begin();
    for (map<uint64_t, TableISBOnchipEntry>::iterator it = entry_map.begin();
            it != entry_map.end(); ++it) {
        if (it->second.rrpv > max_ts) {
            max_ts = it->second.rrpv;
            max_it = it;
        }
    }

    assert(max_it != entry_map.end());
    uint64_t addr = max_it->first;

    debug_cout << "ReplLRU pickVictim: set_id: " << hex << set_id
        << ", victim: " << addr << ", rrpv: " << max_ts << endl;
    return addr;
}

TableISBReplHawkeye::TableISBReplHawkeye(std::vector<std::map<uint64_t, TableISBOnchipEntry> >* entry_list,
        uint64_t assoc)
    : TableISBRepl(entry_list)
{
    max_rrpv = 3;
    uint64_t num_sets = entry_list->size();
    optgen.resize(num_sets);
    optgen_mytimer.resize(num_sets);
    optgen_addr_history.clear();

    for (size_t i = 0; i < num_sets; ++i) {
        optgen[i].init(assoc-2);
    }
}

#include <math.h>
#define bitmask(l) (((l) == 64) ? (unsigned long long)(-1LL) : ((1LL << (l))-1LL))
#define bits(x, i, l) (((x) >> (i)) & bitmask(l))
#define SAMPLED_SET(set) true

void TableISBReplHawkeye::addEntry(uint64_t set_id, uint64_t addr, uint64_t pc)
{
    map<uint64_t, TableISBOnchipEntry>& entry_map = (*entry_list)[set_id];
    if(SAMPLED_SET(set_id))
    {
        uint64_t curr_quanta = optgen_mytimer[set_id];
        bool opt_hit = false;

        signatures[addr] = pc;
        if(optgen_addr_history.find(addr) != optgen_addr_history.end())
        {
            uint64_t last_quanta = optgen_addr_history[addr].last_quanta;
            //    cout << last_quanta << " " << curr_quanta << endl;
            assert(curr_quanta >= optgen_addr_history[addr].last_quanta);
            uint64_t last_pc = optgen_addr_history[addr].PC;
            opt_hit = optgen[set_id].should_cache(curr_quanta, last_quanta, false, 0); //TODO: CPU
            if (opt_hit) {
                predictor.increment(last_pc);
            } else {
                //predictor.decrement(last_pc|(bits(set_id,6,13)<<24));
                predictor.decrement(last_pc);
            }
            debug_cout <<  "Train: " << hex << last_pc << " " << dec << opt_hit << endl;
            //Some maintenance operations for OPTgen
            optgen[set_id].add_access(curr_quanta, 0); //TODO: CPU
        }
        // This is the first time we are seeing this line
        else
        {
            //Initialize a new entry in the sampler
            optgen_addr_history[addr].init(curr_quanta);
            optgen[set_id].add_access(curr_quanta, 0); //TODO: CPU
        }

        optgen_addr_history[addr].update(optgen_mytimer[set_id], pc, false);
        optgen_mytimer[set_id]++;
    }

    bool prediction = predictor.get_prediction(pc);

    debug_cout <<  "Predict: " << hex << pc << " " << dec << prediction << endl;
    if(hawkeye_pc_ps_total_predictions.find(pc) == hawkeye_pc_ps_total_predictions.end())
    {
        hawkeye_pc_ps_total_predictions[pc] = 0;
        hawkeye_pc_ps_hit_predictions[pc] = 0;
    }
    hawkeye_pc_ps_total_predictions[pc]++;
    if (prediction) {
        hawkeye_pc_ps_hit_predictions[pc]++;
        bool saturated = false;
        for(map<uint64_t, TableISBOnchipEntry>::iterator it = entry_map.begin();
                it != entry_map.end(); ++it) {
            if (it->second.rrpv >= max_rrpv-1)
                saturated = true;
        }
        //Age all the cache-friendly  lines
        for(map<uint64_t, TableISBOnchipEntry>::iterator it = entry_map.begin();
                it != entry_map.end(); ++it) {
            if (!saturated && it->second.rrpv < max_rrpv-1)
                ++it->second.rrpv;
        }
        entry_map[addr].rrpv = 0;
    } else {
        entry_map[addr].rrpv = max_rrpv;
    }
    debug_cout << "AddEntry after Entry Map size: " << entry_map.size() << endl;
}

uint64_t TableISBReplHawkeye::pickVictim(uint64_t set_id)
{
    map<uint64_t, TableISBOnchipEntry>& entry_map = (*entry_list)[set_id];
    debug_cout << "PickVictim before Entry Map size: " << entry_map.size() << endl;
    for(map<uint64_t, TableISBOnchipEntry>::iterator it = entry_map.begin();
            it != entry_map.end(); ++it) {
        debug_cout << "entry_map[" << it->first << "] = " << it->second.rrpv
            << endl;
    }
    for(map<uint64_t, TableISBOnchipEntry>::iterator it = entry_map.begin(); it != entry_map.end(); ++it) {
        if (it->second.rrpv == max_rrpv) {
            uint64_t addr = it->first;
            return addr;
        }
    }

    //If we cannot find a cache-averse line, we evict the oldest cache-friendly line
    uint32_t max_rrip = 0;
    uint64_t lru_victim = 0;
    for(map<uint64_t, TableISBOnchipEntry>::iterator it = entry_map.begin();
            it != entry_map.end(); ++it) {
        if (it->second.rrpv >= max_rrip)
        {
            max_rrip = it->second.rrpv;
            lru_victim = it->first;
        }
    }
    assert(entry_map.count(lru_victim));

    //The predictor is trained negatively on LRU evictions
    if( SAMPLED_SET(set) ) {
        debug_cout << "Detrain: " << hex << signatures[lru_victim] << dec<< endl;
        predictor.decrement(signatures[lru_victim]);
    }
    debug_cout << "PickVictim after Entry List size: " << entry_map.size() << endl;
    return lru_victim;
}

void TableISBReplHawkeye::print_stats()
{
    unsigned int hits = 0, access = 0, traffic = 0;
    for(unsigned int i=0; i<optgen.size(); i++)
    {
        access += optgen[i].access;
        hits += optgen[i].get_num_opt_hits();
        traffic += optgen[i].get_traffic();
    }

    std::cout << "OPTgen accesses: " << access
        << ", hits: " << hits
        << ", traffic: " << traffic
        << ", hit rate: " << double(hits) / double(access)
        << ", traffic rate: " << double(traffic) / double(access)
        << std::endl;
}

TableISBReplPerfect::TableISBReplPerfect(std::vector<std::map<uint64_t, TableISBOnchipEntry> >* entry_list)
    : TableISBRepl(entry_list)
{
}

void TableISBReplPerfect::addEntry(uint64_t set_id, uint64_t addr, uint64_t pc)
{
    // Don't do anything
}

uint64_t TableISBReplPerfect::pickVictim(uint64_t set_id)
{
    // Don't do anything
    return 0;
}

