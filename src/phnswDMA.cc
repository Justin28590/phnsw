// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst/core/sst_config.h> // This include is REQUIRED for all implementation files

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

phnswDMA::phnswDMA(ComponentId_t id, Params& params, TimeConverter *time) :
    phnswDMAAPI(id, params, time), clockTC(time)  {
    setDefaultTimeBase(time);
    // amount = params.find<int>("amount",  1);
    amount = 1;

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
}

phnswDMA::~phnswDMA() { }

void phnswDMA::DMAread(SST::Interfaces::StandardMem::Addr addr, size_t size)
{
    std::cout << "<File: phnswDMA.cc> <Function: phnswDMA::DMAread()> DMA read called with addr 0x"
    << std::hex << addr
    << std::dec << " and size " << size
    << " at cycle " << std::dec << getCurrentSimCycle()
    << std::endl;

    SST::Interfaces::StandardMem::Request *req;
    req = new SST::Interfaces::StandardMem::Read(addr, size);
    req->setNoncacheable();
    output.output("%s\n", req->getString().c_str());
    requests[req->getID()] = timestamp;
    memory->send(req);
    num_events_issued++;
}

void phnswDMA::serialize_order(SST::Core::Serialization::serializer& ser) {
    SubComponent::serialize_order(ser);

    SST_SER(amount);
}

void phnswDMA::handleEvent( SST::Interfaces::StandardMem::Request *ev ) {
    std::cout << "<File: phnswDMA.cc> <Function: phnswDMA::handleEvent()> time=" << getCurrentSimTime() << std::endl;
    delete ev;
}

void phnswDMA::init(unsigned int phase) {
    memory->init(phase);
    std::cout << "memory->init(phase) called" << std::endl;
}
