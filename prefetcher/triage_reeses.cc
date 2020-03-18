
#include "triage_reeses.h"

void TriageReeses::set_conf(TriageConfig *config)
{
    TriageBase::set_conf(config);
    tu.FOOTPRINT = config->reeses_footprint;
}

/* This is the main training function for Reeses,
 * which is called on every cache miss. I've removed all the extraneous
 * functions for simplicity's sake, so that you can get a general idea
 * of how to use the training unit.
 */
void TriageReeses::train(uint64_t cur_pc, uint64_t addr, bool cache_hit) {
    // get new correlated pair from training unit
    TUEntry *result = tu.update(cur_pc, addr);
    if (result != nullptr) {
        uint64_t trigger = result->temporal;
        if (!result->has_spatial) {
            // correct ordering for temporal entries
            result->temporal = addr;
            //temporal_counts[cur_pc] += 1;
        }

        // link end of spatials to next temporal
        if (!tu.FOOTPRINT && result->has_spatial) {
            uint64_t last_addr = result->spatial->last_address();
            TUEntry *link = new TUEntry(addr);
            // TODO: Implement on-chip for Triage Reeses
            on_chip_info->train(last_addr, link);
        }
        delete result;
    }
}

void TriageReeses::predict(uint64_t pc, uint64_t addr, bool cache_hit)
{
    // TODO: implement
#if 0
    uint64_t next_addr;
    bool next_addr_exist = on_chip_data.get_next_addr(addr, next_addr, pc, false);
    if (next_addr_exist) {
        debug_cout << hex << "Predict: " << addr << " " << next_addr << dec << endl;
        ++predict_count;
        next_addr_list.push_back(next_addr);
        assert(next_addr != addr);
    }
#endif
}


