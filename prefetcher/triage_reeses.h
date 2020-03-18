#ifndef __TRIAGE_REESES_H__
#define __TRIAGE_REESES_H__

#include "reeses_spatial.h"
#include "reeses_training_unit.h"
#include "triage.h"

class TriageReeses : public TriageBase {
    private:
        ReesesTrainingUnit tu;
    public:
        void set_conf(TriageConfig *config) override;
        void train(uint64_t pc, uint64_t addr, bool cache_hit) override;
        void predict(uint64_t pc, uint64_t addr, bool cache_hit) override;
};

#endif // __TRIAGE_REESES_H__

