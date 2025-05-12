/*
 * @Author: Zeng GuangYi tgy_scut2021@outlook.com
 * @Date: 2024-12-17 16:46:55
 * @LastEditors: Zeng GuangYi tgy_scut2021@outlook.com
 * @LastEditTime: 2025-05-12 18:30:28
 * @FilePath: /phnsw/src/phnswDMA.cc
 * @Description: phnsw DMA Component header
 * 
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved. 
 */

#include <cstdint>
#include <sst/core/sst_config.h> // This include is REQUIRED for all implementation files
#include <vector>

#include "phnswDMA.h"

#ifndef _UTIL_H_
#define _UTIL_H_
#include "util.h"
#endif

#include <sst/core/interfaces/stringEvent.h>
#include <sst/core/realtimeAction.h>

using namespace SST;
using namespace phnsw;

/***********************************************************************************/
// Since the classes are brief, this file has the implementation for all four 
// basicSubComponentAPI subcomponents declared in basicSubComponent_subcomponent.h
/***********************************************************************************/
// phnswDMADRAM

/**
 * @description: Constructor,
 *               Get clock from parent Component phnsw (clock: time),
 *               Find and add parameters to local variables,
 *               Initialize Standard Memory Interface.
 * @param {ComponentId_t} id comes from SST core
 * @param {Params&} params come from SST core
 * @param {TimeConverter} *time comes from phnsw core (parent Component)
 * @return {*}
 */
phnswDMA::phnswDMA(ComponentId_t id, Params& params, TimeConverter *time) :
    phnswDMAAPI(id, params, time), clockTC(time) {
    setDefaultTimeBase(time);
    amount = params.find<int>("amount",  1);

    // Memory parameters
    scratchSize = params.find<uint64_t>("scratchSize", 0);
    maxAddr = params.find<uint64_t>("maxAddr", 0);
    scratchLineSize = params.find<uint64_t>("scratchLineSize", 64);
    memLineSize = params.find<uint64_t>("memLineSize", 64);

    if (!scratchSize) output.fatal(CALL_INFO, -1, "Error (%s): invalid param 'scratchSize' - must be at least 1", getName().c_str());
    if (maxAddr <= scratchSize) output.fatal(CALL_INFO, -1, "Error (%s): invalid param 'maxAddr' - must be larger than 'scratchSize'", getName().c_str());
    if (scratchLineSize < 1) output.fatal(CALL_INFO, -1, "Error (%s): invalid param 'scratchLineSize' - must be greater than 0\n", getName().c_str());
    if (memLineSize < scratchLineSize) output.fatal(CALL_INFO, -1, "Error (%s): invalid param 'memLineSize' - must be greater than or equal to 'scratchLineSize'\n", getName().c_str());
    if (!SST::MemHierarchy::isPowerOfTwo(scratchLineSize)) output.fatal(CALL_INFO, -1, "Error (%s): invalid param 'scratchLineSize' - must be a power of 2\n", getName().c_str());
    if (!SST::MemHierarchy::isPowerOfTwo(memLineSize)) output.fatal(CALL_INFO, -1, "Error (%s): invalid param 'memLineSize' - must be a power of 2\n", getName().c_str());

    log2ScratchLineSize = SST::MemHierarchy::log2Of(scratchLineSize);
    log2MemLineSize = SST::MemHierarchy::log2Of(memLineSize);

    reqQueueSize = params.find<uint32_t>("maxOutstandingRequests", 8);
    reqPerCycle = params.find<uint32_t>("maxRequestsPerCycle", 2);

    reqsToIssue = params.find<uint64_t>("reqsToIssue", 1000);

    std::string size = params.find<std::string>("scratchSize", "0");
    size += "B";
    params.insert("scratchpad_size", size);

    memory = loadUserSubComponent<SST::Interfaces::StandardMem>(
                "memory",
                SST::ComponentInfo::SHARE_NONE,
                time,
                new SST::Interfaces::StandardMem::Handler<phnswDMA>(this, &phnswDMA::handleEvent)
            );

    sst_assert(memory, CALL_INFO, -1, "Unable to load scratchInterface subcomponent\n");
    res = malloc(sizeof(uint64_t));
    
    // Disable StOP Flag
    phnswDMA::stopFlag = false;

    phnswDMA::is_spm = false;

    phnswDMA::is_vst = false;
    phnswDMA::is_vst_write = false;
    phnswDMA::vst_offset = 0;
}

/**
 * @description: Destructor (unused)
 * @return {*}
 */
phnswDMA::~phnswDMA() { }

/**
 * @description: DMA send Read request to memroy or scratchpad, call by phnsw core (parent Component).
 * @param {Addr} addr to read
 * @param {size_t} size of read data
 * @return {*}
 */
void phnswDMA::DMAread(SST::Interfaces::StandardMem::Addr addr, size_t size, void *rd_res, size_t rd_res_size) {
    // std::cout << "<File: phnswDMA.cc> <Function: phnswDMA::DMAread()> DMA read called with addr 0x"
    // << std::hex << addr
    // << std::dec << " and size " << size
    // << " at cycle " << std::dec << getCurrentSimTime()
    // << std::endl;

    SST::Interfaces::StandardMem::Request *req;
    req = new SST::Interfaces::StandardMem::Read(addr, size);
    req->setNoncacheable();
    // output.output("%s\n", req->getString().c_str());
    requests[req->getID()] = timestamp;
    memory->send(req);
    num_events_issued++;
    res = rd_res;
    // std::cout << "res=" << std::hex <<  (uint32_t) *(uint8_t *) res << std::dec << std::endl;
    res_size = rd_res_size;
}

void phnswDMA::DMAvst(SST::Interfaces::StandardMem::Addr addr, size_t size, void *res, size_t res_size) {
    vst_tmp_addr = addr;
    phnswDMA::DMAread(addr, size, res, res_size);
}

void phnswDMA::DMAget(SST::Interfaces::StandardMem::Addr srcAddr, SST::Interfaces::StandardMem::Addr dstAddr, uint32_t data_size) {
    // std::cout << "<File: phnswDMA.cc> <Function: phnswDMA::DMAget()> DMA get called with srcAddr 0x"
    // << std::hex << srcAddr
    // << std::dec << " and dstAddr 0x"<< std::hex << dstAddr
    // << " at cycle " << std::dec << getCurrentSimTime()
    // << std::dec << " and data_size " << data_size
    // << std::endl;

    SST::Interfaces::StandardMem::Request *req;
    req = new Interfaces::StandardMem::MoveData(srcAddr, dstAddr, data_size);
    requests[req->getID()] = timestamp;
    memory->send(req);
    num_events_issued++;
}

void phnswDMA::DMAspmrd(SST::Interfaces::StandardMem::Addr addr, size_t size, void *rd_res, size_t rd_res_size) {
    // std::cout << "<File: phnswDMA.cc> <Function: phnswDMA::DMAspmrd()> DMA spmrd called with addr 0x"
    // << std::hex << addr
    // <<std::dec << " and size " << size << std::endl;

    is_spm = true;
    spm_size = (int) size;
    spm_size_now = 0;
    spm_addr = addr;

    SST::Interfaces::StandardMem::Request *req;
    req = new SST::Interfaces::StandardMem::Read(spm_addr, 8);
    req->setNoncacheable();
    // output.output("%s\n", req->getString().c_str());
    memory->send(req);
    res = rd_res;
    res_size = 8;
    spm_size_now += 8;
}

/**
 * @description: DMA send Write request to memroy or scratchpad, call by phnsw core (parent Component).
 * @param {Addr} addr to write
 * @param {size_t} size of write data
 * @param {std::vector<uint8_t>*} data to write, generated in phnsw core (parent Component)
 * @return {*}
 */
void phnswDMA::DMAwrite(SST::Interfaces::StandardMem::Addr addr, size_t size, std::vector<uint8_t>* data) {
    // std::cout << "<File: phnswDMA.cc> <Function: phnswDMA::DMAwrite()> DMA write called with addr 0x"
    // << std::hex << addr
    // << std::dec << " and size " << size
    // << " at cycle " << std::dec << getCurrentSimTime()
    // << " write " << std::hex;
    // for (auto element: *data) {
    //     std::cout << static_cast<int>(element);
    // }
    // std::cout << std::dec << std::endl;

    SST::Interfaces::StandardMem::Request *req;
    req = new SST::Interfaces::StandardMem::Write(addr, size, *data);
    req->setNoncacheable(); // Key point! if non-cacheable not set, nothing will be written
    // output.output("ScratchCPU (%s) sending Write. Addr: %" PRIu64 ", Size: %lu, simtime: %" PRIu64 "ns\n", getName().c_str(), addr, size, getCurrentSimCycle()/1000);
    // output.output("%s\n", req->getString().c_str());
    // requests[req->getID()] = timestamp;
    memory->send(req);
    num_events_issued++;
}

/**
 * @description: Serializer function, I don't know what it does,
 *               it just exist in the Subcomponent Template so I copy it.
 * @param {serializer&} ser
 * @return {*}
 */
void phnswDMA::serialize_order(SST::Core::Serialization::serializer& ser) {
    SubComponent::serialize_order(ser);

    SST_SER(amount);
}

/**
 * @description: Memory response handler, execute when the memory returns a response.
                 Whenever received an respone, print some information about the respobne,
                 and delete respone.
 * @param {Request} *respone
 * @return {*}
 */
void phnswDMA::handleEvent( SST::Interfaces::StandardMem::Request *respone ) {
    std::vector<uint8_t> data;
    if (typeid(*respone) == typeid(SST::Interfaces::StandardMem::ReadResp))
        data = ((SST::Interfaces::StandardMem::ReadResp*) respone)->data;
    uint8_t temp_data[512] = {0};
    // std::cout << "<File: phnswDMA.cc> <Function: phnswDMA::handleEvent()> time=" << getCurrentSimTime()
    // << "; respone: " << respone->getString()
    // // << " Data=" << (uint16_t) data.back()
    // << std::endl;

    if (typeid(*respone) == typeid(SST::Interfaces::StandardMem::ReadResp)) {
        for (size_t i = 0; i < data.size(); i++) {
            temp_data[i] = data[i];
            // std::cout << "temp_data[" << i << "]=" << (uint16_t) temp_data[i] << std::endl;
        }
        if (is_vst) {
            is_vst = false;
            // std::cout << "vst read temp_data[0]=" << (uint16_t) temp_data[0] << std::endl;
            if (is_vst_write) {
                temp_data[0] = temp_data[0] | (1 << vst_offset);
                // std::cout << "after vst read temp_data[0]=" << (uint16_t) temp_data[0] << std::endl;
                is_vst_write = false;
                vst_tmp_data = temp_data[0];
                delete respone;
                vst_tmp_data = vst_tmp_data | (1 << vst_offset);
                std::vector<uint8_t> vst_tmp_data_write = {vst_tmp_data};
                phnswDMA::DMAwrite(vst_tmp_addr, 1, &vst_tmp_data_write);
                return;
            } else {
                temp_data[0] = temp_data[0] & (1 << vst_offset);
            }
        }
        std::memcpy(res, temp_data, res_size);
    }

    delete respone;
    if (spm_size_now >= spm_size) is_spm = false;

    if (is_spm) {
        SST::Interfaces::StandardMem::Request *req;
        req = new SST::Interfaces::StandardMem::Read(spm_addr + spm_size_now, 8);
        req->setNoncacheable();
        // output.output("%s\n", req->getString().c_str());
        memory->send(req);
        spm_size_now += 8;
        res = (void *) ((uint64_t) res + 8);
    } else {
        // is_spm = false;
        phnsw::phnswDMA::stopFlag = false;
    }
}

/**
 * @description: lifecycle function: init.
 *               Init memory interface of DMA.
 * @param {unsigned int} phase from SST core
 * @return {*}
 */
void phnswDMA::init(unsigned int phase) {
    memory->init(phase);
    std::cout << "memory->init(phase) called" << std::endl;
}

void phnswDMA::Resset(void *res, size_t res_size) {
    phnswDMA::res = res;
    phnswDMA::res_size = res_size;
}
