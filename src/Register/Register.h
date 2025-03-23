#ifndef REGISTER_H
#define REGISTER_H

#include <iostream>
#include <unordered_map>
#include <stdint.h>
#include <string>
#include <vector>
#include <any>

namespace SST {
namespace phnsw {

template <typename T>
struct RegTemp {
    size_t size;
    std::string description;
    T reg;

    RegTemp() = default;
    RegTemp(const std::string& desc, const T& value)
        : description(desc), reg(value), size(sizeof(T)) {}
};

struct Register {
    // @description: all reg map
    // @param {size_t} size
    // @param {std::string} description
    // @param {void *} reg_ptr<std::array<uint8_t,  128>, std::array<uint32_t, 10>, uint8_t, uint32_t, uint64_t>
    std::unordered_map<std::string, void*> reg_map;
    
    /* {"reg_name", "description", init value} */
    Register () {
        // Sources
        reg_map["raw1"]        = new RegTemp<std::array<uint8_t,  128>>{"DistCalc", {0}};
        reg_map["raw2"]        = new RegTemp<std::array<uint8_t,  128>>{"DistCalc", {0}};
        reg_map["list"]        = new RegTemp<std::array<uint32_t, 10>>{"LookUp", {0}};
        reg_map["list_index"]  = new RegTemp<std::array<uint32_t, 10>>{"LookUp", {0}};
        reg_map["target"]      = new RegTemp<uint32_t>{"LookUp", 0};
        reg_map["cmp1"]        = new RegTemp<uint32_t>{"CMP", 0};
        reg_map["cmp2"]        = new RegTemp<uint32_t>{"CMP", 0};
        reg_map["dma_addr"]    = new RegTemp<uint64_t>{"DMA start addr", 0};
        reg_map["dma_offset"]  = new RegTemp<uint64_t>{"DMA read length", 0};
        reg_map["num1"]        = new RegTemp<uint8_t>{"ALU", 0};
        reg_map["num2"]        = new RegTemp<uint8_t>{"ALU", 0};
        reg_map["vst_index"] = new RegTemp<uint32_t>{"VISIT", 0};
        reg_map["jmp_addr"]    = new RegTemp<uint64_t>{"JMP", 0};
        reg_map["raw_index"]   = new RegTemp<uint8_t>{"fetch RAW from SPM", 0};
        reg_map["wrm_index"]   = new RegTemp<uint32_t>{"WRM", 0};
        reg_map["index2addr"]  = new RegTemp<uint32_t>{"index2addr", 0};
          // Destinations
        reg_map["dist_res"]       = new RegTemp<uint32_t>{"DistCalc", 0};
        reg_map["look_res_index"] = new RegTemp<uint32_t>{"LookUp", 0};
        reg_map["look_res_dist"]  = new RegTemp<uint32_t>{"LookUp", 0};
        reg_map["cmp_res"]        = new RegTemp<uint8_t>{"CMP", 0};
        reg_map["dma_res"]        = new RegTemp<uint64_t>{"DMA", 0};
        reg_map["alu_res"]        = new RegTemp<uint8_t>{"ALU", 0};
        reg_map["vst_res"]      = new RegTemp<uint8_t>{"VISIT", 0};
        reg_map["raw_res"]        = new RegTemp<std::array<uint8_t, 128>>{"RAW", {0}};
        reg_map["addr"]           = new RegTemp<uint32_t>{"index2addr", 0};
        // Vars
        reg_map["C_dist"]            = new RegTemp<std::array<uint32_t, 60>>{"Candidate Dist", {0}};
        reg_map["C_index"]           = new RegTemp<std::array<uint32_t, 60>>{"Candidate Index", {0}};
        reg_map["C_size"]            = new RegTemp<uint32_t>{"Candidate Size", 0};
        reg_map["W_index"]           = new RegTemp<std::array<uint32_t, 40>>{"Wait Index", {0}};
        reg_map["W_dist"]            = new RegTemp<std::array<uint32_t, 40>>{"Wait Dist", {0}};
        reg_map["W_size"]            = new RegTemp<uint32_t>{"Wait Size", 0};
        reg_map["lowB_index"]        = new RegTemp<uint32_t>{"lower bound index", 0};
        reg_map["lowB_dist"]         = new RegTemp<uint32_t>{"lower bound dist", 0};
        reg_map["current_node"]      = new RegTemp<uint32_t>{"current node", 0};
        reg_map["CN_neighbor_index"] = new RegTemp<uint32_t>{"Current Node NeighborList indexs", {0}};
        reg_map["i"]                 = new RegTemp<uint32_t>{"temp var", 0};
        reg_map["i20"]               = new RegTemp<uint32_t>{"temp var", 0};
        reg_map["dist1"]             = new RegTemp<uint32_t>{"dist1", 0};
        std::cout << "Register init done" << std::endl;
    }

    /**
     * @description: find match register by name and return its reg_ptr and size
     * @param {string& name} name to find
     * @param {size_t& size} size of register to return
     * @return {void *} reg ptr
     */
    void* find_match(const std::string& name, size_t& size) {
        if (reg_map.find(name) == reg_map.end()) {
            throw "Register not found: ";
        }
        size = *(size_t *) (reg_map[name]);
        if (size == sizeof(std::array<uint8_t, 128>)) {
            return &((RegTemp<std::array<uint8_t, 128>> *) (reg_map[name]))->reg;
        } else if (size == sizeof(std::array<uint32_t, 10>)) {
            return &((RegTemp<std::array<uint32_t, 10>> *) (reg_map[name]))->reg;
        } else if (size == sizeof(std::array<uint32_t, 40>)) { // W
            return &((RegTemp<std::array<uint32_t, 40>> *) (reg_map[name]))->reg;
        } else if (size == sizeof(std::array<uint32_t, 60>)) { // C
            return &((RegTemp<std::array<uint32_t, 60>> *) (reg_map[name]))->reg;
        } else if (size == sizeof(uint8_t)) {
            return &((RegTemp<uint8_t> *) (reg_map[name]))->reg;
        } else if (size == sizeof(uint32_t)) {
            return &((RegTemp<uint32_t> *) (reg_map[name]))->reg;
        } else if (size == sizeof(uint64_t)) {
            return &((RegTemp<uint64_t> *) (reg_map[name]))->reg;
        }
        throw "Register type not found";
    }

    size_t find_size(const std::string& name) {
        if (reg_map.find(name) == reg_map.end()) {
            // throw "Register not found: ";
            return 0;
        }
        return *(size_t *) (reg_map[name]);
    }
};

} // namespace phnsw
} // namespace SST

#endif