#ifndef __TRIAGE_REESES_H__
#define __TRIAGE_REESES_H__

#include "reeses_spatial.h"
#include "reeses_training_unit.h"
#include "triage.h"

class TriageReeses : public TriageBase {
    private:
        uint64_t temporal_update, spatial_update;

        ReesesTrainingUnit tu;
    public:
        TriageReeses();
        void set_conf(TriageConfig *config) override;
        void train(uint64_t pc, uint64_t addr, bool cache_hit) override;
        void predict(uint64_t pc, uint64_t addr, bool cache_hit) override;
        void print_stats() override;
};

#endif // __TRIAGE_REESES_H__

