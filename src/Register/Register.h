#ifndef REGISTER_H
#define REGISTER_H

#include <stdint.h>
#include <string>

namespace SST {
namespace phnsw {

template <typename T>
struct Register_template {
    std::string reg_name;
    std::string description;
    T reg;
};

struct Register {
    Register_template<uint8_t[128]> raw1 = {"raw1", "DistCalc", {0}};
    Register_template<uint8_t[128]> raw2 = {"raw2", "DistCalc", {0}};
    Register_template<uint32_t[10]> list = {"list", "LookUp", {0}};
    Register_template<uint32_t[10]> list_index = {"list_index", "LookUp", 0};
    Register_template<uint32_t> target = {"target", "LookUp", 0};
    Register_template<uint32_t> cmp1 = {"cmp1", "CMP", 0};
    Register_template<uint32_t> cmp2 = {"cmp2", "CMP", 0};
    Register_template<uint64_t> dma_addr = {"dma_addr", "DMA start addr", 0};
    Register_template<uint64_t> dma_offset = {"dma_offset", "DMA read length", 0};
    Register_template<uint8_t> num1 = {"num1", "ALU", 0};
    Register_template<uint8_t> num2 = {"num2", "ALU", 0};
    Register_template<uint32_t> visit_index = {"visit_index", "VISIT", 0};
    Register_template<uint64_t> jmp_addr = {"jmp_addr", "JMP", 0};
    Register_template<uint8_t> raw_index = {"raw_index", "fetch RAW from SPM", 0};
    Register_template<uint32_t> wrm_index = {"wrm_index", "WRM", 0};
    Register_template<uint32_t> index2addr = {"index2addr", "index2addr", 0};
};

} // namespace phnsw
} // namespace SST

#endif