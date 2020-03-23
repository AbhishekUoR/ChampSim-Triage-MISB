#ifndef REESES_TRAINING_UNIT_H
#define REESES_TRAINING_UNIT_H

#include <map>

#include "reeses_spatial.h"

#define REESES_MAX_CONF 3
#define INIT_CONF 3

#define NO_SPATIAL false

struct TUEntry {
    uint64_t temporal;
    uint32_t conf;
    bool has_spatial;
    SpatialPattern *spatial;

    TUEntry *clone() const {
        TUEntry *result = new TUEntry();
        result->temporal = temporal;
        result->conf = conf;
        result->has_spatial = has_spatial;
        if (spatial != nullptr) {
            result->spatial = spatial->clone();
        }
        return result;
    }

    void inc() {
        if (conf != REESES_MAX_CONF)
            conf++;
    }

    bool dec() {
        if (conf != 0)
            conf--;
        return !conf;
    }

    bool operator==(const TUEntry &other) const {
        if (other.has_spatial) {
            return has_spatial && (*spatial == *(other.spatial));
        } else {
            return !has_spatial && temporal == other.temporal;
        }
    }
        
    ~TUEntry() {
        if (spatial != nullptr)
            delete spatial;
    }

    TUEntry() :
        temporal(0), conf(INIT_CONF), has_spatial(false), spatial(nullptr) {}
    TUEntry(uint64_t addr) :
        temporal(addr), conf(INIT_CONF), has_spatial(false), spatial(nullptr) {}
    TUEntry(const TUEntry& entry) {
        temporal = entry.temporal;
        conf = entry.conf;
        has_spatial = entry.has_spatial;
        spatial = entry.spatial->clone();
    }
};

/* similar to ISB's training unit
 * keeps tracker of past accesses by PC to create correlations */
struct ReesesTrainingUnit {
    // stores mapping between each PC and its history
    std::map<uint64_t, TUEntry*> data;
    bool FOOTPRINT;

    ReesesTrainingUnit(bool f = false) :
        FOOTPRINT(f) {}

    uint64_t last_address(uint64_t cur_pc) {
        if (data.find(cur_pc) == data.end()) {
            return 0;
        } else {
            if (data[cur_pc]->has_spatial) {
                return data[cur_pc]->spatial->last_address();
            } else {
                return data[cur_pc]->temporal;
            }
        }
    }

    /* updates the PC's history and returns the old history, if evicted */
    TUEntry *update(uint64_t cur_pc, uint64_t addr_B) {
        TUEntry *result = nullptr;
        if (data.find(cur_pc) == data.end()) {
            // this is a new PC
            data[cur_pc] = new TUEntry(addr_B);
        } else if (data[cur_pc]->has_spatial) {
            // existing spatial pattern
            SpatialPattern *existing = data[cur_pc]->spatial;
            if (existing->last_address() == addr_B)
                return nullptr;

            if (existing->matches(addr_B)) {
                // new addr matches old pattern
                existing->add(addr_B);
            } else {
                // new addr doesn't match old pattern
                result = data[cur_pc];
                data[cur_pc] = new TUEntry(addr_B);
            }
        } else {
            // no existing spatial pattern
            uint64_t last_addr = data[cur_pc]->temporal;
            if (last_addr == addr_B)
                return nullptr;

            uint64_t prev_reg = last_addr >> LOG2_REGION_SIZE;
            uint64_t new_reg = addr_B >> LOG2_REGION_SIZE;
            int32_t delta = addr_B-last_addr;

            if (!NO_SPATIAL && FOOTPRINT && prev_reg == new_reg) {
                // creating a new Footprint
                data[cur_pc]->spatial = new Footprint(last_addr);
                data[cur_pc]->spatial->add(addr_B);
                data[cur_pc]->has_spatial = true;
            } else if (!NO_SPATIAL && !FOOTPRINT && delta >= -REGION_SIZE && delta < REGION_SIZE) {
                // creating a new delta pattern
                data[cur_pc]->spatial = new DeltaPattern(delta, addr_B);
                data[cur_pc]->has_spatial = true;
            } else {
                // kicking out old temporal
                result = data[cur_pc];
                data[cur_pc] = new TUEntry(addr_B);
            }
        }

        return result;
    }
};

#endif
