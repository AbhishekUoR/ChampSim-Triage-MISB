
#include <assert.h>
#include <iostream>

#include "triage.h"

using namespace std;

//#define DEBUG

#ifdef DEBUG
#define debug_cout cerr << "[TRIAGE] "
#else
#define debug_cout if (0) cerr
#endif


TriageBase::TriageBase()
{
    trigger_count = 0;
    predict_count = 0;
    same_addr = 0;
    new_addr = 0;
    no_next_addr = 0;
    conf_dec_retain = 0;
    conf_dec_update = 0;
    conf_inc = 0;
    new_stream = 0;
    total_assoc = 0;
}

void TriageBase::set_conf(TriageConfig *config)
{
    assert(config != NULL);
    lookahead = config->lookahead;
    degree = config->degree;
    prefetch_queue_degree = config->prefetch_queue_degree;
    use_layer_prediction = config->use_layer_prediction;
    assert(prefetch_queue_degree <= degree);

    on_chip_data.set_conf(config);
}

uint32_t TriageBase::get_assoc()
{
    return on_chip_data.get_assoc();
}

void TriageBase::print_stats()
{
    cout << dec << "trigger_count=" << trigger_count <<endl;
    cout << "predict_count=" << predict_count <<endl;
    cout << "same_addr=" << same_addr <<endl;
    cout << "new_addr=" << new_addr <<endl;
    cout << "new_stream=" << new_stream <<endl;
    cout << "no_next_addr=" << no_next_addr <<endl;
    cout << "conf_dec_retain=" << conf_dec_retain <<endl;
    cout << "conf_dec_update=" << conf_dec_update <<endl;
    cout << "conf_inc=" << conf_inc <<endl;
    cout << "total_assoc=" << total_assoc <<endl;

    on_chip_data.print_stats();
}

void TriageBase::calculatePrefetch(uint64_t pc, uint64_t addr,
    bool cache_hit, int max_degree, uint64_t cpu)
{
    // XXX Only allow lookahead = 1 for now
    assert(lookahead == 1);
    assert(degree <= max_degree);

    addr = addr >> 6;
    
    if (pc == 0) return; //TODO: think on how to handle prefetches from lower level

    debug_cout << hex << "Trigger: pc: " << pc << ", addr: "
        << addr << dec << " " << cache_hit << endl;

    ++trigger_count;
    total_assoc+=get_assoc();

    // Predict
    predict(pc, addr, cache_hit);

    // Train
    train(pc, addr, cache_hit);

    /*
    for (size_t i = 0; i < prefetch_queue_degree ; ++i) {
        if (prefetch_queue[pc].empty())
            break;
        prefetch_list[i] = prefetch_queue[pc].front() << 6;
        prefetch_queue[pc].pop_front();
    }*/
}

uint64_t TriageBase::getNextPrefetchAddr(uint64_t pc, uint64_t addr)
{
    uint64_t pref_addr = 0;
    if (prefetch_queue[pc].empty())
        return 0;
    pref_addr = prefetch_queue[pc].front() << 6;
    prefetch_queue[pc].pop_front();
    return pref_addr;
}

Triage::Triage()
{
}

void Triage::set_conf(TriageConfig *config)
{
    TriageBase::set_conf(config);
    training_unit.set_conf(config);
}

void Triage::train(uint64_t pc, uint64_t addr, bool cache_hit)
{
    if(cache_hit) {
        training_unit.set_addr(pc, addr);
        return;
    }
    uint64_t prev_addr;
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

            vector<uint64_t> predict_list = on_chip_data.get_next_addr(prev_addr, pc, false);
            if (predict_list.empty()) {
                on_chip_data.update(prev_addr, addr, pc, true, nullptr);
                ++no_next_addr;
            } else if (find(predict_list.begin(), predict_list.end(), addr) == predict_list.end()) {
                int conf = on_chip_data.decrease_confidence(prev_addr);
                ++conf_dec_retain;
                if (conf == 0) {
                    ++conf_dec_update;
                    on_chip_data.update(prev_addr, addr, pc, false, nullptr);
                }
            } else {
                on_chip_data.increase_confidence(prev_addr);
                ++conf_inc;
            }
        }
    } else {
        // New Stream
        debug_cout << hex << "StreamHead: " << addr <<endl;
        ++new_stream;
    }

    training_unit.set_addr(pc, addr);
}

void TriageBase::predict(uint64_t pc, uint64_t addr, bool cache_hit)
{
    uint64_t next_addr;
    vector<uint64_t> temp_list;
    deque<uint64_t> predict_list;
    predict_list.push_back(addr);
    while (prefetch_queue[pc].size() < degree && !predict_list.empty()) {
        addr = predict_list.front();
        predict_list.pop_front();
        temp_list = on_chip_data.get_next_addr(addr, pc, false);
        for (uint64_t next_addr : temp_list) {
            debug_cout << hex << "Predict: " << addr << " " << next_addr << dec << endl;
            prefetch_queue[pc].push_back(next_addr);
            ++predict_count;
            if (use_layer_prediction)
                predict_list.push_back(next_addr);
        }
    }
}


