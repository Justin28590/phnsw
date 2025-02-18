#ifndef REGISTER_H
#define REGISTER_H

#include <iostream>
#include <stdint.h>
#include <string>
#include <vector>
#include <any>

namespace SST {
namespace phnsw {

template <typename T>
struct Register_template {
    std::string reg_name;
    std::string description;
    T reg;
};

struct Register {
    std::vector<Register_template<uint8_t[128]>> raw_array;
    std::vector<Register_template<uint32_t[10]>> list_array;
    std::vector<Register_template<uint8_t>> r8;
    std::vector<Register_template<uint32_t>> r32;
    std::vector<Register_template<uint64_t>> r64;
    std::vector<Register_template<__uint128_t>> r128;
    // std::vector<Register_template<uint32_t[60]>> array_uint32_60;
    // std::vector<Register_template<uint32_t[32]>> array_uint32_32;
    
    /* {"reg_name", "description", init value} */
    Register () {
        // Sources
        raw_array.push_back({"raw1", "DistCalc", {0}});
        raw_array.push_back({"raw2", "DistCalc", {0}});
        list_array.push_back({"list", "LookUp", {0}});
        r32.push_back({"list_index", "LookUp", 0});
        r32.push_back({"target", "LookUp", 0});
        r32.push_back({"cmp1", "CMP", 0});
        r32.push_back({"cmp2", "CMP", 0});
        r64.push_back({"dma_addr", "DMA start addr", 0});
        r64.push_back({"dma_offset", "DMA read length", 0});
        r8.push_back({"num1", "ALU", 0});
        r8.push_back({"num2", "ALU", 0});
        r32.push_back({"visit_index", "VISIT", 0});
        r64.push_back({"jmp_addr", "JMP", 0});
        r8.push_back({"raw_index", "fetch RAW from SPM", 0});
        r32.push_back({"wrm_index", "WRM", 0});
        r32.push_back({"index2addr", "index2addr", 0});
        // Destinations
        r32.push_back({"distance_res", "DistCalc", 0});
        r32.push_back({"look_res_index", "LookUp", 0});
        r32.push_back({"look_res_distance", "LookUp", 0});
        r8.push_back({"cmp_res", "CMP", 0});
        r128.push_back({"dma_res", "DMA", 0});
        r8.push_back({"alu_res", "ALU", 0});
        r8.push_back({"visit_res", "VISIT", 0});
        raw_array.push_back({"raw_res", "RAW", 0});
        r32.push_back({"addr", "index2addr", 0});
        // Vars
        r32.push_back({"C_index", "Candidate Index", {0}});
        r32.push_back({"C_dist", "Candidate Dist", {0}});
        r32.push_back({"C_size", "Candidate Size", 0});
        r32.push_back({"W_index", "Wait Index", {0}});
        r32.push_back({"W_dist", "Wait Dist", {0}});
        r32.push_back({"W_size", "Wait Size", 0});
        r32.push_back({"lowB_index", "lower bound index", 0});
        r32.push_back({"lowB_dist", "lower bound dist", 0});
        r32.push_back({"current_node", "current node", 0});
        r32.push_back({"CN_neighbor_index", "Current Node NeighborList indexs", {0}});
        r32.push_back({"i", "temp var", 0});
        r32.push_back({"i20", "temp var", 0});
        r32.push_back({"dist1", "dist1", 0});
    }
};

} // namespace phnsw
} // namespace SST

#endif