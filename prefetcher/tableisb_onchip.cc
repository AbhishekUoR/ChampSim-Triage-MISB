
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
    init();
}

void TableISBOnchipEntry::init()
{
    for (unsigned i = 0; i < ONCHIP_LINE_SIZE; ++i) {
        next_addr[i] = INVALID_ADDR;
        confidence[i] = 3;
        valid[i] = false;
    }
}

void TableISBOnchipEntry::increase_confidence(unsigned offset)
{
    if (confidence[offset] < 3)
        ++confidence[offset];
}

void TableISBOnchipEntry::decrease_confidence(unsigned offset)
{
    if (confidence[offset] > 0)
        --confidence[offset];
}

TableISBOnchip::TableISBOnchip()
{
}

void TableISBOnchip::set_conf(TableISBConfig *config)
{
    assoc = config->on_chip_assoc;
    num_sets = config->on_chip_size / assoc;
    num_sets = num_sets >> ONCHIP_LINE_SHIFT;
    repl_type = config->repl;
    index_mask = num_sets - 1;

    entry_list.resize(num_sets);
    repl = TableISBRepl::create_repl(&entry_list, repl_type, assoc);
    cout << "Num Sets: " << num_sets << endl;
}

uint64_t TableISBOnchip::get_line_offset(uint64_t addr)
{
    uint64_t line_offset = (addr>>6) & (ONCHIP_LINE_SIZE-1);
    return line_offset;
}


uint64_t TableISBOnchip::get_set_id(uint64_t addr)
{
    uint64_t set_id = (addr>>6>>ONCHIP_LINE_SHIFT) & index_mask;
    debug_cout << "num_sets: " << num_sets << ", index_mask: " << index_mask
        << ", set_id: " << set_id <<endl;
    assert(set_id < num_sets);
    return set_id;
}

int TableISBOnchip::increase_confidence(uint64_t addr)
{
    uint64_t set_id = get_set_id(addr);
    assert(set_id < num_sets);
    uint64_t line_offset = get_line_offset(addr);
    uint64_t tag = addr >> 6 >> ONCHIP_LINE_SHIFT;
    map<uint64_t, TableISBOnchipEntry>& entry_map = entry_list[set_id];
    map<uint64_t, TableISBOnchipEntry>::iterator it = entry_map.find(tag);

    it->second.increase_confidence(line_offset);
    return it->second.confidence[line_offset];
}

int TableISBOnchip::decrease_confidence(uint64_t addr)
{
    uint64_t set_id = get_set_id(addr);
    assert(set_id < num_sets);
    uint64_t line_offset = get_line_offset(addr);
    uint64_t tag = addr >> 6 >> ONCHIP_LINE_SHIFT;
    map<uint64_t, TableISBOnchipEntry>& entry_map = entry_list[set_id];
    map<uint64_t, TableISBOnchipEntry>::iterator it = entry_map.find(tag);

    it->second.decrease_confidence(line_offset);
    return it->second.confidence[line_offset];
}

void TableISBOnchip::update(uint64_t prev_addr, uint64_t next_addr, uint64_t pc, bool update_repl)
{
    uint64_t set_id = get_set_id(prev_addr);
    debug_cout << hex << "update prev_addr: " << prev_addr
        << ", next_addr: " << next_addr
        << ", set_id: " << set_id << endl;
    assert(set_id < num_sets);
    uint64_t line_offset = get_line_offset(prev_addr);
    uint64_t tag = prev_addr >> 6 >> ONCHIP_LINE_SHIFT;
    map<uint64_t, TableISBOnchipEntry>& entry_map = entry_list[set_id];
    map<uint64_t, TableISBOnchipEntry>::iterator it = entry_map.find(tag);

    if (it != entry_map.end()) {
        it->second.next_addr[line_offset] = next_addr;
        it->second.valid[line_offset] = true;
        if(update_repl)
            repl->addEntry(set_id, tag, pc);
    } else {
        if (repl_type != TABLEISB_REPL_PERFECT && entry_map.size() >= assoc) {
            assert(entry_map.size() == assoc);
            uint64_t victim_addr = repl->pickVictim(set_id);
            entry_map.erase(victim_addr);
        }
        assert(!entry_map.count(tag));
        entry_map[tag].init();
        entry_map[tag].next_addr[line_offset] = next_addr;
        entry_map[tag].confidence[line_offset] = 3;
        entry_map[tag].valid[line_offset] = true;
        repl->addEntry(set_id, tag, pc);
    }
}

bool TableISBOnchip::get_next_addr(uint64_t prev_addr, uint64_t &next_addr,
        uint64_t pc, bool update_stats)
{
    uint64_t set_id = get_set_id(prev_addr);
    assert(set_id < num_sets);
    uint64_t line_offset = get_line_offset(prev_addr);
    uint64_t tag = prev_addr >> 6 >> ONCHIP_LINE_SHIFT;
    map<uint64_t, TableISBOnchipEntry>& entry_map = entry_list[set_id];

    map<uint64_t, TableISBOnchipEntry>::iterator it = entry_map.find(tag);

    if (it != entry_map.end() && (it->second.valid[line_offset])) {
        next_addr = it->second.next_addr[line_offset];
        if (update_stats) {
            repl->addEntry(set_id, tag, pc);
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

