#ifndef __TRIAGE_H__
#define __TRIAGE_H__

#include <iostream>
#include <map>
#include <vector>

#include "inc/champsim.h"
#include "triage_training_unit.h"
#include "triage_onchip.h"


struct TriageConfig {
    // The lookahead of Triage. XXX: Not implemented for lookead > 1
    int lookahead;
    // The degree of Triage. XXX: Must set use_layer_prediction=true for
    // degree>1
    int degree;
    // Degree of prefetch from prefetch queue for each access. Should be set to equivalent to
    // degree.
    int prefetch_queue_degree;

    //
    int on_chip_set, on_chip_assoc, log_on_chip_set;
    int training_unit_size;
    bool use_dynamic_assoc;
    bool use_rap_assoc;
    bool use_sba_assoc;
    bool use_compressed_tag;
    bool use_reeses;
    bool use_layer_prediction;
    bool reeses_footprint;
    bool regular_on_compulsory;
    bool reeses_perfect_trigger;

    double bloom_fprate;
    int bloom_capacity;


    TriageReplType repl;
};

class TriageBase {
    protected:
        uint64_t cpu;
        int lookahead, degree, prefetch_queue_degree;
        bool use_layer_prediction;
        // Stats
        uint64_t same_addr, new_addr, new_stream;
        uint64_t no_next_addr, conf_dec_retain, conf_dec_update, conf_inc;
        uint64_t predict_count, trigger_count;
        uint64_t total_assoc;

        std::map<uint64_t, std::deque<uint64_t>> prefetch_queue;
        virtual void train(uint64_t pc, uint64_t addr, bool hit) = 0;
        virtual void predict(uint64_t pc, uint64_t addr, bool hit);

    public:
        TriageOnchip on_chip_data;

        TriageBase();
        virtual void set_conf(uint64_t cpu, TriageConfig *config);
        void set_rap(RAH* rap) {on_chip_data.set_rap(rap);}
        virtual void print_stats();
        uint32_t get_assoc();
        virtual void calculatePrefetch(uint64_t pc, uint64_t addr,
                bool cache_hit, int max_degree, uint64_t cpu);
        virtual uint64_t getNextPrefetchAddr(uint64_t pc, uint64_t addr);
        bool should_skip_prefetch(int assoc);
};

class Triage : public TriageBase {
    private:
        TriageTrainingUnit training_unit;
    protected:
        virtual void train(uint64_t pc, uint64_t addr, bool hit) override;
    public:
        Triage();
        virtual void set_conf(uint64_t cpu, TriageConfig *config) override;
};

#endif // __TRIAGE_H__
