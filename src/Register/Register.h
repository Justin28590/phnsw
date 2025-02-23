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
    std::string description;
    T reg;
};

struct Register {
    std::unordered_map<std::string, RegTemp<std::array<uint8_t, 128>>> raw_array;
    std::unordered_map<std::string, RegTemp<std::array<uint32_t, 10>>> list_array;
    std::unordered_map<std::string, RegTemp<uint8_t>> r8;
    std::unordered_map<std::string, RegTemp<uint32_t>> r32;
    std::unordered_map<std::string, RegTemp<uint64_t>> r64;
    std::unordered_map<std::string, RegTemp<__uint128_t>> r128;
    
    /* {"reg_name", "description", init value} */
    Register () {
        // Sources
        raw_array["raw1"] = {"DistCalc", {0}};
        raw_array["raw2"] = {"DistCalc", {0}};
        list_array["list"] = {"LookUp", {0}};
        r32["list_index"] = {"LookUp", 0};
        r32["target"] = {"LookUp", 0};
        r32["cmp1"] = {"CMP", 0};
        r32["cmp2"] = {"CMP", 0};
        r64["dma_addr"] = {"DMA start addr", 0};
        r64["dma_offset"] = {"DMA read length", 0};
        r8["num1"] = {"ALU", 0};
        r8["num2"] = {"ALU", 0};
        r32["visit_index"] = {"VISIT", 0};
        r64["jmp_addr"] = {"JMP", 0};
        r8["raw_index"] = {"fetch RAW from SPM", 0};
        r32["wrm_index"] = {"WRM", 0};
        r32["index2addr"] = {"index2addr", 0};
        // Destinations
        r32["distance_res"] = {"DistCalc", 0};
        r32["look_res_index"] = {"LookUp", 0};
        r32["look_res_distance"] = {"LookUp", 0};
        r8["cmp_res"] = {"CMP", 0};
        r128["dma_res"] = {"DMA", 0};
        r8["alu_res"] = {"ALU", 0};
        r8["visit_res"] = {"VISIT", 0};
        raw_array["raw_res"] = {"RAW", 0};
        r32["addr"] = {"index2addr", 0};
        // Vars
        r32["C_index"] = {"Candidate Index", {0}};
        r32["C_dist"] = {"Candidate Dist", {0}};
        r32["C_size"] = {"Candidate Size", 0};
        r32["W_index"] = {"Wait Index", {0}};
        r32["W_dist"] = {"Wait Dist", {0}};
        r32["W_size"] = {"Wait Size", 0};
        r32["lowB_index"] = {"lower bound index", 0};
        r32["lowB_dist"] = {"lower bound dist", 0};
        r32["current_node"] = {"current node", 0};
        r32["CN_neighbor_index"] = {"Current Node NeighborList indexs", {0}};
        r32["i"] = {"temp var", 0};
        r32["i20"] = {"temp var", 0};
        r32["dist1"] = {"dist1", 0};
    }

    void* find_match(std::string name, size_t &reg_size) {
        for (auto &&i : raw_array) {
            if (name == i.first) {
                reg_size = sizeof(i.second.reg);
                return &i.second.reg;
            }
        }
        for (auto &&i : list_array) {
            if (name == i.first) {
                reg_size = sizeof(i.second.reg);
                return &i.second.reg;
            }
        }
        for (auto &&i : r8) {
            if (name == i.first) {
                reg_size = sizeof(i.second.reg);
                return &i.second.reg;
            }
        }
        for (auto &&i : r32) {
            if (name == i.first) {
                reg_size = sizeof(i.second.reg);
                return &i.second.reg;
            }
        }
        for (auto &&i : r64) {
            if (name == i.first) {
                reg_size = sizeof(i.second.reg);
                return &i.second.reg;
            }
        }
        for (auto &&i : r128) {
            if (name == i.first) {
                reg_size = sizeof(i.second.reg);
                return &i.second.reg;
            }
        }
        throw "no matched register";
    }
};

} // namespace phnsw
} // namespace SST

#endif