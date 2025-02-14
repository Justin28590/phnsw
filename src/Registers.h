#ifndef REGISTER_H
#define REGISTER_H

#include <stdint.h>

namespace SST
{
namespace phnsw
{
/* Registers */
struct Register {
    // Source
    uint8_t raw1[128]; // Distance Calculator
    uint8_t raw2[128]; // Distance Calculator
    uint32_t list[10]; // Look Up
    uint32_t list_index[10]; // Look Up
    uint32_t target; // Look Up
    uint32_t cmp1; // CMP
    uint32_t cmp2; // CMP
    uint64_t dma_addr; // DMA
    uint64_t dma_offset; // DMA
    uint8_t num1; // ALU
    uint8_t num2; // ALU
    uint32_t visit_index; // VISIT
    uint64_t jmp_addr; // JMP
    uint8_t raw_index; // RAW
    uint32_t wrm_index; // WRM
    uint32_t index2addr; // index2addr
    // Destination
    uint32_t distance_res; // Diatance Calculator
    uint32_t look_res_index; // Look UP
    uint32_t look_res_distance; // Look Up
    uint8_t cmp_res; // CMP
    __uint128_t dma_res; // DMA
    uint8_t alu_res; // ALU
    uint8_t visit_res; // VISIT
    uint8_t raw_res[128]; // RAW
    uint32_t addr; // index2addr
};

} // namespace phnsw
    
} // namespace SST


#endif