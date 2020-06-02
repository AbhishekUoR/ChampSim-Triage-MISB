
#include <assert.h>
#include <stdint.h>
#include <iostream>

#include "triage_onchip.h"

using namespace std;

//#define DEBUG

#ifdef DEBUG
#define debug_cout cerr << "[TRIAGE_REPL] "
#else
#define debug_cout if (0) cerr
#endif

//#define PRINT_TRIAGE_REPL_DETAIL

unsigned hawkeye_sample_assoc[] = {4,8};

TriageRepl::TriageRepl(std::vector<std::map<uint64_t, TriageOnchipEntry> >* entry_list)
    : entry_list(entry_list)
{
}

TriageRepl* TriageRepl::create_repl(
        std::vector<std::map<uint64_t, TriageOnchipEntry> >* entry_list,
        TriageReplType repl_type, uint64_t assoc, bool use_dynamic_assoc,
        TriageOnchip *onchip)
{
    TriageRepl *repl;
    switch (repl_type) {
        case TRIAGE_REPL_LRU:
            repl = new TriageReplLRU(entry_list);
            break;
        case TRIAGE_REPL_HAWKEYE:
            repl = new TriageReplHawkeye(entry_list, assoc, use_dynamic_assoc, onchip);
            break;
        case TRIAGE_REPL_PERFECT:
            repl = new TriageReplPerfect(entry_list);
            break;
        default:
            cerr << "Unknown repl type: " << repl_type <<endl;
            assert(0);
    }

    return repl;
}

bool TriageRepl::should_skip_prefetch(int assoc)
{
    return false;
}

TriageReplLRU::TriageReplLRU(std::vector<std::map<uint64_t, TriageOnchipEntry> >* entry_list)
    : TriageRepl(entry_list)
{
}

void TriageReplLRU::addEntry(uint64_t set_id, uint64_t addr, uint64_t pc)
{
    map<uint64_t, TriageOnchipEntry>& entry_map = (*entry_list)[set_id];
    map<uint64_t, TriageOnchipEntry>::iterator it = entry_map.find(addr);
    assert(it != entry_map.end());
    // Lower RRPV is more recently used
    for (map<uint64_t, TriageOnchipEntry>::iterator jt = entry_map.begin();
            jt != entry_map.end(); ++jt) {
        ++jt->second.rrpv;
    }
    it->second.rrpv = 0;

    debug_cout << "ReplLRU addEntry: set_id: " << hex << set_id
        << ", addr: " << addr << endl;
}

uint64_t TriageReplLRU::pickVictim(uint64_t set_id)
{
    unsigned max_ts = 0;
    map<uint64_t, TriageOnchipEntry>& entry_map = (*entry_list)[set_id];
    map<uint64_t, TriageOnchipEntry>::iterator max_it = entry_map.begin();
    for (map<uint64_t, TriageOnchipEntry>::iterator it = entry_map.begin();
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

TriageReplHawkeye::TriageReplHawkeye(std::vector<std::map<uint64_t, TriageOnchipEntry> >* entry_list,
        uint64_t assoc, bool use_dynamic_assoc, TriageOnchip * onchip)
    : TriageRepl(entry_list), on_chip_info(onchip)
{
    max_rrpv = 3;
    uint64_t num_sets = entry_list->size();
//    optgen.resize(num_sets);
    optgen_mytimer.resize(num_sets);
    cout << "Init TriageReplHawkeye, assoc: " << assoc << ", use_dynamic_assoc: " << use_dynamic_assoc
        << endl;

//    for (size_t i = 0; i < num_sets; ++i) {
//        optgen[i].init(assoc-2);
//    }

    last_access_count = curr_access_count = 0;
    this->assoc = assoc;
    if (assoc==4) {
        dynamic_optgen_choice = 0;
    } else if (assoc ==8) {
        dynamic_optgen_choice = 1;
    } else {
        dynamic_optgen_choice = 0;
    }
    use_dynamic = use_dynamic_assoc;
    sample_optgen.resize(HAWKEYE_SAMPLE_ASSOC_COUNT);
    if (!use_dynamic)
        hawkeye_sample_assoc[0] = assoc;
    for (unsigned l = 0; l < HAWKEYE_SAMPLE_ASSOC_COUNT; ++l) {
        sample_optgen[l].resize(num_sets);
        for (size_t i = 0; i < num_sets; ++i) {
            sample_optgen[l][i].init(hawkeye_sample_assoc[l]-2);
        }
    }
}

#include <math.h>
#define bitmask(l) (((l) == 64) ? (unsigned long long)(-1LL) : ((1LL << (l))-1LL))
#define bits(x, i, l) (((x) >> (i)) & bitmask(l))
#define SAMPLED_SET(set) true

void TriageReplHawkeye::addEntry(uint64_t set_id, uint64_t addr, uint64_t pc)
{
    if (!use_dynamic) {
        for (OPTgen& optgen : sample_optgen[0]) {
            optgen.CACHE_SIZE = on_chip_info->get_assoc()>2?on_chip_info->get_assoc()-2:1;
        }
    }
    debug_cout << hex << "Hawkeye addEntry: set_id: " << set_id << ", addr: " << addr << ", pc: " << pc << endl;
    map<uint64_t, TriageOnchipEntry>& entry_map = (*entry_list)[set_id];
    if (use_dynamic) {
        if (curr_access_count - last_access_count > HAWKEYE_EPOCH_LENGTH) {
            choose_optgen();
            last_access_count = curr_access_count;
        } else {
            ++curr_access_count;
        }
    }
    debug_cout << "hawkeye optgen choice: " << dynamic_optgen_choice << endl;
    if(SAMPLED_SET(set_id))
    {
        uint64_t curr_quanta = optgen_mytimer[set_id];
        bool opt_hit[] = {false, false};

        signatures[addr] = pc;
        if(optgen_addr_history[set_id].find(addr) != optgen_addr_history[set_id].end())
        {
            uint64_t last_quanta = optgen_addr_history[set_id][addr].last_quanta;
            assert(curr_quanta >= last_quanta);

            uint64_t last_pc = optgen_addr_history[set_id][addr].PC;
            for (unsigned l = 0; l < HAWKEYE_SAMPLE_ASSOC_COUNT; ++l) {
                debug_cout << l << " SHOULD CACHE ADDR: " << hex << addr << ", opt_hit: " << dec << opt_hit[l]
                    << ", curr_quanta: " << curr_quanta << ", last_quanta: " << last_quanta
                    << endl;
                assert(curr_quanta > last_quanta);
                opt_hit[l] = sample_optgen[l][set_id].should_cache(curr_quanta, last_quanta, false);
                last_hit[l] = opt_hit[l];
                sample_optgen[l][set_id].add_access(curr_quanta);
                debug_cout << l << " SHOULD CACHE ADDR: " << hex << addr << ", opt_hit: " << dec << opt_hit[l]
                    << ", curr_quanta: " << curr_quanta << ", last_quanta: " << last_quanta
                    << endl;
            }
            if (dynamic_optgen_choice != 2) {
                if (opt_hit[dynamic_optgen_choice]) {
                    predictor.increment(last_pc);
                } else {
                    //predictor.decrement(last_pc|(bits(set_id,6,13)<<24));
                    predictor.decrement(last_pc);
                }
                debug_cout <<  "Train: " << hex << last_pc << " " << dec << opt_hit[dynamic_optgen_choice] << endl;
            }
            //Some maintenance operations for OPTgen
            //optgen[set_id].add_access(curr_quanta, 0); //TODO: CPU
        }
        // This is the first time we are seeing this line
        else
        {
            //Initialize a new entry in the sampler
            optgen_addr_history[set_id][addr].init(curr_quanta);
            debug_cout << " INITNEWENTRY: setid: " << set_id << "tag: " << addr
                << ", curr_quanta: " << curr_quanta << endl;

            //optgen[set_id].add_access(curr_quanta, 0); //TODO: CPU
            for (unsigned l = 0; l < HAWKEYE_SAMPLE_ASSOC_COUNT; ++l) {
                sample_optgen[l][set_id].add_access(curr_quanta);
                last_hit[l] = false;
            }
        }

        optgen_addr_history[set_id][addr].update(optgen_mytimer[set_id], pc, false);
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
        for(map<uint64_t, TriageOnchipEntry>::iterator it = entry_map.begin();
                it != entry_map.end(); ++it) {
            if (it->second.rrpv >= max_rrpv-1)
                saturated = true;
        }
        //Age all the cache-friendly  lines
        for(map<uint64_t, TriageOnchipEntry>::iterator it = entry_map.begin();
                it != entry_map.end(); ++it) {
            if (!saturated && it->second.rrpv < max_rrpv-1)
                ++it->second.rrpv;
        }
        if (entry_map.count(addr)) {
            entry_map[addr].rrpv = 0;
        }
    } else {
        if (entry_map.count(addr)) {
            entry_map[addr].rrpv = max_rrpv;
        }
    }
    debug_cout << "AddEntry after Entry Map size: " << entry_map.size() << endl;
}

uint64_t TriageReplHawkeye::pickVictim(uint64_t set_id)
{
    if (!use_dynamic) {
        for (OPTgen& optgen : sample_optgen[0]) {
            optgen.CACHE_SIZE = on_chip_info->get_assoc()>2?on_chip_info->get_assoc()-2:1;
        }
    }
    map<uint64_t, TriageOnchipEntry>& entry_map = (*entry_list)[set_id];
    debug_cout << "PickVictim before Entry Map size: " << entry_map.size() << endl;
    for(map<uint64_t, TriageOnchipEntry>::iterator it = entry_map.begin();
            it != entry_map.end(); ++it) {
        debug_cout << "entry_map[" << it->first << "] = " << it->second.rrpv
            << endl;
    }
    for(map<uint64_t, TriageOnchipEntry>::iterator it = entry_map.begin(); it != entry_map.end(); ++it) {
        if (it->second.rrpv == max_rrpv) {
            uint64_t addr = it->first;
            return addr;
        }
    }

    //If we cannot find a cache-averse line, we evict the oldest cache-friendly line
    uint32_t max_rrip = 0;
    uint64_t lru_victim = 0;
    for(map<uint64_t, TriageOnchipEntry>::iterator it = entry_map.begin();
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

#define HAWKEYE_OPTGEN_THRESHOLD 0.05
#define HAWKEYE_OPTGEN_THRESHOLD2 0.05
void TriageReplHawkeye::choose_optgen()
{
    double hit_rate[HAWKEYE_SAMPLE_ASSOC_COUNT];
    for (unsigned l = 0; l < HAWKEYE_SAMPLE_ASSOC_COUNT; ++l) {
        uint64_t access = 0, hits = 0;
        for (size_t i = 0; i < sample_optgen[0].size(); ++i) {
            debug_cout << "sampleoptgen[" << l << "][" << i << "].access = " << sample_optgen[l][i].access
                << ", hits = " << sample_optgen[l][i].get_num_opt_hits() << endl;

            access += sample_optgen[l][i].access;
            hits +=  sample_optgen[l][i].get_num_opt_hits();
        }
        hit_rate[l] = double(hits) / double(access);
    }
    // XXX Ad-hoc way of choosing the optgen
    if (hit_rate[1] > HAWKEYE_OPTGEN_THRESHOLD+HAWKEYE_OPTGEN_THRESHOLD2 && hit_rate[1] > hit_rate[0] + HAWKEYE_OPTGEN_THRESHOLD2) {
        dynamic_optgen_choice = 1;
    } else if (hit_rate[0] > HAWKEYE_OPTGEN_THRESHOLD) {
        dynamic_optgen_choice = 0;
    } else {
        dynamic_optgen_choice = 2;
    }
#ifdef PRINT_TRIAGE_REPL_DETAIL
    cout << "hit_rate[0]: " << hit_rate[0] << ", hit_rate[1]: " << hit_rate[1] << ", dynamic_optgen_choice: "
        << dynamic_optgen_choice << endl;
#endif
}

uint32_t TriageReplHawkeye::get_assoc()
{
    if (use_dynamic) {
        switch (dynamic_optgen_choice) {
            case 0:
                return 4;
            case 1:
                return 8;
            case 2:
                return 0;
            default:
                // This can't happen
                assert(0);
        }
    } else {
        return assoc;
    }
}

bool TriageReplHawkeye::should_skip_prefetch(int assoc)
{
    int config = 0;
    switch (assoc) {
        case 8:
            config = 1;
            break;
        case 4:
            config = 0;
            break;
        case 0:
            config = 2;
            break;
        default:
            cout << "We only support assoc 0,4,8 for now." << endl;
            assert(0);
    }
    if (config == 2) {
        // No assoc, skip all prefetches
        return true;
    }
    return last_hit[config];
    
    return false;
}

void TriageReplHawkeye::print_stats()
{
    unsigned int hits = 0, access = 0, traffic = 0;
    /*
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
    */
    for (unsigned l = 0; l < HAWKEYE_SAMPLE_ASSOC_COUNT; ++l) {
        access = 0, hits = 0, traffic = 0;
        for (size_t i = 0; i < sample_optgen[l].size(); ++i) {
            access += sample_optgen[l][i].access;
            hits +=  sample_optgen[l][i].get_num_opt_hits();
            traffic +=  sample_optgen[l][i].get_traffic();
        }
        std::cout << "SAMPLEOPTGEN " << l << " accesses: " << access
            << ", hits: " << hits
            << ", traffic: " << traffic
            << ", hit rate: " << double(hits) / double(access)
            << ", traffic rate: " << double(traffic) / double(access)
            << std::endl;
    }
}

TriageReplPerfect::TriageReplPerfect(std::vector<std::map<uint64_t, TriageOnchipEntry> >* entry_list)
    : TriageRepl(entry_list)
{
}

void TriageReplPerfect::addEntry(uint64_t set_id, uint64_t addr, uint64_t pc)
{
    // Don't do anything
}

uint64_t TriageReplPerfect::pickVictim(uint64_t set_id)
{
    // Don't do anything
    return 0;
}

