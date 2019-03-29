
#include <assert.h>
#include <iostream>

#include "tableisb.h"

using namespace std;

//#define DEBUG

#ifdef DEBUG
#define debug_cout cerr << "[TABLEISB] "
#else
#define debug_cout if (0) cerr
#endif


TableISB::TableISB()
{
    trigger_count = 0;
    predict_count = 0;
    same_addr = 0;
    new_addr = 0;
    new_stream = 0;
}

void TableISB::set_conf(TableISBConfig *config)
{
    assert(config != NULL);
    lookahead = config->lookahead;
    degree = config->degree;

    training_unit.set_conf(config);
    on_chip_data.set_conf(config);
}

void TableISB::train(uint64_t pc, uint64_t addr, bool cache_hit)
{
    if(cache_hit) {
        training_unit.set_addr(pc, addr);
        return;
    }
    uint64_t prev_addr, next_addr;
    bool prev_addr_exist = training_unit.get_last_addr(pc, prev_addr);
    if (prev_addr_exist) {
        if (prev_addr == addr) {
            // Same Addr
            debug_cout << hex << "Same Addr: " << prev_addr << ", " << addr <<endl;
            ++same_addr;
        } else {
            // New Addr
            debug_cout << hex << "New Addr: " << prev_addr << ", " << addr <<endl;
            ++new_addr;

            bool next_addr_exists = on_chip_data.get_next_addr(prev_addr, next_addr, pc, false);
            if (!next_addr_exists) {
                on_chip_data.update(prev_addr, addr, pc, true);
            } else if (next_addr != addr) {
                int conf = on_chip_data.decrease_confidence(prev_addr);
                if (conf == 0) {
                    on_chip_data.update(prev_addr, addr, pc, false);
                }
            } else {
                on_chip_data.increase_confidence(prev_addr);
            }
        }
    } else {
        // New Stream
        debug_cout << hex << "StreamHead: " << addr <<endl;
        ++new_stream;
    }

    training_unit.set_addr(pc, addr);
}

void TableISB::predict(uint64_t pc, uint64_t addr, bool cache_hit)
{
    uint64_t next_addr;
    bool next_addr_exist = on_chip_data.get_next_addr(addr, next_addr, pc, false);
    if (next_addr_exist) {
        debug_cout << hex << "Predict: " << addr << " " << next_addr << dec << endl;
        ++predict_count;
        next_addr_list.push_back(next_addr);
        assert(next_addr != addr);
    }
}

void TableISB::calculatePrefetch(uint64_t pc, uint64_t addr,
    bool cache_hit, uint64_t *prefetch_list, int max_degree)
{
    // XXX Only allow lookahead = 1 and degree=1 for now
    assert(lookahead == 1);
    assert(degree == 1);

    assert(degree <= max_degree);
    
    if (pc == 0) return; //TODO: think on how to handle prefetches from lower level

    debug_cout << hex << "Trigger: pc: " << pc << ", addr: "
        << addr << dec << " " << cache_hit << endl;

    next_addr_list.clear();
    ++trigger_count;

    // Predict
    predict(pc, addr, cache_hit);

    // Train
    train(pc, addr, cache_hit);

    for (size_t i = 0; i < degree && i < next_addr_list.size(); ++i) {
        prefetch_list[i] = next_addr_list[i];
    }
}

void TableISB::print_stats()
{
    cout << dec << "trigger_count=" << trigger_count <<endl;
    cout << "predict_count=" << predict_count <<endl;
    cout << "same_addr=" << same_addr <<endl;
    cout << "new_addr=" << new_addr <<endl;
    cout << "new_stream=" << new_stream <<endl;

    on_chip_data.print_stats();
}

