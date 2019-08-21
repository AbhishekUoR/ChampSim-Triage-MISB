#ifndef __MEM_CACHE_PREFETCH_ISB_ONCHIP_H__
#define __MEM_CACHE_PREFETCH_ISB_ONCHIP_H__

#include <cassert>
#include <map>
#include <set>
#include <vector>
#include "isb_offchip.h"
#include "optgen_simple.h"
#include "isb_hawkeye_predictor.h"
#include "cache.h"
//#include "replacement/hawkeye_config.h"
//#include "replacement/hawkeye_predictor.h"
//#define COUNT_STREAM_DETAIL
//#define MAX_OCI_FILLER 12800

#define PS_METADATA_LINE_SIZE 1
#define SP_METADATA_LINE_SIZE 32
#define PS_METADATA_LINE_SHIFT 0
#define SP_METADATA_LINE_SHIFT 5
class OffChipInfo;
class IsbPrefetcher;

class ISBOnChipPref
{
    OffChipInfo *off_chip_info;
    unsigned degree;

    public:
        uint64_t metapref_success, metapref_stream_end, metapref_not_found;

        void init(OffChipInfo *off_chip_info, int metapref_degree);
        std::vector<uint64_t> getNextAddress(uint64_t pc, uint64_t paddr,
                uint32_t saddr);
};

class OnChipBandwidthConstraint
{
    uint64_t tick_interval;
    uint64_t next_available_tick;

    public:
        OnChipBandwidthConstraint();
        bool allow_next_access(uint64_t current_tick);
        void make_access(uint64_t current_tick);
};

class OnChip_Replacement
{
    public:
//        virtual void setSize(size_t size) = 0;
        virtual uint64_t pickVictim(IsbHawkeyePCPredictor* predictor) = 0;
        // Use  CACHE ACCESS TYPE in memory_class.h
        virtual void addEntry(uint64_t entry, uint64_t pc, int type, bool hit, IsbHawkeyePCPredictor* predictor) = 0;
        virtual void eraseEntry(uint64_t entry) = 0;
};

class OnChipReplacementLRU : public OnChip_Replacement
{
    std::map<uint64_t, unsigned> entry_list;
    public:
        uint64_t pickVictim(IsbHawkeyePCPredictor*);
        void addEntry(uint64_t entry, uint64_t pc, int type, bool hit, IsbHawkeyePCPredictor*);
        void eraseEntry(uint64_t entry);
};

class OnChipReplacementLFU : public OnChip_Replacement
{
    std::map<uint64_t, unsigned> entry_list;
    public:
        uint64_t pickVictim(IsbHawkeyePCPredictor*);
        void addEntry(uint64_t entry, uint64_t pc, int type, bool hit, IsbHawkeyePCPredictor*);
        void eraseEntry(uint64_t entry);
};

class OnChipReplacementRRIP : public OnChip_Replacement
{
    protected:
        // entry->rrpv
        std::map<uint64_t, int> entry_list;
        int max_rrpv;
    public:
        OnChipReplacementRRIP(int max_rrpv);
        virtual uint64_t pickVictim(IsbHawkeyePCPredictor*);
        virtual void eraseEntry(uint64_t entry);
};

class OnChipReplacementSRRIP : public OnChipReplacementRRIP
{
    public:
        OnChipReplacementSRRIP();
        void addEntry(uint64_t entry, uint64_t pc, int type, bool hit, IsbHawkeyePCPredictor*);
};

class OnChipReplacementBRRIP : public OnChipReplacementRRIP
{
    protected:
        // entry->rrpv
        int bip_count, bip_max;
    public:
        OnChipReplacementBRRIP();
        void addEntry(uint64_t entry, uint64_t pc, int type, bool hit, IsbHawkeyePCPredictor*);
};

class OnChipReplacementDRRIP : public OnChipReplacementBRRIP
{
    enum SD_state {
        SRRIP,
        BRRIP,
        FOLLOWER
    };
    uint64_t get_set_id(uint64_t addr);
    SD_state get_state(uint64_t addr);
    int psel, psel_max;
    public:
        OnChipReplacementDRRIP();
        void addEntry(uint64_t entry, uint64_t pc, int type, bool hit, IsbHawkeyePCPredictor*);
};

struct MetaHawkeyeEntry
{
    int rrpv;
    bool valid;
};

class OnChipReplacementHawkeye : public OnChipReplacementRRIP
{
    // addr->rrpv, -1 means does not exist
    bool update_optgen_on_write;
    std::map<uint64_t, MetaHawkeyeEntry> entry_list;
    uint64_t optgen_mytimer;
    OPTgen optgen;
    map<uint64_t, ADDR_INFO> optgen_addr_history;
    int region_shift;
    map<uint64_t, uint64_t> signatures;
    uint64_t last_addr;
    std::map<uint64_t, uint32_t> hawkeye_pc_ps_hit_predictions;
    std::map<uint64_t, uint32_t> hawkeye_pc_ps_total_predictions;
    uint64_t set;
    uint64_t num_sets;

    public:
        OnChipReplacementHawkeye(uint64_t numsets, uint64_t setId, unsigned assoc, bool update_on_write);
        uint64_t pickVictim(IsbHawkeyePCPredictor*);
        void update_optgen(uint64_t addr, uint64_t pc, int type, IsbHawkeyePCPredictor*);
        void addEntry(uint64_t entry, uint64_t pc, int type, bool hit, IsbHawkeyePCPredictor*);
        void eraseEntry(uint64_t entry);
        OPTgen* get_optgen() { return &optgen; }
        size_t get_entry_list_size() { return entry_list.size(); }
};

class OnChip_PS_Entry
{
  public:
    uint32_t str_addr[PS_METADATA_LINE_SIZE];
    bool valid[PS_METADATA_LINE_SIZE];
    unsigned int confidence[PS_METADATA_LINE_SIZE];
    bool tlb_resident;
    bool dirty;
    uint64_t last_access;

    OnChip_PS_Entry() {
        reset();
    }

    void reset(){
        for(unsigned int i=0; i<PS_METADATA_LINE_SIZE; i++)
        {
            valid[i] = false;
            str_addr[i] = 0;
            confidence[i] = 0;
        }
        tlb_resident = true;
        last_access = 0;
    }

    void reset(unsigned int offset)
    {
        valid[offset] = false;
        str_addr[offset] = 0;
        confidence[offset] = 0;
    }

    void set(uint32_t addr, unsigned int offset){
        //if (!cached)
        //    return;

        reset(offset);
        str_addr[offset] = addr;
        valid[offset] = true;
        confidence[offset] = 3;
    }
    void increase_confidence(unsigned int offset){
        assert(valid[offset]);
        confidence[offset] = (confidence[offset] == 3) ? confidence[offset] : (confidence[offset]+1);
    }
    bool lower_confidence(unsigned int offset){
        assert(valid[offset]);
        confidence[offset] = (confidence[offset] == 0) ? confidence[offset] : (confidence[offset]-1);
        return confidence[offset];
    }
    void mark_tlb_resident()
    {
       tlb_resident = true;
    }
    void mark_tlb_evicted()
    {
       tlb_resident = false;
    }

};

class OnChip_SP_Entry
{
  public:
    uint64_t phy_addr[SP_METADATA_LINE_SIZE];
    bool valid[SP_METADATA_LINE_SIZE];
    bool tlb_resident;
    uint64_t last_access;

    void reset(){   
        for(unsigned int i=0; i<SP_METADATA_LINE_SIZE; i++)
        {
            valid[i] = false;
            phy_addr[i] = 0;
        }
        tlb_resident = true;
        last_access = 0;
    }

    void reset(unsigned int offset)
    {
        valid[offset] = false;
        phy_addr[offset] = 0;
    }

    void set(uint64_t addr, unsigned int offset){
    //    if (!cached)
    //        return;
        reset(offset);
        phy_addr[offset] = addr;
        valid[offset] = true;
    }
    void mark_tlb_resident()
    {
       tlb_resident = true;
    }
    void mark_tlb_evicted()
    {
       tlb_resident = false;
    }
};

class OnChipInfo
{
    OffChipInfo *off_chip_info;
    IsbPrefetcher *pref;
    uint64_t off_chip_latency;
    uint64_t curr_timestamp;
    unsigned int num_ps_sets;
    unsigned int num_sp_sets;
    unsigned int ps_indexMask; 
    unsigned int sp_indexMask; 

    unsigned amc_size, amc_assoc;
    unsigned regionsize, log_regionsize, log_cacheblocksize;

    unsigned filler_count;

    std::vector<uint64_t> ps_optgen_mytimer, sp_optgen_mytimer;
    std::vector<OPTgen> ps_optgen, sp_optgen;
    std::vector<map<uint64_t, ADDR_INFO>> ps_optgen_addr_history, sp_optgen_addr_history;
    uint64_t ps_optgen_index_mask, sp_optgen_index_mask;
    IsbHawkeyePCPredictor ps_predictor;
    IsbHawkeyePCPredictor sp_predictor;
//    coroutine filler[MAX_OCI_FILLER];

   public:
    bool ideal_off_chip_transaction;
    bool off_chip_writeback;
    bool count_off_chip_write_traffic;

    std::vector < std::map<uint64_t,OnChip_PS_Entry*> > ps_amc;
    std::vector < std::map<uint32_t,OnChip_SP_Entry*> > sp_amc;
#ifdef COUNT_STREAM_DETAIL
    std::set<uint32_t> active_stream_set;
#endif
    std::map<uint64_t, uint32_t> optgen_pc_ps_hits;
    std::map<uint64_t, uint32_t> optgen_pc_ps_misses;
    std::map<uint64_t, uint32_t> optgen_pc_ps_total;

    OnChipInfo(const pf_isb_conf_t*, OffChipInfo* off_chip_info, IsbPrefetcher* pref);
    void set_conf(const pf_isb_conf_t*, IsbPrefetcher*);
    void reset();
    bool get_structural_address(uint64_t phy_addr, uint32_t& str_addr, bool update_stats, uint64_t pc, bool clear_dirty = false);
    bool get_structural_address_optimal(uint64_t pc, uint64_t phy_addr, uint32_t& str_addr, bool update_stats, bool clear_dirty = false);
    bool get_physical_address(uint64_t& phy_addr, uint32_t str_addr, bool update_stats, uint64_t pc);
    bool get_physical_address_optimal(uint64_t& phy_addr, uint32_t str_addr, bool update_stats);
    void update(uint64_t phy_addr, uint32_t str_addr, bool set_dirty, uint64_t pc);
    void invalidate(uint64_t phy_addr, uint32_t str_addr);
    void increase_confidence(uint64_t phy_addr);
    bool lower_confidence(uint64_t phy_addr);
    bool exists_off_chip(uint64_t);
    void print();
    void mark_tlb_resident(uint64_t addr);
    void mark_not_tlb_resident(uint64_t addr);

#ifdef COUNT_STREAM_DETAIL
    bool is_trigger_access(uint32_t str_addr);
#endif

//    void fetch_bulk(uint64_t phy_addr, off_chip_req_type_t req_type);

    void doPrefetch(uint64_t phy_addr, uint32_t str_addr, bool is_second, uint64_t pc);
//    void doPrefetchBulk(uint64_t phy_addr, uint32_t str_addr, bool is_second);

    void update_phy_region(uint64_t phy_addr);
    void update_str_region(uint32_t str_addr);
    void write_off_chip_region(uint64_t phy_addr, uint32_t str_addr, off_chip_req_type_t req_type);

//    void oci_filler_impl(control_t* data);
    void access_off_chip(uint64_t phy_addr, uint32_t str_addr, off_chip_req_type_t req_type);

    isb_repl_type_t repl_policy;
    ISBOnChipPref oci_pref;
    bool check_bandwidth;
    OnChipBandwidthConstraint bandwidth_constraint;
    std::vector<OnChip_Replacement *> repl_ps, repl_sp;

    uint64_t ps_accesses, ps_hits, ps_prefetch_hits, ps_prefetch_count;
    uint64_t sp_accesses, sp_hits, sp_prefetch_hits, sp_prefetch_count;
    uint64_t bulk_actual;
    uint64_t update_count;
    uint64_t sp_not_found, sp_invalid, metapref_count;
    uint64_t metapref_conflict, metapref_duplicate, metapref_actual;

    uint64_t filler_full_count;
    uint64_t filler_same_addr;
    uint64_t filler_load_count;
    uint64_t filler_load_ps_count;
    uint64_t filler_load_sp1_count;
    uint64_t filler_load_sp2_count;
    uint64_t filler_store_count;


    uint64_t bandwidth_delay_cycles;
    uint64_t issue_delay_cycles;

    void evict_ps(unsigned);
    void evict_sp(unsigned);
    void evict_ps_lru(unsigned);
    void evict_sp_lru(unsigned);
    void evict_ps_tlbsync(unsigned int);
    void evict_sp_tlbsync(unsigned int);

    size_t get_sp_size();
    size_t get_ps_size();
    void insert_metadata_ps(uint64_t phy_addr);
    void insert_metadata_sp(uint32_t str_addr);
};

#endif // __MEM_CACHE_PREFETCH_ISB_ONCHIP_H

