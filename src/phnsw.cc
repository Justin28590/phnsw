// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst/core/sst_config.h> // This include is REQUIRED for all implementation files

#include "phnsw.h"
#include "util.h"

#include <sst/core/interfaces/stringEvent.h>
#include <sst/core/realtimeAction.h>

Phnsw::Phnsw( SST::ComponentId_t id, SST::Params& params ) :
    SST::Component(id), repeats(0) {

    uint32_t z_seed = params.find<uint32_t>("rngseed", 7);
    output.init("Phnsw-" + getName() + "-> ", 137, 0, SST::Output::STDOUT);

    printFreq  = params.find<SST::Cycle_t>("printFrequency", 5);
    maxRepeats = params.find<SST::Cycle_t>("repeats", 10);

    if( ! (printFreq > 0) ) {
    	output.fatal(CALL_INFO, -1, "Error: printFrequency must be greater than zero.\n");
    }

    output.verbose(CALL_INFO, 1, 0, "Config: maxRepeats=%" PRIu64 ", printFreq=%" PRIu64 "\n",
        static_cast<uint64_t>(maxRepeats), static_cast<uint64_t>(printFreq));

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

    // CPU parameters
    SST::UnitAlgebra clock = params.find<SST::UnitAlgebra>("clock", "1GHz");
    clockHandler = new SST::Clock::Handler<Phnsw>(this, &Phnsw::clockTick);
    clockTC = registerClock( clock, clockHandler );

    reqQueueSize = params.find<uint32_t>("maxOutstandingRequests", 8);
    reqPerCycle = params.find<uint32_t>("maxRequestsPerCycle", 2);

    reqsToIssue = params.find<uint64_t>("reqsToIssue", 1000);

    // Tell SST to wait until we authorize it to exit
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    std::string size = params.find<std::string>("scratchSize", "0");
    size += "B";
    params.insert("scratchpad_size", size);

    // output.output("hello world!!!\n");

    memory = loadUserSubComponent<SST::Interfaces::StandardMem>(
                "memory",
                SST::ComponentInfo::SHARE_NONE,
                clockTC,
                new SST::Interfaces::StandardMem::Handler<Phnsw>(this, &Phnsw::handleEvent)
            );

    sst_assert(memory, CALL_INFO, -1, "Unable to load scratchInterface subcomponent\n");

    // Initialize local variables
    Phnsw::timestamp = 0;
    Phnsw::num_events_issued = Phnsw::num_events_returned = 0;

}


Phnsw::~Phnsw() { }

void Phnsw::init(unsigned int phase) {
    memory->init(phase);
    // output.verbose(CALL_INFO, 1, 0, "Component is participating in phase %d of init.\n", phase);
}

void Phnsw::setup() {
    // output.verbose(CALL_INFO, 1, 0, "Component is being setup.\n");
}

void Phnsw::complete(unsigned int phase) {
    // output.verbose(CALL_INFO, 1, 0, "Component is participating in phase %d of complete.\n", phase);
}

void Phnsw::finish() {
    // output.verbose(CALL_INFO, 1, 0, "Component is being finished.\n");
}


bool Phnsw::clockTick( SST::Cycle_t currentCycle ) {

    // if( currentCycle % printFreq == 0 ) {
    //     output.verbose(CALL_INFO, 1, 0, "Hello World! Current Cycle=%ld\n", currentCycle);
    // }

    // repeats++;

    // if( repeats == maxRepeats ) {
    //     primaryComponentOKToEndSim();
	//     return true;    // Stop calling this clock handler
    // } else {
	//     return false;   // Keep calling this clock handler
    // }

    timestamp++;

    if (num_events_issued == reqsToIssue) {
        if (requests.empty()) {
            primaryComponentOKToEndSim();
            return true;
        }
    } else {
        // Can we issue another request this cycle?
        if (requests.size() < reqQueueSize) {
            SST::Interfaces::StandardMem::Request *req;

            // Send Read request
            uint32_t size = 2;
            SST::Interfaces::StandardMem::Addr addr = 0; // currentCycle * 8;

            if (currentCycle == 10 /*% 50 == 0*/) {
                // std::vector<uint8_t> data(size, 0xff);
                std::vector<uint8_t> data = {0xab, 0xcd};
                req = new SST::Interfaces::StandardMem::Write(addr, size, data);
                output.output("ScratchCPU (%s) sending Write. Addr: %" PRIu64 ", Size: %u, simtime: %" PRIu64 "ns\n", getName().c_str(), addr, size, getCurrentSimCycle()/1000);
                output.output("%s\n", req->getString().c_str());
                requests[req->getID()] = timestamp;
                memory->send(req);
                num_events_issued++;
            } else if (currentCycle == 50/*% 10 == 0*/) {
                req = new SST::Interfaces::StandardMem::Read(addr, size);
                output.output("ScratchCPU (%s) sending Read.%" PRIu64 " Addr: %" PRIu64 ", Size: %u, simtime: %" PRIu64 "ns\n", getName().c_str(), req->getID(), addr, size, getCurrentSimCycle()/1000);
                requests[req->getID()] = timestamp;
                memory->send(req);
                num_events_issued++;
            }
        }
    }
    return false;
}

// Memory response handler
void Phnsw::handleEvent(SST::Interfaces::StandardMem::Request * response) {
    std::unordered_map<uint64_t, SST::SimTime_t>::iterator i = requests.find(response->getID());
    sst_assert(i != requests.end(), CALL_INFO, -1, "Received response but request not found! ID = %" PRIu64 "\n", response->getID());
    requests.erase(i);
    // output.output("erase request %" PRIu64 " at %" PRIu64 "ns\n", response->getID(), getCurrentSimCycle()/1000);
    output.output("Time.%" PRIu64 " at %s\n", getCurrentSimCycle()/1000, response->getString().c_str());
    num_events_returned++;
    delete response;
}

