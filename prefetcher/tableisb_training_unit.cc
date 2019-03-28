
#include <assert.h>
#include "tableisb_training_unit.h"
#include "tableisb.h"

using namespace std;

TableISBTrainingUnit::TableISBTrainingUnit()
{
    current_timer = 0;
}

void TableISBTrainingUnit::set_conf(TableISBConfig* conf)
{
    max_size = conf->training_unit_size;
}

bool TableISBTrainingUnit::get_last_addr(uint64_t pc, uint64_t &prev_addr)
{
    map<uint64_t, TableISBTrainingUnitEntry>::iterator it = entry_list.find(pc);
    if (it != entry_list.end()) {
        prev_addr = it->second.addr;
        return true;
    } else {
        return false;
    }
}

void TableISBTrainingUnit::set_addr(uint64_t pc, uint64_t addr)
{
    map<uint64_t, TableISBTrainingUnitEntry>::iterator it = entry_list.find(pc);
    if (it != entry_list.end()) {
        it->second.addr = addr;
        it->second.timer = current_timer++;
    } else {
        if (entry_list.size() == max_size) {
            evict();
        }
        entry_list[pc].addr = addr;
        entry_list[pc].timer = current_timer++;
    }
    assert(entry_list.size() <= max_size);
}

void TableISBTrainingUnit::evict()
{
    assert(entry_list.size() == max_size);
    map<uint64_t, TableISBTrainingUnitEntry>::iterator it, min_it;
    uint64_t min_timer = current_timer;
    for (it = entry_list.begin(); it != entry_list.end(); ++it) {
        assert(it->second.timer < current_timer);
        if (it->second.timer < min_timer) {
            min_it = it;
            min_timer = it->second.timer;
        }
    }

    entry_list.erase(min_it);
}

