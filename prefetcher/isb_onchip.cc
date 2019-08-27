
#include <cassert>
#include <climits>

#include <iostream>

#include "isb_onchip.h"
#include "isb.h"
//#include "optgen_simple.h"

using namespace std;

//#define OVERWRITE_OFF_CHIP

//#define DEBUG

#ifdef DEBUG
#define debug_cout std::cerr << "[ISBONCHIP] "
#else
#define debug_cout if (0) std::cerr
#endif

std::ostream* outf = &std::cerr;

//#define OPTGEN_BULK
#define PS_LINE_ID(phy_addr) (phy_addr >> 6 >> PS_METADATA_LINE_SHIFT)
#define PS_LINE_OFFSET(phy_addr) ((phy_addr >> 6) % PS_METADATA_LINE_SIZE)
#define PS_SET_ID(phy_addr) ((phy_addr >> 6 >> PS_METADATA_LINE_SHIFT) & ps_indexMask)

#define SP_LINE_OFFSET(str_addr) (str_addr % SP_METADATA_LINE_SIZE)
#define SP_LINE_ID(str_addr) (str_addr >> SP_METADATA_LINE_SHIFT)
#define SP_SET_ID(str_addr) ((str_addr >> SP_METADATA_LINE_SHIFT) & sp_indexMask)

#include <math.h>
#define bitmask(l) (((l) == 64) ? (unsigned long long)(-1LL) : ((1LL << (l))-1LL))
#define bits(x, i, l) (((x) >> (i)) & bitmask(l))
//#define SAMPLED_SET(set) (bits(set, 0 , 7) == bits(set, ((unsigned long long)log2(num_sets) - 7), 7) )
#define SAMPLED_SET(set) true

OnChipInfo::OnChipInfo(const pf_isb_conf_t *p, OffChipInfo* off_chip_info, IsbPrefetcher* pref)
{
    this->off_chip_info = off_chip_info;
    this->pref = pref;
    off_chip_latency = p->isb_off_chip_latency;
    ideal_off_chip_transaction = p->isb_off_chip_ideal;
    off_chip_writeback = p->isb_off_chip_writeback;
    count_off_chip_write_traffic = p->count_off_chip_write_traffic;
    filler_count = p->isb_off_chip_fillers;
    oci_pref.init(off_chip_info, p->amc_metapref_degree);
#ifdef OVERWRITE_OFF_CHIP
    cout << "OVERWRITE_OFF_CHIP ON" << endl;;
#else
    cout << "OVERWRITE_OFF_CHIP OFF" << endl;;
#endif
}

void OnChipInfo::set_conf(const pf_isb_conf_t *p, IsbPrefetcher *pf)
{
    check_bandwidth = p->check_bandwidth;
    curr_timestamp = 0;
    repl_policy = p->repl_policy;
    amc_size = p->amc_size;
    amc_assoc = p->amc_assoc;
    regionsize = p->amc_repl_region_size;
    log_regionsize = p->amc_repl_log_region_size;
    log_cacheblocksize = p->log_cacheblocksize;
    num_ps_sets = amc_size / amc_assoc;
    num_ps_sets = num_ps_sets >> PS_METADATA_LINE_SHIFT;
    ps_indexMask = num_ps_sets-1;
    num_sp_sets = amc_size / amc_assoc;
    num_sp_sets = num_sp_sets >> PS_METADATA_LINE_SHIFT;
    sp_indexMask = num_sp_sets-1;
    reset();

    off_chip_latency = p->isb_off_chip_latency;
    ideal_off_chip_transaction = p->isb_off_chip_ideal;
    filler_count = p->isb_off_chip_fillers;

    ps_accesses = 0, ps_hits = 0, ps_prefetch_hits = 0, ps_prefetch_count = 0;
    sp_accesses = 0, sp_hits = 0, sp_prefetch_hits = 0, ps_prefetch_count = 0;
    sp_not_found = 0; sp_invalid = 0;
    metapref_count = 0, metapref_conflict = 0, metapref_duplicate = 0, metapref_actual = 0;
    bulk_actual = 0;
    filler_full_count = 0;
    filler_same_addr = 0;
    filler_load_count = 0;
    filler_store_count = 0;

    bandwidth_delay_cycles = 0;
    issue_delay_cycles = 0;

    ps_optgen_index_mask = num_ps_sets - 1;
    sp_optgen_index_mask = num_sp_sets - 1;
    ps_optgen_mytimer.resize(num_ps_sets, 0);
    sp_optgen_mytimer.resize(num_sp_sets, 0);
    ps_optgen.resize(num_ps_sets); 
    sp_optgen.resize(num_sp_sets); 
    ps_optgen_addr_history.resize(num_ps_sets); 
    sp_optgen_addr_history.resize(num_sp_sets); 
    for (unsigned i = 0; i < num_ps_sets; ++i) {
        ps_optgen_addr_history[i].clear(); 
        ps_optgen[i].init(amc_assoc);
    }
    for (unsigned i = 0; i < num_sp_sets; ++i) {
        sp_optgen_addr_history[i].clear(); 
        sp_optgen[i].init(amc_assoc);
    }

    switch (repl_policy) {
        case ISB_REPL_TYPE_LRU:
        case ISB_REPL_TYPE_METAPREF:
        case ISB_REPL_TYPE_BULKLRU:
        case ISB_REPL_TYPE_BULKMETAPREF:
            repl_ps.resize(num_ps_sets);
            repl_sp.resize(num_sp_sets);
            for (unsigned i = 0; i < num_ps_sets; ++i) {
                repl_ps[i] = new OnChipReplacementLRU;
            }
            for (unsigned i = 0; i < num_sp_sets; ++i) {
                repl_sp[i] = new OnChipReplacementLRU;
            }
            break;
        case ISB_REPL_TYPE_LFU:
            repl_ps.resize(num_ps_sets);
            repl_sp.resize(num_sp_sets);
            for (unsigned i = 0; i < num_ps_sets; ++i) {
                repl_ps[i] = new OnChipReplacementLFU;
            }
            for (unsigned i = 0; i < num_sp_sets; ++i) {
                repl_sp[i] = new OnChipReplacementLFU;
            }
            break;
        case ISB_REPL_TYPE_SRRIP:
            repl_ps.resize(num_ps_sets);
            repl_sp.resize(num_sp_sets);
            for (unsigned i = 0; i < num_ps_sets; ++i) {
                repl_ps[i] = new OnChipReplacementSRRIP;
            }
            for (unsigned i = 0; i < num_sp_sets; ++i) {
                repl_sp[i] = new OnChipReplacementSRRIP;
            }
            break;
        case ISB_REPL_TYPE_BRRIP:
            repl_ps.resize(num_ps_sets);
            repl_sp.resize(num_sp_sets);
            for (unsigned i = 0; i < num_ps_sets; ++i) {
                repl_ps[i] = new OnChipReplacementBRRIP;
            }
            for (unsigned i = 0; i < num_sp_sets; ++i) {
                repl_sp[i] = new OnChipReplacementBRRIP;
            }
            break;
        case ISB_REPL_TYPE_DRRIP:
            repl_ps.resize(num_ps_sets);
            repl_sp.resize(num_sp_sets);
            for (unsigned i = 0; i < num_ps_sets; ++i) {
                repl_ps[i] = new OnChipReplacementDRRIP;
            }
            for (unsigned i = 0; i < num_sp_sets; ++i) {
                repl_sp[i] = new OnChipReplacementDRRIP;
            }
            break;
        case ISB_REPL_TYPE_HAWKEYE:
            repl_ps.resize(num_ps_sets);
            repl_sp.resize(num_sp_sets);
            //cout << "PS sampled: " << endl;
            for (unsigned i = 0; i < num_ps_sets; ++i) {
                repl_ps[i] = new OnChipReplacementHawkeye(num_ps_sets, i, amc_assoc, true);
            }
            //cout << "SP sampled: " << endl;
            for (unsigned i = 0; i < num_sp_sets; ++i) {
                //repl_sp[i] = new OnChipReplacementHawkeye(num_sp_sets, i, amc_assoc, true);
                repl_sp[i] = new OnChipReplacementLRU;
            }
            break;
        case ISB_REPL_TYPE_TLBSYNC:
        case ISB_REPL_TYPE_TLBSYNC_METAPREF:
        case ISB_REPL_TYPE_TLBSYNC_BULKMETAPREF:
        case ISB_REPL_TYPE_OPTGEN:
        case ISB_REPL_TYPE_PERFECT:
            repl_ps.resize(num_ps_sets);
            repl_sp.resize(num_sp_sets);
            for (unsigned i = 0; i < num_ps_sets; ++i) {
                repl_ps[i] = NULL;
            }
            for (unsigned i = 0; i < num_sp_sets; ++i) {
                repl_sp[i] = NULL;
            }
            break;
        default:
            // We have encountered a replacement policy which we don't support.
            // This should not happen. So we just panic quit.
            cerr << "Invalid ISB On Chip Replacement Policy " <<  repl_policy << endl;
    }
}

void OnChipInfo::reset()
{
    ps_amc.resize(num_ps_sets);
    sp_amc.resize(num_sp_sets);
    for (unsigned int i=0; i<num_ps_sets; i++)
        ps_amc[i].clear();

    for (unsigned int i=0; i<num_sp_sets; i++)
        sp_amc[i].clear();
}

size_t OnChipInfo::get_sp_size()
{
    size_t total_size = 0;

    for (size_t i = 0; i < sp_amc.size(); ++i) {
        total_size += sp_amc[i].size();
    }

    return total_size;
}

size_t OnChipInfo::get_ps_size()
{
    size_t total_size = 0;

    for (size_t i = 0; i < ps_amc.size(); ++i) {
        total_size += ps_amc[i].size();
    }

    return total_size;
}

bool OnChipInfo::get_structural_address(uint64_t phy_addr, uint32_t& str_addr, bool update_stats, uint64_t pc, bool clear_dirty)
{
    curr_timestamp++;
    unsigned int setId = PS_SET_ID(phy_addr);
    debug_cout << "get structural addr for addr " << (void*)phy_addr << ", set_id: " << setId
        << ", indexMask: " << ps_indexMask << endl;
    std::map<uint64_t, OnChip_PS_Entry*>& ps_map = ps_amc[setId];
    std::map<uint64_t, OnChip_PS_Entry*>::iterator ps_iter = ps_map.find(PS_LINE_ID(phy_addr));
    unsigned int line_offset = PS_LINE_OFFSET(phy_addr);

    debug_cout << "PS ACCESS: " << (void*)phy_addr << endl;
    if (update_stats) {
        ++ps_accesses;
    }
    if (ps_iter == ps_map.end()) {
#ifdef DEBUG
        (*outf)<<"In on-chip get_structural address of phy_addr "
            <<phy_addr<<", str addr not found\n";
#endif
        return false;
    }
    else if (ps_iter->second->valid[line_offset]) {
        if (update_stats) {
            if (repl_ps[setId] != NULL) {
                debug_cout << "R PS ADD ENTRY: " << setId << ", "
                    << phy_addr << endl;
                repl_ps[setId]->addEntry(PS_LINE_ID(phy_addr), pc, LOAD, true, &ps_predictor);
            }
        }
        str_addr = ps_iter->second->str_addr[line_offset];
        ps_iter->second->last_access = curr_timestamp;
#ifdef DEBUG
        cout <<"In on-chip get_structural address of phy_addr "
            <<(void*)phy_addr<<", str addr is "<< (void*)str_addr<<endl;
#endif
        if (update_stats) {
            ++ps_hits;
        }
        if (clear_dirty) {
            ps_iter->second->dirty = false;
        }
        return true;
    } else {
        debug_cout << "Invalid: " << line_offset << " " << ps_iter->second->valid[line_offset] << endl;
#ifdef DEBUG
        (*outf)<<"In on-chip get_structural address of phy_addr "
            <<phy_addr<<", str addr not valid\n";
#endif
        return false;
    }
}

//#define PERFECT_PS
bool OnChipInfo::get_structural_address_optimal(uint64_t pc, uint64_t phy_addr, uint32_t& str_addr, bool update_stats, bool clear_dirty)
{
    assert(update_stats);
    curr_timestamp++;

    if (update_stats) {
        ++ps_accesses;
    }
    bool off_chip_str_addr_exists = off_chip_info->get_structural_address(phy_addr, str_addr);
#ifdef PERFECT_PS
    if (!off_chip_str_addr_exists) {
        str_addr = INVALID_ADDR;
        return false;
    }

    if (update_stats) {
        ++ps_hits;
    }

    if (clear_dirty) {
        //ps_iter->second->dirty = false;
    }
    return true;
#endif

#ifdef OPTGEN_BULK
    uint64_t paddr = phy_addr >> 6 >> 4;
#else
    uint64_t paddr = phy_addr >> 6;
#endif
    uint64_t setId = paddr & ps_optgen_index_mask;
    assert(setId < num_ps_sets);
    uint64_t curr_quanta = ps_optgen_mytimer[setId];
    bool opt_hit = false;

    if(ps_optgen_addr_history[setId].find(paddr) != ps_optgen_addr_history[setId].end())
    {
        debug_cout << hex << "get_structural_address_optimal() PC:"<<  pc 
            << ", phy addr:"<<phy_addr <<", updates_stats:"<<update_stats
            << ", clear_dirty:"<< clear_dirty
            <<", previous_exist: true"
           << endl;

        uint64_t last_quanta = ps_optgen_addr_history[setId][paddr].last_quanta;
    //    cout << last_quanta << " " << curr_quanta << endl;
        assert(curr_quanta >= ps_optgen_addr_history[setId][paddr].last_quanta);

        opt_hit = (ps_optgen[setId].should_cache(curr_quanta, last_quanta, false)); //TODO: CPU
        uint64_t last_pc = ps_optgen_addr_history[setId][paddr].PC;
        if (update_stats) {
            if (opt_hit){
                if (!optgen_pc_ps_hits.count(last_pc)) {
                    optgen_pc_ps_hits[last_pc] = 0;
                }
                optgen_pc_ps_hits[last_pc]++;
            } else {
                if (!optgen_pc_ps_misses.count(last_pc)) {
                    optgen_pc_ps_misses[last_pc] = 0;
                }
                optgen_pc_ps_misses[last_pc]++;
            }
        }

        //Some maintenance operations for OPTgen
        ps_optgen[setId].add_access(curr_quanta); //TODO: CPU
    }
    // This is the first time we are seeing this line
    else
    {
        debug_cout << hex << "get_structural_address_optimal() PC:"<<  pc 
            << ", phy addr:"<<phy_addr <<", updates_stats:"<<update_stats
            << ", clear_dirty:"<< clear_dirty
            <<", previous_exist: false"
           << endl;

        //Initialize a new entry in the sampler
        ps_optgen_addr_history[setId][paddr].init(curr_quanta);
        ps_optgen[setId].add_access(curr_quanta); //TODO: CPU
    }
    if (update_stats) {
        if (!optgen_pc_ps_total.count(pc)) {
            optgen_pc_ps_total[pc] = 0;
        }
    }
    optgen_pc_ps_total[pc]++;
        
    ps_optgen_addr_history[setId][paddr].update(ps_optgen_mytimer[setId], pc, false); //TODO: PC
    ps_optgen_mytimer[setId]++;
   
    if(opt_hit) {
        bool off_chip_str_addr_exists = off_chip_info->get_structural_address(phy_addr, str_addr);
        if (!off_chip_str_addr_exists) {
            str_addr = INVALID_ADDR;
            return false;
        }

        if (update_stats) {
            ++ps_hits;
        }
        return true;
    }

    return false;

}


bool OnChipInfo::get_physical_address_optimal(uint64_t& phy_addr, uint32_t str_addr, bool update_stats)
{
    curr_timestamp++;

    if (update_stats) {
        ++sp_accesses;
    }
    bool off_chip_phy_addr_exists = off_chip_info->get_physical_address(phy_addr, str_addr);
#ifdef PERFECT_SP
    if (!off_chip_phy_addr_exists) {
        physical_addr = INVALID_ADDR;
        return false;
    }

    if (update_stats) {
        ++sp_hits;
    }

    return true;
#endif

#ifdef OPTGEN_BULK
    uint64_t saddr = str_addr >> 3; //TODO
#else
    uint64_t saddr = str_addr; //TODO
#endif
    uint64_t setId = saddr & sp_optgen_index_mask;
    assert(setId < num_sp_sets);

    uint64_t curr_quanta = sp_optgen_mytimer[setId];
    bool opt_hit = false;

    if(sp_optgen_addr_history[setId].find(saddr) != sp_optgen_addr_history[setId].end())
    {
        uint64_t last_quanta = sp_optgen_addr_history[setId][saddr].last_quanta;
    //    cout << last_quanta << " " << curr_quanta << endl;
        assert(curr_quanta >= sp_optgen_addr_history[setId][saddr].last_quanta);

        opt_hit = (sp_optgen[setId].should_cache(curr_quanta, last_quanta, false)); //TODO: CPU

        //Some maintenance operations for OPTgen
        sp_optgen[setId].add_access(curr_quanta); //TODO: CPU
    }
    // This is the first time we are seeing this line
    else
    {
        //Initialize a new entry in the sampler
        sp_optgen_addr_history[setId][saddr].init(curr_quanta);
        sp_optgen[setId].add_access(curr_quanta); //TODO: CPU
    }

    sp_optgen_addr_history[setId][saddr].update(sp_optgen_mytimer[setId], 0, false); //TODO: PC
    sp_optgen_mytimer[setId]++;

    if(opt_hit) {
        bool off_chip_str_addr_exists = off_chip_info->get_structural_address(phy_addr, str_addr);
        if (!off_chip_str_addr_exists) {
            str_addr = INVALID_ADDR;
            return false;
        }

        if (update_stats) {
            ++sp_hits;
        }
        return true;
    }

    return false;
}

bool OnChipInfo::get_physical_address(uint64_t& phy_addr, uint32_t str_addr, bool update_stats, uint64_t pc)
{
    curr_timestamp++;

    unsigned int setId = SP_SET_ID(str_addr);
    debug_cout << "get physical addr for addr " << (void*)phy_addr << ", set_id: " << setId
        << ", indexMask: " << sp_indexMask << endl;
    std::map<uint32_t, OnChip_SP_Entry*>& sp_map = sp_amc[setId];

    std::map<uint32_t, OnChip_SP_Entry*>::iterator sp_iter = sp_map.find(SP_LINE_ID(str_addr));
    unsigned int line_offset = SP_LINE_OFFSET(str_addr);

    if (update_stats) {
        ++sp_accesses;
    }
    if (sp_iter == sp_map.end()) {
#ifdef DEBUG
        (*outf)<<"In on-chip get_physical_address of str_addr "
            <<str_addr<<", phy addr not found\n";
#endif
        if (update_stats) {
            ++sp_not_found;
        }
        return false;
    }
    else {
        if (sp_iter->second->valid[line_offset]) {
            if (update_stats) {
                if (repl_sp[setId] != NULL) {
                    debug_cout << "SP ADD ENTRY: " << setId << ", "
                        << str_addr << endl;
                    repl_sp[setId]->addEntry(SP_LINE_ID(str_addr), pc, LOAD, true, &sp_predictor);
                }
            }
            phy_addr = sp_iter->second->phy_addr[line_offset];
            sp_iter->second->last_access = curr_timestamp;
#ifdef DEBUG
            (*outf)<<"In on-chip get_physical_address of str_addr "
                <<str_addr<<", phy addr is "<<phy_addr<<endl;
#endif
            if (update_stats) {
                ++sp_hits;
            }
            return true;
        }
        else {
            if (update_stats) {
                ++sp_invalid;
            }
#ifdef DEBUG
            std::cout <<"In on-chip get_physical_address of str_addr "
                <<str_addr<<", phy addr not valid\n";
#endif

            return false;
        }
    }
}

void OnChipInfo::evict_ps_tlbsync(unsigned int setId)
{
    assert(0);
}

void OnChipInfo::evict_sp_tlbsync(unsigned int setId)
{
    assert(0);
}

void OnChipInfo::evict_ps_lru(unsigned ps_setId)
{
    std::map<uint64_t, OnChip_PS_Entry*>& ps_map = ps_amc[ps_setId];
    uint64_t phy_addr_victim = INVALID_ADDR;

    assert(repl_ps.size() > ps_setId);
    assert(repl_ps[ps_setId] != NULL);
    debug_cout << "repl_ps " << (void*) repl_ps[ps_setId] << endl;
    debug_cout << "PS PickVictim: " << ps_setId << endl;

    while (!ps_map.count(phy_addr_victim)) {
        phy_addr_victim = repl_ps[ps_setId]->pickVictim(&ps_predictor); //Should be aligned at metadata line granularity
        std::map<uint64_t, OnChip_PS_Entry*>::iterator ps_iter = ps_map.find(phy_addr_victim);

        debug_cout << "[ISBONCHIP] onchip eviction of " << hex << phy_addr_victim  << endl;
        if(ps_iter == ps_map.end()) {
            assert(0);
            continue;
        }

        //HAO CHECK
        if (off_chip_writeback && ps_iter->second->dirty) {
    
            for(unsigned int i=0; i<PS_METADATA_LINE_SIZE; i++)
            {
                uint64_t paddr = ((ps_iter->first << PS_METADATA_LINE_SHIFT) + i) << 6;
                off_chip_info->update(paddr, ps_iter->second->str_addr[i]);
                debug_cout << "Evict " << hex << paddr << dec<< endl;
#ifdef BLOOM_ISB
#ifdef BLOOM_ISB_TRAFFIC_DEBUG
                printf("Bloom add b: 0x%lx\n", ps_iter->first);
#endif
                if (pref->get_bloom_capacity() != 0) {
                    pref->add_to_bloom_filter_ps(paddr);
                    pref->add_to_bloom_filter_sp(ps_iter->second->str_addr[i]>>3);
                }
#endif
            if (count_off_chip_write_traffic) {
#ifdef BLOOM_ISB
#ifdef BLOOM_ISB_TRAFFIC_DEBUG
                    printf("Bloom add b: 0x%lx count\n", ps_iter->first);
#endif
#endif  
                    //TODO: AJ
                    //access_off_chip(ps_iter->first, ps_iter->second->str_addr, ISB_OCI_REQ_STORE);
                    access_off_chip(paddr, ps_iter->second->str_addr[i], ISB_OCI_REQ_STORE);
                }
            }
        }
        debug_cout << "evict_ps_lru entry, ps_set_id: " << ps_setId
            << ", ps_set_size: " << ps_map.size()
            << endl;
        assert(ps_map.count(phy_addr_victim));
        ps_map.erase(phy_addr_victim);
        break;
    }
}

void OnChipInfo::evict_sp_lru(unsigned sp_setId)
{
    debug_cout << "evict sp lru for " << sp_setId << endl;
    std::map<uint32_t, OnChip_SP_Entry*>& sp_map = sp_amc[sp_setId];
    assert(sp_map.size() >= amc_assoc);
    uint32_t str_addr_victim = INVALID_ADDR;

    assert(repl_sp.size() > sp_setId);
    assert(repl_sp[sp_setId] != NULL);
    debug_cout << "repl_sp " << (void*) repl_sp[sp_setId] << endl;
    debug_cout << "SP PickVictim: " << sp_setId << endl;
    bool victim_not_found = true;
    while (victim_not_found) {
        str_addr_victim = repl_sp[sp_setId]->pickVictim(&sp_predictor);
        debug_cout << "str victim: " << (void*)(str_addr_victim) << endl;
        for (map<uint32_t, OnChip_SP_Entry*>::iterator it = sp_map.begin();
                it != sp_map.end(); ++it)
            debug_cout << "sp_map[" << it->first << "] = " << it->second
                << endl;
        assert(sp_map.count(str_addr_victim));
        if (sp_map.count(str_addr_victim)) {
            victim_not_found = false;
            sp_map.erase(str_addr_victim);
        }
    }
#ifdef COUNT_STREAM_DETAIL
    active_stream_set.erase(str_addr_victim>>STREAM_MAX_LENGTH_BITS);
#endif
}

void OnChipInfo::evict_ps(unsigned ps_setId)
{
    std::map<uint64_t, OnChip_PS_Entry*>& ps_map = ps_amc[ps_setId];
    debug_cout << "Trying to evict for ps_setId: " << ps_setId
        << ", size: " << ps_map.size() << endl;

    switch(repl_policy) {
        case ISB_REPL_TYPE_TLBSYNC:
        case ISB_REPL_TYPE_TLBSYNC_METAPREF:
        case ISB_REPL_TYPE_TLBSYNC_BULKMETAPREF:
            while (ps_map.size() > amc_assoc) {
                evict_ps_tlbsync(ps_setId);
            }
            break;
        case ISB_REPL_TYPE_PERFECT:
            // DO NOTHING
            break;
        case ISB_REPL_TYPE_OPTGEN:
            break;
        default:
            while (ps_map.size() >= amc_assoc) {
                evict_ps_lru(ps_setId);
            }
    }
}

void OnChipInfo::evict_sp(unsigned sp_setId)
{
    std::map<uint32_t, OnChip_SP_Entry*>& sp_map = sp_amc[sp_setId];
    debug_cout << "Trying to evict for sp_setId: " << sp_setId
        << ", size: " << sp_map.size() << endl;

    switch(repl_policy) {
        case ISB_REPL_TYPE_TLBSYNC:
        case ISB_REPL_TYPE_TLBSYNC_METAPREF:
        case ISB_REPL_TYPE_TLBSYNC_BULKMETAPREF:
            while (sp_map.size() >= amc_assoc) {
                evict_sp_tlbsync(sp_setId);
            }
            break;
        case ISB_REPL_TYPE_PERFECT:
            // DO NOTHING
            break;
        case ISB_REPL_TYPE_OPTGEN:
            break;
        default:
            while (sp_map.size() >= amc_assoc) {
                evict_sp_lru(sp_setId);
            }
            break;
    }
}

void OnChipInfo::update(uint64_t phy_addr, uint32_t str_addr, bool set_dirty, uint64_t pc)
{
    if (repl_policy == ISB_REPL_TYPE_OPTGEN)
        return;
#ifdef DEBUG
    (*outf)<<"In on_chip_info update, phy_addr is "
        <<phy_addr<<", str_addr is "<<str_addr<<endl;
#endif

    unsigned int ps_setId = PS_SET_ID(phy_addr);
    std::map<uint64_t, OnChip_PS_Entry*>& ps_map = ps_amc[ps_setId];

    unsigned int sp_setId = SP_SET_ID(str_addr);
    std::map<uint32_t, OnChip_SP_Entry*>& sp_map = sp_amc[sp_setId];
    ++update_count;

    debug_cout << "[ISBONCHIP] onchip update: phy_addr=" << (void*)phy_addr
        << " str_addr=" << (void*)str_addr
        << " ps_setId=" << ps_setId
        << " sp_setId=" << sp_setId
        << endl;

    std::map<uint64_t, OnChip_PS_Entry*>::iterator ps_iter = ps_map.find(PS_LINE_ID(phy_addr));
    unsigned int line_offset = PS_LINE_OFFSET(phy_addr);
    debug_cout << "ID: " << hex << PS_LINE_ID(phy_addr) << " " << PS_LINE_OFFSET(phy_addr) << dec << endl;

    if (ps_iter == ps_map.end()) {
        evict_ps(ps_setId);
        OnChip_PS_Entry* ps_entry = new OnChip_PS_Entry();
        ps_map[PS_LINE_ID(phy_addr)] = ps_entry;
        ps_map[PS_LINE_ID(phy_addr)]->set(str_addr, line_offset);
        ps_map[PS_LINE_ID(phy_addr)]->last_access = curr_timestamp;
        if (set_dirty) {
            ps_map[PS_LINE_ID(phy_addr)]->dirty = true;
        }
        if (repl_ps[ps_setId] != NULL) {
            debug_cout << "W PS ADD ENTRY UPDATE: " << ps_setId << ", "
                << phy_addr << endl;
            repl_ps[ps_setId]->addEntry(PS_LINE_ID(phy_addr), pc, WRITEBACK, false, &ps_predictor); 
        }
    }
    //This counts double if we come here after an invalidation
    else if (ps_iter->second->valid[line_offset] == false) {
        ps_map[PS_LINE_ID(phy_addr)]->set(str_addr, line_offset);
        ps_map[PS_LINE_ID(phy_addr)]->last_access = curr_timestamp;
        if (set_dirty) {
            ps_map[PS_LINE_ID(phy_addr)]->dirty = true;
        }
        if (repl_ps[ps_setId] != NULL) {
            debug_cout << "W PS ADD ENTRY UPDATE: " << ps_setId << ", "
                << phy_addr << endl;
            repl_ps[ps_setId]->addEntry(PS_LINE_ID(phy_addr), pc, WRITEBACK, false, &ps_predictor); 
        }
    }
    else if (ps_iter->second->str_addr[line_offset] != str_addr) {
#ifdef OVERWRITE_OFF_CHIP
        ps_iter->second->set(str_addr, line_offset);
        ps_iter->second->last_access = curr_timestamp;
        if (set_dirty) {
            ps_map[PS_LINE_ID(phy_addr)]->dirty = true;
        } else {
            ps_map[PS_LINE_ID(phy_addr)]->dirty = false;
        }
#else
        if (set_dirty) {
            ps_iter->second->set(str_addr, line_offset);
            ps_iter->second->last_access = curr_timestamp;
            ps_map[PS_LINE_ID(phy_addr)]->dirty = true;
        }
#endif
    } else {
        ps_iter->second->last_access = curr_timestamp;
        if (!set_dirty) {
            ps_map[PS_LINE_ID(phy_addr)]->dirty = false;
        }
    }

    //SP Map Update
    std::map<uint32_t, OnChip_SP_Entry*>::iterator sp_iter = sp_map.find(SP_LINE_ID(str_addr));
    line_offset = SP_LINE_OFFSET(str_addr);
    if (sp_iter == sp_map.end()) {
        evict_sp(sp_setId);
        OnChip_SP_Entry* sp_entry = new OnChip_SP_Entry();
        sp_map[SP_LINE_ID(str_addr)] = sp_entry;
        sp_map[SP_LINE_ID(str_addr)]->set(phy_addr, line_offset);
        sp_map[SP_LINE_ID(str_addr)]->last_access = curr_timestamp;
    }
    else {
#ifdef OVERWRITE_OFF_CHIP
        sp_iter->second->set(phy_addr, line_offset);
        sp_iter->second->last_access = curr_timestamp;
#else
        if(set_dirty) {
            sp_iter->second->set(phy_addr, line_offset);
            sp_iter->second->last_access = curr_timestamp;
        }
#endif
    }

    if (repl_sp[sp_setId] != NULL) {
        debug_cout << "SP ADD ENTRY UPDATE: " << sp_setId << ", "
            << str_addr << endl;
        repl_sp[sp_setId]->addEntry(SP_LINE_ID(str_addr), pc, WRITEBACK, false, &sp_predictor);
    }

    debug_cout << "add entry, ps_set_id: " << ps_setId
        << ", ps_set_size: " << ps_map.size()
        << ", sp_set_size: " << sp_map.size()
        << endl;

    if (repl_policy == ISB_REPL_TYPE_HAWKEYE) {
        debug_cout << "add entry, repl_ps_size: "
            << static_cast<OnChipReplacementHawkeye*>(repl_ps[ps_setId])->get_entry_list_size() <<
            "add entry, repl_sp_size: "
            << static_cast<OnChipReplacementHawkeye*>(repl_sp[sp_setId])->get_entry_list_size()
            << endl;
//        assert(static_cast<OnChipReplacementHawkeye*>(repl_ps[ps_setId])->get_entry_list_size() == ps_map.size());
//        assert(static_cast<OnChipReplacementHawkeye*>(repl_sp[sp_setId])->get_entry_list_size() == sp_map.size());
    }
}

void OnChipInfo::invalidate(uint64_t phy_addr, uint32_t str_addr)
{
    unsigned int ps_setId = PS_SET_ID(phy_addr);
    std::map<uint64_t, OnChip_PS_Entry*>& ps_map = ps_amc[ps_setId];

    unsigned int sp_setId = SP_SET_ID(str_addr);
    std::map<uint32_t, OnChip_SP_Entry*>& sp_map = sp_amc[sp_setId];
#ifdef DEBUG
    (*outf)<<"In on_chip_info invalidate, phy_addr is "
        <<phy_addr<<", str_addr is "<<str_addr<<endl;
#endif
    //PS Map Invalidate
    std::map<uint64_t, OnChip_PS_Entry*>::iterator ps_iter =
        ps_map.find(PS_LINE_ID(phy_addr));
    if (ps_iter != ps_map.end()) {
        ps_iter->second->reset(PS_LINE_OFFSET(phy_addr));
        debug_cout << "invalidate entry, ps_set_id: " << ps_setId
            << ", ps_set_size: " << ps_map.size()
            << endl;

        //TODO AJ: Should we inform the replacement policy?
        //delete ps_iter->second;
        //ps_map.erase(ps_iter);
        //if (repl_ps[ps_setId] != NULL) {
        //    repl_ps[ps_setId]->eraseEntry(phy_addr);
        //}
    }

    //SP Map Invalidate
    std::map<uint32_t, OnChip_SP_Entry*>::iterator sp_iter =
        sp_map.find(SP_LINE_ID(str_addr));
    if (sp_iter != sp_map.end()) {
        sp_iter->second->reset(SP_LINE_OFFSET(str_addr));
        //delete sp_iter->second;
        //sp_map.erase(sp_iter);
    }
    else {
        //TODO TBD
    }

#ifdef DEBUG
    (*outf)<<"In on_chip_info invalidate prefetch buffer, phy_addr is "
        <<phy_addr<<", str_addr is "<<str_addr<<endl;
#endif
}

void OnChipInfo::increase_confidence(uint64_t phy_addr)
{
    unsigned int ps_setId = PS_SET_ID(phy_addr);
    std::map<uint64_t, OnChip_PS_Entry*>& ps_map = ps_amc[ps_setId];
#ifdef DEBUG
    (*outf)<<"In on_chip_info increase_confidence, phy_addr is "
        <<phy_addr<<endl;
#endif
    std::map<uint64_t, OnChip_PS_Entry*>::iterator ps_iter =
        ps_map.find(PS_LINE_ID(phy_addr));
    if (ps_iter != ps_map.end()) {
        ps_iter->second->increase_confidence(PS_LINE_OFFSET(phy_addr));
    }
}

bool OnChipInfo::lower_confidence(uint64_t phy_addr)
{
    bool ret = false;

    unsigned int ps_setId = PS_SET_ID(phy_addr);
    std::map<uint64_t, OnChip_PS_Entry*>& ps_map = ps_amc[ps_setId];
#ifdef DEBUG
    (*outf)<<"In on_chip_info lower_confidence, phy_addr is "
        <<phy_addr<<endl;
#endif

    std::map<uint64_t, OnChip_PS_Entry*>::iterator ps_iter =
        ps_map.find(PS_LINE_ID(phy_addr));
    if (ps_iter != ps_map.end()) {
        ret = ps_iter->second->lower_confidence(PS_LINE_OFFSET(phy_addr));
    }
    return ret;
}

void OnChipInfo::mark_not_tlb_resident(uint64_t phy_addr)
{
    assert(0);
}

void OnChipInfo::mark_tlb_resident(uint64_t phy_addr)
{
    assert(0);
}

#ifdef COUNT_STREAM_DETAIL
bool OnChipInfo::is_trigger_access(uint32_t str_addr)
{
    if (!active_stream_set.count(str_addr >> STREAM_MAX_LENGTH_BITS)) {
        active_stream_set.insert(str_addr >> STREAM_MAX_LENGTH_BITS);
        return true;
    }

    return false;
}
#endif

void OnChipInfo::doPrefetch(uint64_t phy_addr, uint32_t str_addr, bool is_second, uint64_t pc)
{
    //cout << "Prefetch " << hex << str_addr << dec << " " << is_second << endl;
    std::vector<uint64_t> pref_list =
        oci_pref.getNextAddress(0, phy_addr, str_addr);
    uint64_t pref_phy_addr;
    uint32_t pref_str_addr = str_addr;
    uint64_t exist_phy_addr;
    for (unsigned i = 0; i < pref_list.size(); ++i) {
        pref_phy_addr = pref_list[i];
        ++pref_str_addr;

        debug_cout << hex << "ONCHIPINFOPREF STR ADDR: " << str_addr
            << ", PHY ADDR: " << phy_addr
            << ", PREF STR ADDR: " << pref_str_addr
            << ", PREF PHY ADDR: " << pref_phy_addr
            << endl;
        bool phy_on_chip_exist;
        if (repl_policy != ISB_REPL_TYPE_OPTGEN){
            phy_on_chip_exist = get_physical_address(exist_phy_addr, pref_str_addr, false, 0);
        } else {
            phy_on_chip_exist = get_physical_address_optimal(exist_phy_addr, pref_str_addr, false);
        }

        if (phy_on_chip_exist) {
            if (exist_phy_addr == pref_phy_addr) {
                ++metapref_duplicate;
            } else {
                ++metapref_conflict;
            }
        } else {
            ++metapref_actual;
        }
        if (!phy_on_chip_exist) {
            if (ideal_off_chip_transaction) {
                if (pref_phy_addr != INVALID_ADDR) {
                    update(pref_phy_addr, pref_str_addr, false, pc);
                }
            } else {
                debug_cout << "Metapref " << (void*)pref_phy_addr << " to " << (void*)pref_str_addr << endl;
                if (is_second) {
                    access_off_chip(pref_phy_addr, pref_str_addr, ISB_OCI_REQ_LOAD_SP2);
                } else {
                    access_off_chip(pref_phy_addr, pref_str_addr, ISB_OCI_REQ_LOAD_SP1);
                }
            }
        }
        ++metapref_count;
    }
}

#if 0
void OnChipInfo::doPrefetchBulk(uint64_t phy_addr, uint32_t str_addr, bool is_second)
{
    assert(0);
    std::vector<uint64_t> pref_list =
        oci_pref.getNextAddress(0, phy_addr, str_addr);
    uint64_t pref_phy_addr;
    uint32_t pref_str_addr = str_addr;
    for (unsigned i = 0; i < pref_list.size(); ++i) {
        pref_phy_addr = pref_list[i];
        debug_cout << hex << "STR ADDR: " << str_addr
            << ", PHY ADDR: " << phy_addr
            << ", PREF PHY ADDR: " << pref_phy_addr
            << endl;
        ++metapref_count;
        uint64_t exist_phy_addr;
        bool phy_on_chip_exist = get_physical_address(exist_phy_addr, pref_str_addr, false);
        ++pref_str_addr;
        if (phy_on_chip_exist) {
            if (exist_phy_addr == pref_phy_addr) {
                ++metapref_duplicate;
            } else {
                ++metapref_conflict;
            }
        } else {
            ++metapref_actual;
        }

        fetch_bulk(pref_phy_addr, is_second?ISB_OCI_REQ_LOAD_SP2:ISB_OCI_REQ_LOAD_SP1);
    }
}
#endif

void OnChipInfo::print()
{
    std::cerr << "OnChipInfo PHY ACCESS: " << ps_accesses
        << ", HITS: " << ps_hits
        << ", PREFHIT: " << ps_prefetch_hits << std::endl;
    std::cerr << "OnChipInfo STR ACCESS: " << sp_accesses
        << ", HITS: " << sp_hits
        << ", PREFHIT: " << sp_prefetch_hits << std::endl;

//#if 0
    // Print PC Bias detail
    //
    if (repl_policy == ISB_REPL_TYPE_OPTGEN) {
        cout << "OPTGEN PC HIT COUNT:" << endl;
        for (map<uint64_t, uint32_t>::const_iterator it = optgen_pc_ps_total.begin();
                it != optgen_pc_ps_total.end(); ++it) {
            uint64_t pc = it->first;
            if(it->second > 50)
                cout << hex << pc << ' ' << dec << it->second << ' ' << optgen_pc_ps_hits[pc]
                << ' ' << optgen_pc_ps_misses[pc] << " " << (double)optgen_pc_ps_hits[pc]/(double)optgen_pc_ps_total[pc] << endl;
        }
    }
//#endif
    // Print OPTGEN results for Hawkeye replacement policy
    if (repl_policy == ISB_REPL_TYPE_HAWKEYE) {
        unsigned int ps_hits = 0, ps_access = 0, ps_traffic = 0;
        unsigned int sp_hits = 0, sp_access = 0, sp_traffic = 0;
        OPTgen* optgen;
        for(unsigned int i=0; i<repl_ps.size(); i++)
        {
            optgen = static_cast<OnChipReplacementHawkeye*>(repl_ps[i])->get_optgen();
            ps_access += optgen->access;
            ps_hits += optgen->get_num_opt_hits();
            ps_traffic += optgen->get_traffic();
#if 0
            cout << "Hawkeye PC HIT COUNT:" << endl;
            for (map<uint64_t, uint32_t>::const_iterator it = repl_ps[i]->hawkeye_pc_ps_total_predictions.begin();
                    it != repl_ps[i]->hawkeye_pc_ps_total_predictions.end(); ++it) {
                uint64_t pc = it->first;
                if(it->second > 50)
                    cout << hex << pc << ' ' << dec << it->second << ' ' << repl_ps[i]->hawkeye_pc_ps_hit_predictions[pc]
                        << ' ' << (double)repl_ps[i]->hawkeye_pc_ps_hit_predictions[pc]/(double)repl_ps[i]->hawkeye_pc_ps_total_predictions[pc] << endl;
            }
#endif
        }
        for(unsigned int i=0; i<repl_sp.size(); i++)
        {
            optgen = static_cast<OnChipReplacementHawkeye*>(repl_sp[i])->get_optgen();
            sp_access += optgen->access;
            sp_hits += optgen->get_num_opt_hits();
            sp_traffic += optgen->get_traffic();
        }

        std::cout << "PS OPTgen accesses: " << ps_access
            << ", hits: " << ps_hits
            << ", traffic: " << ps_traffic
            << ", hit rate: " << double(ps_hits) / double(ps_access)
            << ", traffic rate: " << double(ps_traffic) / double(ps_access)
            << std::endl;
        std::cout << "SP OPTgen accesses: " << sp_access
            << ", hits: " << sp_hits
            << ", traffic: " << sp_traffic
            << ", hit rate: " << double(sp_hits) / double(sp_access)
            << ", traffic rate: " << double(sp_traffic) / double(sp_access)
            << std::endl;

        
    }
    if (repl_policy == ISB_REPL_TYPE_OPTGEN) {
        unsigned int ps_hits = 0, ps_access = 0, ps_traffic = 0;
        unsigned int sp_hits = 0, sp_access = 0, sp_traffic = 0;
        OPTgen* optgen;
        for(unsigned int i=0; i<ps_optgen.size(); i++)
        {
            optgen = &ps_optgen[i];
            ps_access += optgen->access;
            ps_hits += optgen->get_num_opt_hits();
            ps_traffic += optgen->get_traffic();
        }
        for(unsigned int i=0; i<sp_optgen.size(); i++)
        {
            optgen = &sp_optgen[i];
            sp_access += optgen->access;
            sp_hits += optgen->get_num_opt_hits();
            sp_traffic += optgen->get_traffic();
        }

        std::cout << "PS OPTgen accesses: " << ps_access
            << ", hits: " << ps_hits
            << ", traffic: " << ps_traffic
            << ", hit rate: " << double(ps_hits) / double(ps_access)
            << ", traffic rate: " << double(ps_traffic) / double(ps_access)
            << std::endl;
        std::cout << "SP OPTgen accesses: " << sp_access
            << ", hits: " << sp_hits
            << ", traffic: " << sp_traffic
            << ", hit rate: " << double(sp_hits) / double(sp_access)
            << ", traffic rate: " << double(sp_traffic) / double(sp_access)
            << std::endl;
    }

}

void OnChipInfo::write_off_chip_region(uint64_t phy_addr, uint32_t str_addr, off_chip_req_type_t req_type)
{
    // Only Support store request for write_off_chip_region
    assert(req_type == ISB_OCI_REQ_STORE);
    bool success;
    phy_addr = (phy_addr>>10)<<10;
    for (unsigned offset = 0; offset < 16; ++offset) {
        success = get_structural_address(phy_addr + (offset<<6), str_addr, false, 0, true);
        if (success) {
            debug_cout << "OCIFILLER UPDATE PHY REGION: " << (void*)(phy_addr+(offset<<6)) << " TO "
                << (void*)str_addr << endl;
            off_chip_info->update(phy_addr+(offset<<6), str_addr);
            #ifdef BLOOM_ISB
              #ifdef BLOOM_ISB_TRAFFIC_DEBUG
              printf("Bloom add i: 0x%lx\n", phy_addr+(offset<<6));
              #endif
            if (pref->get_bloom_capacity() != 0) {
                pref->add_to_bloom_filter_ps(phy_addr+(offset<<6));
                  //printf("Adding to SP: 0x%lx\n", str_addr);
                pref->add_to_bloom_filter_sp(str_addr>>3);
            }
            #endif
        }
    }
}

void OnChipInfo::update_phy_region(uint64_t phy_addr)
{
    bool success = true;
    phy_addr = (phy_addr>>10)<<10;
    uint32_t str_addr;

    for (unsigned offset = 0; offset < 16; ++ offset) {
        success = off_chip_info->get_structural_address(phy_addr + (offset<<6), str_addr);
        if (success) {
            debug_cout << "UPDATE PHY REGION: " << (void*)(phy_addr+(offset<<6)) << " TO "
                << (void*)str_addr << endl;
            update(phy_addr+(offset<<6), str_addr, false, 0);
        }
    }
    pref->issue_prefetch_buffer();
}

void OnChipInfo::update_str_region(uint32_t str_addr)
{
    bool success;
    str_addr = (str_addr>>3)<<3;
    uint64_t phy_addr;
    for (unsigned offset = 0; offset < 8; ++ offset) {
        success = off_chip_info->get_physical_address(phy_addr, str_addr+offset);
        if (success) {
            //cout << "                  " << hex << (str_addr+offset) << dec << endl;
            debug_cout << "OCIFILLER UPDATE STR REGION: " << (void*)(phy_addr+(offset<<6)) << " TO "
                << (void*)str_addr << endl;
            debug_cout << "OCIFILLER UPDATE STR REGION: " << (void*)(phy_addr) << " TO "
                << (void*)(str_addr+offset) << endl;
            update(phy_addr, str_addr+offset, false, 0);
        }
    }
    pref->issue_prefetch_buffer();
}


void OnChipInfo::access_off_chip(uint64_t phy_addr, uint32_t str_addr, off_chip_req_type_t req_type)
{
    //insert_metadata(phy_addr, str_addr, req_type);

    if (req_type == 0 && str_addr != INVALID_ADDR)
    debug_cout << "Inside access_off_chip: phy_addr: " << (void*)phy_addr
        << ", str_addr: " << (void*)str_addr
        << ", req_type: " << req_type
        << endl;

   if (req_type == ISB_OCI_REQ_STORE) {
        pref->write_metadata(phy_addr>>10, req_type);
        assert(phy_addr != INVALID_ADDR && str_addr != INVALID_ADDR);
        if (repl_policy == ISB_REPL_TYPE_LRU || repl_policy == ISB_REPL_TYPE_LFU
                || repl_policy == ISB_REPL_TYPE_METAPREF) {
            off_chip_info->update(phy_addr, str_addr);
                        #ifdef BLOOM_ISB
                          #ifdef BLOOM_ISB_TRAFFIC_DEBUG
                          printf("Bloom add j: 0x%lx\n", phy_addr);
                          printf("Bloom add j: 0x%lx count\n", phy_addr);
                          #endif
                        if (pref->get_bloom_capacity() != 0) {
                           pref->add_to_bloom_filter_ps(phy_addr);
                              //printf("Adding to SP: 0x%lx\n", str_addr);
                           pref->add_to_bloom_filter_sp(str_addr>>3);
                        }
                        #endif
        } else if (repl_policy == ISB_REPL_TYPE_BULKLRU
                || repl_policy == ISB_REPL_TYPE_BULKMETAPREF
                || repl_policy == ISB_REPL_TYPE_SRRIP
                || repl_policy == ISB_REPL_TYPE_BRRIP
                || repl_policy == ISB_REPL_TYPE_DRRIP
                || repl_policy == ISB_REPL_TYPE_HAWKEYE
                ) {
            write_off_chip_region(phy_addr, str_addr, req_type);
        }
    } else if (req_type == ISB_OCI_REQ_LOAD_PS) {
        assert(phy_addr != INVALID_ADDR);
        if (repl_policy == ISB_REPL_TYPE_LRU || repl_policy == ISB_REPL_TYPE_LFU
                || repl_policy == ISB_REPL_TYPE_METAPREF) {


    #ifdef BLOOM_ISB
    // Check bloom filter
    if (pref->get_bloom_capacity() != 0) {
      if (!pref->lookup_bloom_filter_ps(phy_addr)) {
        return;
      }
    }
    #endif

            pref->read_metadata((uint64_t)(phy_addr>>10), phy_addr, str_addr, req_type);
            //off_chip_info->get_structural_address(phy_addr, str_addr);
            //if (str_addr != INVALID_ADDR) {
             //   update(phy_addr, str_addr, false);
            //}
        } else if (repl_policy == ISB_REPL_TYPE_BULKLRU
                || repl_policy == ISB_REPL_TYPE_SRRIP
                || repl_policy == ISB_REPL_TYPE_BRRIP
                || repl_policy == ISB_REPL_TYPE_DRRIP
                || repl_policy == ISB_REPL_TYPE_HAWKEYE
                || repl_policy == ISB_REPL_TYPE_BULKMETAPREF) {
            //bool ret = update_phy_region(phy_addr); //Done: Make sure not already on chip
#ifdef BLOOM_ISB
    if (pref->get_bloom_capacity() != 0) {
        if (!pref->lookup_bloom_filter_ps(phy_addr)) {
            debug_cout << "Filter PS read to " << hex << phy_addr << dec << endl;
            return;
        }
        debug_cout << "Allow PS read to " << hex << phy_addr << dec << endl;
    }
#endif

            //assert(ret);
            pref->read_metadata((uint64_t)(phy_addr>>10), phy_addr, str_addr, req_type);
        }
        // TODO: This should actually be done after the data has arrived!
        //debug_cout << "str_addr: " << str_addr << ", invalid_addr: " << INVALID_ADDR
         //   << ", equal = " << (str_addr == INVALID_ADDR) << endl;
        //if (str_addr != INVALID_ADDR) {
         //   pref->isb_predict(phy_addr, str_addr);
        //}
    } else if (req_type == ISB_OCI_REQ_LOAD_SP1 || req_type == ISB_OCI_REQ_LOAD_SP2) {
        assert(str_addr != INVALID_ADDR);
    // Check bloom filter
    #ifdef BLOOM_ISB_SP
    if (pref->get_bloom_capacity() != 0) {
      bool off_chip_bloom_found = pref->lookup_bloom_filter_sp(str_addr>>3);
      uint64_t fake_phy_addr;
      bool off_chip_found = off_chip_info->get_physical_address(fake_phy_addr, str_addr);

      if (off_chip_bloom_found) {
          ++pref->sp_bloom_found;
          if (off_chip_found) {
              debug_cout << "SP OFFCHIP YES CORRECT" << (void*)str_addr << endl;
              ++pref->sp_offchip_bloom_correct;
          } else {
              debug_cout << "SP OFFCHIP YES INCORRECT" << (void*)str_addr << endl;
              ++pref->sp_offchip_bloom_incorrect;
          }
      } else {
          ++pref->sp_bloom_not_found;
          if (!off_chip_found) {
              debug_cout << "SP OFFCHIP NO CORRECT" << (void*)str_addr << endl;
              ++pref->sp_not_offchip_bloom_correct;
          } else {
              debug_cout << "SP OFFCHIP NO INCORRECT" << (void*)str_addr << endl;
              ++pref->sp_not_offchip_bloom_incorrect;
          }
      }
      if (off_chip_found) {
          ++pref->sp_bloom_wb_found;
      } else {
          ++pref->sp_bloom_wb_not_found;
      }

      if (!pref->lookup_bloom_filter_sp(str_addr>>3)) {
            debug_cout << "Filter SP read to " << hex << str_addr << dec << endl;
            return;
      }
        debug_cout << "Allow SP read to " << hex << str_addr << dec << endl;
    }
    #endif

        if (repl_policy == ISB_REPL_TYPE_LRU || repl_policy == ISB_REPL_TYPE_LFU
                || repl_policy == ISB_REPL_TYPE_METAPREF) {
            pref->read_metadata((uint64_t)(str_addr>>3), phy_addr, str_addr, req_type);
          //  off_chip_info->get_physical_address(phy_addr, str_addr);
            //if (phy_addr != INVALID_ADDR) {
              //  update(phy_addr, str_addr, false);
     //       }
        } else if (repl_policy == ISB_REPL_TYPE_BULKLRU
                || repl_policy == ISB_REPL_TYPE_SRRIP
                || repl_policy == ISB_REPL_TYPE_BRRIP
                || repl_policy == ISB_REPL_TYPE_DRRIP
                || repl_policy == ISB_REPL_TYPE_HAWKEYE
                || repl_policy == ISB_REPL_TYPE_BULKMETAPREF) {
        //    update_str_region(str_addr);
            pref->read_metadata((uint64_t)(str_addr>>3), phy_addr, str_addr, req_type);
        }
    }
}

void OnChipInfo::insert_metadata_ps(uint64_t phy_addr)
{
    debug_cout << "Inside insert_metdata_ps: phy_addr: " << (void*)phy_addr << endl;

    uint32_t str_addr;
    assert(phy_addr != INVALID_ADDR);
    if (repl_policy == ISB_REPL_TYPE_LRU || repl_policy == ISB_REPL_TYPE_LFU
            || repl_policy == ISB_REPL_TYPE_METAPREF) {

        bool ret = off_chip_info->get_structural_address(phy_addr, str_addr);
        if (ret && str_addr != INVALID_ADDR) {
            update(phy_addr, str_addr, false, 0);
        }
    } else if (repl_policy == ISB_REPL_TYPE_BULKLRU
                || repl_policy == ISB_REPL_TYPE_SRRIP
                || repl_policy == ISB_REPL_TYPE_BRRIP
                || repl_policy == ISB_REPL_TYPE_DRRIP
                || repl_policy == ISB_REPL_TYPE_HAWKEYE
            || repl_policy == ISB_REPL_TYPE_BULKMETAPREF) {
        update_phy_region(phy_addr);
    }

    //bool ret = off_chip_info->get_structural_address(phy_addr, str_addr);
    //if (ret && (str_addr != INVALID_ADDR)) {
        //cout << "Doing delayed prediction on " << hex << phy_addr << " " << str_addr << dec << endl;
     //   pref->isb_predict(phy_addr, str_addr);
    //}
}

void OnChipInfo::insert_metadata_sp(uint32_t str_addr)
{
    debug_cout << "Inside insert_metdata: phy_addr: "
        << ", str_addr: " << (void*)str_addr
        << endl;
    debug_cout << "Completed metadata " << hex << str_addr << dec << endl;
    assert(str_addr != INVALID_ADDR);
    uint64_t phy_addr;
    if (repl_policy == ISB_REPL_TYPE_LRU || repl_policy == ISB_REPL_TYPE_LFU
            || repl_policy == ISB_REPL_TYPE_METAPREF) {
        bool ret = off_chip_info->get_physical_address(phy_addr, str_addr);
        if (ret && phy_addr != INVALID_ADDR) {
            update(phy_addr, str_addr, false, 0);
        }
    } else if (repl_policy == ISB_REPL_TYPE_BULKLRU
                || repl_policy == ISB_REPL_TYPE_SRRIP
                || repl_policy == ISB_REPL_TYPE_BRRIP
                || repl_policy == ISB_REPL_TYPE_DRRIP
                || repl_policy == ISB_REPL_TYPE_HAWKEYE
            || repl_policy == ISB_REPL_TYPE_BULKMETAPREF) {
        update_str_region(str_addr);
    }

}



void ISBOnChipPref::init(OffChipInfo* off_chip_info, int metapref_degree)
{
    this->off_chip_info = off_chip_info;
    degree = metapref_degree;

    metapref_success = 0, metapref_not_found = 0, metapref_stream_end = 0;
}

vector<uint64_t> ISBOnChipPref::getNextAddress(uint64_t pc, uint64_t paddr, uint32_t saddr)
{
    // This approach uses Structure uint64_tess to Prefetch
    // We might use other features if appropriate
    //
    vector<uint64_t> result_list;
    debug_cout << hex << "isb on chip pref receive: pc: " << pc << ", paddr: " << paddr << ", saddr: " << saddr
        << ", degree = " << degree << endl;

    uint32_t pref_saddr;
    uint64_t pref_paddr;
    for (unsigned i = 1; i <= degree; ++i) {
        pref_saddr = saddr + i;
        if (pref_saddr % STREAM_MAX_LENGTH == 0) {
            ++metapref_stream_end;
            break;
        }

        bool success =
            off_chip_info->get_physical_address(pref_paddr, pref_saddr);

        off_chip_info->phy_access_count++;
        if (success) {
            off_chip_info->phy_success_count++;
            metapref_success++;
        } else {
            metapref_not_found++;
        }

#ifdef DEBUG
        std::cerr << " ISB metadata Prefetch success: " << success
            << " PHY ADDR: " << pref_paddr
            << " STR ADDR: " << pref_saddr
            << std::endl;
#endif
        if (!success) {
            pref_paddr = INVALID_ADDR;
        }

        result_list.push_back(pref_paddr);
    }

    return result_list;
}

void OnChipReplacementLRU::addEntry(uint64_t entry, uint64_t pc, int type, bool hit, IsbHawkeyePCPredictor* predictor)
{
    map<uint64_t, unsigned>::iterator it = entry_list.find(entry);
    if (it != entry_list.end()) {
        it->second = 0;
    } else {
        for (map<uint64_t, unsigned>::iterator it = entry_list.begin();
                it != entry_list.end(); ++it) {
            ++it->second;
        }
        entry_list[entry] = 0;
    }
}

void OnChipReplacementLRU::eraseEntry(uint64_t entry)
{
    entry_list.erase(entry);
}

uint64_t OnChipReplacementLRU::pickVictim(IsbHawkeyePCPredictor* predictor)
{
    assert(entry_list.size() != 0);
    unsigned max_ts = 0;
    map<uint64_t, unsigned>::iterator max_it = entry_list.begin();
    for (map<uint64_t, unsigned>::iterator it = entry_list.begin();
            it != entry_list.end(); ++it) {
        if (it->second > max_ts) {
            max_ts = it->second;
            max_it = it;
        }
    }

    assert(max_it != entry_list.end());
    uint64_t addr = max_it->first;
    entry_list.erase(max_it);

    return addr;
}

void OnChipReplacementLFU::addEntry(uint64_t entry, uint64_t pc, int type, bool hit, IsbHawkeyePCPredictor* predictor)
{
    map<uint64_t, unsigned>::iterator it = entry_list.find(entry);
    if (it != entry_list.end()) {
        ++it->second;
    } else {
        entry_list[entry] = 1;
    }
}

void OnChipReplacementLFU::eraseEntry(uint64_t entry)
{
    entry_list.erase(entry);
}

uint64_t OnChipReplacementLFU::pickVictim(IsbHawkeyePCPredictor* predictor)
{
    unsigned min_ts = UINT32_MAX;
    map<uint64_t, unsigned>::iterator min_it = entry_list.begin();
    for (map<uint64_t, unsigned>::iterator it = entry_list.begin();
            it != entry_list.end(); ++it) {
        if (it->second < min_ts) {
            min_ts = it->second;
            min_it = it;
        }
    }

    uint64_t addr = min_it->first;
    entry_list.erase(min_it);

    return addr;
}

OnChipReplacementRRIP::OnChipReplacementRRIP(int max_rrpv)
    : max_rrpv(max_rrpv)
{
}

void OnChipReplacementRRIP::eraseEntry(uint64_t entry)
{
    entry_list.erase(entry);
}

uint64_t OnChipReplacementRRIP::pickVictim(IsbHawkeyePCPredictor* predictor)
{
    uint64_t victim_addr = INVALID_ADDR;
    while (true) {
        for (map<uint64_t, int>::iterator it = entry_list.begin();
            it != entry_list.end(); ++it) {
            if (it->second == max_rrpv) {
                // Pick this
                victim_addr = it->first;
                entry_list.erase(it);
                return victim_addr;
            }
        }

        for (map<uint64_t, int>::iterator it = entry_list.begin();
            it != entry_list.end(); ++it) {
            ++it->second;
            assert(it->second <= max_rrpv);
        }
    }
}

OnChipReplacementSRRIP::OnChipReplacementSRRIP()
    : OnChipReplacementRRIP(3)
{
}

void OnChipReplacementSRRIP::addEntry(uint64_t entry, uint64_t pc, int type, bool hit, IsbHawkeyePCPredictor* predictor)
{
    map<uint64_t, int>::iterator it = entry_list.find(entry);
    if (it != entry_list.end()) {
        // Hit
        it->second = 0;
    } else {
        // Miss
        entry_list[entry] = max_rrpv -1;
    }
}

OnChipReplacementBRRIP::OnChipReplacementBRRIP()
    : OnChipReplacementRRIP(3)
{
    // MAX RRPV
    bip_count = 0;
    bip_max = 32;
}

void OnChipReplacementBRRIP::addEntry(uint64_t entry, uint64_t pc, int type, bool hit, IsbHawkeyePCPredictor* predictor)
{
    map<uint64_t, int>::iterator it = entry_list.find(entry);
    if (it != entry_list.end()) {
        // Hit
        it->second = 0;
    } else {
        // Miss
        // Probabilitstic Update
        if ((++bip_count) == bip_max) {
            bip_count = 0;
            entry_list[entry] = max_rrpv - 1;
        } else {
            entry_list[entry] = max_rrpv;
        }
    }
}

OnChipReplacementDRRIP::OnChipReplacementDRRIP()
{
    // TODO: Implement init
    psel = 0;
    psel_max = 1<<10;
}

uint64_t OnChipReplacementDRRIP::get_set_id(uint64_t addr)
{
    // TODO IMplement DRRIP
//    uint64_t setId = (addr >> offset) & indexMask;
    uint64_t setId = addr;
    return setId;
}

OnChipReplacementDRRIP::SD_state OnChipReplacementDRRIP::get_state(uint64_t addr)
{
    uint64_t set_id = get_set_id(addr);
    if (set_id == 0) {
        return SRRIP;
    } else if (set_id == 1) {
        return BRRIP;
    } else {
        return FOLLOWER;
    }
}

void OnChipReplacementDRRIP::addEntry(uint64_t entry, uint64_t pc, int type, bool hit, IsbHawkeyePCPredictor* predictor)
{
    map<uint64_t, int>::iterator it = entry_list.find(entry);
    if (it != entry_list.end()) {
        // Hit
        it->second = 0;
    } else {
        // Miss
        // Use set dueling to decide rrpv
        SD_state sd_state = get_state(entry);
        switch (sd_state) {
            case SRRIP:
                if (psel < psel_max) ++psel;
                entry_list[entry] = max_rrpv -1;
                break;

            case BRRIP:
                if (psel > -psel_max) --psel;
                if ((++bip_count) == bip_max) {
                    bip_count = 0;
                    entry_list[entry] = max_rrpv - 1;
                } else {
                    entry_list[entry] = max_rrpv;
                }
                break;

            case FOLLOWER:
                if (psel < 0) {
                    // BRRIP
                    if ((++bip_count) == bip_max) {
                        bip_count = 0;
                        entry_list[entry] = max_rrpv - 1;
                    } else {
                        entry_list[entry] = max_rrpv;
                    }
                } else {
                    // SRRIP
                    entry_list[entry] = max_rrpv -1;
                }
                break;
        }
    }
}

OnChipReplacementHawkeye::OnChipReplacementHawkeye(
        uint64_t numsets, uint64_t setID, unsigned assoc, bool update_on_write)
    :   OnChipReplacementRRIP(3),
        update_optgen_on_write(update_on_write)
{
    num_sets = numsets;
    set = setID;
    optgen_addr_history.clear(); 
    optgen.init(assoc-2);
    last_addr = 0;
    
    //if(SAMPLED_SET(setID))
     //   cout << setID << endl;
}

void OnChipReplacementHawkeye::update_optgen(uint64_t addr, uint64_t pc, int type, IsbHawkeyePCPredictor* predictor)
{
    if(SAMPLED_SET(set))
    {
        uint64_t curr_quanta = optgen_mytimer;
        bool opt_hit = false;

        signatures[addr] = pc;
        if(optgen_addr_history.find(addr) != optgen_addr_history.end())
        {
            uint64_t last_quanta = optgen_addr_history[addr].last_quanta;
            //    cout << last_quanta << " " << curr_quanta << endl;
            assert(curr_quanta >= optgen_addr_history[addr].last_quanta);
            uint64_t last_pc = optgen_addr_history[addr].PC;
            opt_hit = optgen.should_cache(curr_quanta, last_quanta, false); //TODO: CPU
            if (opt_hit) {
                predictor->increment(last_pc|(bits(set,6,13)<<24));
            } else {
                predictor->decrement(last_pc|(bits(set,6,13)<<24));
            }
            debug_cout <<  "Train: " << hex << last_pc << " " << dec << opt_hit << endl;
            //Some maintenance operations for OPTgen
            optgen.add_access(curr_quanta); //TODO: CPU
        }
        // This is the first time we are seeing this line
        else
        {
            //Initialize a new entry in the sampler
            optgen_addr_history[addr].init(curr_quanta);
            optgen.add_access(curr_quanta); //TODO: CPU
        }

        optgen_addr_history[addr].update(optgen_mytimer, pc, false);
        optgen_mytimer++;
    }

    bool prediction = predictor->get_prediction(pc|(bits(set,6,13)<<24));

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
        for(map<uint64_t, MetaHawkeyeEntry>::iterator it = entry_list.begin(); it != entry_list.end(); ++it) {
            if (it->second.rrpv >= max_rrpv-1)
                saturated = true;
        }
        //Age all the cache-friendly  lines
        for(map<uint64_t, MetaHawkeyeEntry>::iterator it = entry_list.begin(); it != entry_list.end(); ++it) {
            if (!saturated && it->second.rrpv < max_rrpv-1)
                ++it->second.rrpv;
        }
        entry_list[addr].rrpv = 0;
    } else {
        entry_list[addr].rrpv = max_rrpv;
    }
    debug_cout << "AddEntry after Entry List size: " << entry_list.size() << endl;
}

void OnChipReplacementHawkeye::addEntry(uint64_t entry, uint64_t pc, int type, bool hit, IsbHawkeyePCPredictor* predictor)
{
    //In case we call it twice on same access. Happens on a Load hit -> Invalidate -> Write
    //if(entry == last_addr)
    //    return;
    
    last_addr = entry;

   /* if(type == WRITEBACK)
    {
        uint64_t curr_quanta = optgen_mytimer;
        optgen_addr_history[entry].init(curr_quanta);
        optgen.add_access(curr_quanta, 0); //TODO: CPU
        optgen_addr_history[entry].update(optgen_mytimer, pc, false);
        optgen_mytimer++;
        entry_list[entry].rrpv = 0;
        return;
    }*/

    update_optgen(entry, pc, type, predictor);
    /*
    bool valid = false;
    debug_cout << "AddEntry before Entry List size: " << entry_list.size() << endl;
    if (type == LOAD) {
        map<uint64_t, MetaHawkeyeEntry>::iterator it = entry_list.find(entry);
        valid = it != entry_list.end();
        update_optgen(entry, pc, type);
    } else if (type == WRITEBACK) {
        for(map<uint64_t, MetaHawkeyeEntry>::iterator it = entry_list.begin(); it != entry_list.end(); ++it) {
            if (it->first == entry && !it->second.valid) {
                it->second.valid = true;
            }
        }
        if (update_optgen_on_write) {
            update_optgen(entry, pc, type);
        }
    } else {
        // Shouldn't reach this point!
        assert(0);
    }
*/
}

void OnChipReplacementHawkeye::eraseEntry(uint64_t entry)
{
    entry_list.erase(entry);
    debug_cout << "EraseEntry after Entry List size: " << entry_list.size() << endl;
}

uint64_t OnChipReplacementHawkeye::pickVictim(IsbHawkeyePCPredictor* predictor)
{
    assert(entry_list.size() != 0);
    // XXX This assert needs to be removed
    debug_cout << "PickVictim before Entry List size: " << entry_list.size() << endl;
    for(map<uint64_t, MetaHawkeyeEntry>::iterator it = entry_list.begin(); it != entry_list.end(); ++it) {
        debug_cout << "repl_sp[" << it->first << "] = " << it->second.rrpv
            << endl;
    }
//    assert(entry_list.size() == 8);
    // look for the maxRRPV line
#define HK_NOINVALID
#ifndef HK_NOINVALID
    for(map<uint64_t, MetaHawkeyeEntry>::iterator it = entry_list.begin(); it != entry_list.end(); ++it) {
        if (!it->second.valid) {
            uint64_t addr = it->first;
            entry_list.erase(it);
            return addr;
        }
    }
#endif
    for(map<uint64_t, MetaHawkeyeEntry>::iterator it = entry_list.begin(); it != entry_list.end(); ++it) {
        if (it->second.rrpv == max_rrpv) {
            uint64_t addr = it->first;
            entry_list.erase(it);
            return addr;
        }
    }

    //If we cannot find a cache-averse line, we evict the oldest cache-friendly line
    uint32_t max_rrip = 0;
    uint64_t lru_victim = 0;
    for(map<uint64_t, MetaHawkeyeEntry>::iterator it = entry_list.begin(); it != entry_list.end(); ++it) {
        if (it->second.rrpv >= max_rrip)
        {
            max_rrip = it->second.rrpv;
            lru_victim = it->first;
        }
    }
    assert(entry_list.count(lru_victim));
    entry_list.erase(lru_victim);

    //The predictor is trained negatively on LRU evictions
    if( SAMPLED_SET(set) ) {
        debug_cout << "Detrain: " << hex << signatures[lru_victim] << dec<< endl;
        predictor->decrement(signatures[lru_victim]|(bits(set,6,13)<<24));
    }
    debug_cout << "PickVictim after Entry List size: " << entry_list.size() << endl;
    return lru_victim;
}

OnChipBandwidthConstraint::OnChipBandwidthConstraint()
{
    // XXX Magic number
    tick_interval = 2;
    next_available_tick = 0;
}

bool OnChipBandwidthConstraint::allow_next_access(uint64_t current_tick)
{
    if (current_tick >= next_available_tick) {
        return true;
    } else {
        return false;
    }
}

void OnChipBandwidthConstraint::make_access(uint64_t current_tick)
{
    next_available_tick = current_tick + tick_interval;
}



