
#include <assert.h>

#include "tableisb_onchip.h"
#include "tableisb.h"

using namespace std;

//#define DEBUG

#ifdef DEBUG
#define debug_cout cerr << "[TABLEISB_ONCHIP] "
#else
#define debug_cout if (0) cerr
#endif

TableISBOnchipEntry::TableISBOnchipEntry()
{
    confidence = 3;
}

void TableISBOnchipEntry::increase_confidence()
{
    if (confidence < 3)
        ++confidence;
}

void TableISBOnchipEntry::decrease_confidence()
{
    if (confidence > 0)
        --confidence;
}

TableISBOnchip::TableISBOnchip()
{
}

void TableISBOnchip::set_conf(TableISBConfig *config)
{
    assoc = config->on_chip_assoc;
    num_sets = config->on_chip_size / assoc;
    repl_type = config->repl;
    index_mask = num_sets - 1;

    entry_list.resize(num_sets);
    repl = TableISBRepl::create_repl(&entry_list, repl_type, assoc);
    cout << "Num Sets: " << num_sets << endl;
}

uint64_t TableISBOnchip::get_set_id(uint64_t addr)
{
    uint64_t set_id = (addr>>6) & index_mask;
    debug_cout << "num_sets: " << num_sets << ", index_mask: " << index_mask
        << ", set_id: " << set_id <<endl;
    assert(set_id < num_sets);
    return set_id;
}

int TableISBOnchip::increase_confidence(uint64_t addr)
{
    uint64_t set_id = get_set_id(addr);
    assert(set_id < num_sets);
    map<uint64_t, TableISBOnchipEntry>& entry_map = entry_list[set_id];
    map<uint64_t, TableISBOnchipEntry>::iterator it = entry_map.find(addr);

    it->second.increase_confidence();
    return it->second.confidence;
}

int TableISBOnchip::decrease_confidence(uint64_t addr)
{
    uint64_t set_id = get_set_id(addr);
    assert(set_id < num_sets);
    map<uint64_t, TableISBOnchipEntry>& entry_map = entry_list[set_id];
    map<uint64_t, TableISBOnchipEntry>::iterator it = entry_map.find(addr);

    it->second.decrease_confidence();
    return it->second.confidence;
}

void TableISBOnchip::update(uint64_t prev_addr, uint64_t next_addr, uint64_t pc, bool update_repl)
{
    assoc = repl->get_assoc();
    if (assoc == 0) {
        return;
    }
    uint64_t set_id = get_set_id(prev_addr);
    debug_cout << hex << "update prev_addr: " << prev_addr
        << ", next_addr: " << next_addr
        << ", set_id: " << set_id << endl;
    assert(set_id < num_sets);
    map<uint64_t, TableISBOnchipEntry>& entry_map = entry_list[set_id];
    map<uint64_t, TableISBOnchipEntry>::iterator it = entry_map.find(prev_addr);

    if (it != entry_map.end()) {
        it->second.next_addr = next_addr;
        if(update_repl)
            repl->addEntry(set_id, prev_addr, pc);
    } else {
        while (repl_type != TABLEISB_REPL_PERFECT && entry_map.size() >= assoc) {
//            assert(entry_map.size() == assoc);
            uint64_t victim_addr = repl->pickVictim(set_id);
            entry_map.erase(victim_addr);
        }
        entry_map[prev_addr].next_addr = next_addr;
        entry_map[prev_addr].confidence = 3;
        repl->addEntry(set_id, prev_addr, pc);
    }
}

bool TableISBOnchip::get_next_addr(uint64_t prev_addr, uint64_t &next_addr,
        uint64_t pc, bool update_stats)
{
    assoc = repl->get_assoc();
    if (assoc == 0) {
        return false;
    }
    uint64_t set_id = get_set_id(prev_addr);
    assert(set_id < num_sets);
    map<uint64_t, TableISBOnchipEntry>& entry_map = entry_list[set_id];

    map<uint64_t, TableISBOnchipEntry>::iterator it = entry_map.find(prev_addr);

    if (it != entry_map.end()) {
        next_addr = it->second.next_addr;
        if (update_stats) {
            repl->addEntry(set_id, prev_addr, pc);
        }
        return true;
    } else {
        return false;
    }
}

void TableISBOnchip::print_stats()
{
    assert(repl != NULL);
    repl->print_stats();
}

