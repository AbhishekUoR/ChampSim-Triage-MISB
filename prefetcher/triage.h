#ifndef __TRIAGE_H__
#define __TRIAGE_H__

#include <iostream>
#include <map>
#include <vector>

#include "inc/champsim.h"
#include "triage_training_unit.h"
#include "triage_onchip.h"

struct TriageConfig {
    int lookahead;
    int degree;

    int on_chip_set, on_chip_assoc, log_on_chip_set;
    int training_unit_size;
    bool use_dynamic_assoc;
    bool use_compressed_tag;
    bool use_reeses;
    bool use_layer_prediction;
    bool reeses_footprint;

    TriageReplType repl;
};

class TriageBase {
    protected:
        int lookahead, degree;
        bool use_layer_prediction;
        // Stats
        uint64_t same_addr, new_addr, new_stream;
        uint64_t no_next_addr, conf_dec_retain, conf_dec_update, conf_inc;
        uint64_t predict_count, trigger_count;
        uint64_t total_assoc;

        std::vector<uint64_t> next_addr_list;
        virtual void train(uint64_t pc, uint64_t addr, bool hit) = 0;
        virtual void predict(uint64_t pc, uint64_t addr, bool hit);

    public:
        TriageOnchip on_chip_data;

        TriageBase();
        virtual void set_conf(TriageConfig *config);
        virtual void print_stats();
        uint32_t get_assoc();
        virtual void calculatePrefetch(uint64_t pc, uint64_t addr,
                bool cache_hit, uint64_t *prefetch_list,
                int max_degree, uint64_t cpu);
};

class Triage : public TriageBase {
    private:
        TriageTrainingUnit training_unit;
    protected:
        virtual void train(uint64_t pc, uint64_t addr, bool hit) override;
    public:
        Triage();
        virtual void set_conf(TriageConfig *config) override;
};

#endif // __TRIAGE_H__
