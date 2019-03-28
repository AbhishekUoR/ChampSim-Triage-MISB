#ifndef TABLEISB_TRAINING_UNIT_H__
#define TABLEISB_TRAINING_UNIT_H__

#include <stdint.h>
#include <map>

class TableISBConfig;

struct TableISBTrainingUnitEntry {
    uint64_t addr;
    uint64_t timer;
};

class TableISBTrainingUnit {
    // XXX Only support fully associative LRU for now
    // PC->TrainingUnitEntry
    std::map<uint64_t, TableISBTrainingUnitEntry> entry_list;
    uint64_t current_timer;
    uint64_t max_size;

    void evict();

    public:
        TableISBTrainingUnit();
        void set_conf(TableISBConfig* conf);
        bool get_last_addr(uint64_t pc, uint64_t &prev_addr);
        void set_addr(uint64_t pc, uint64_t addr);
};

#endif // TABLEISB_TRAINING_UNIT_H__
