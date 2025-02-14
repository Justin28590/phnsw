/*
 * @Author: Zeng GuangYi tgy_scut2021@outlook.com
 * @Date: 2024-11-10 00:22:53
 * @LastEditors: Zeng GuangYi tgy_scut2021@outlook.com
 * @LastEditTime: 2025-02-14 16:45:06
 * @FilePath: /phnsw/src/phnsw.cc
 * @Description: phnsw Core Component
 * 
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved. 
 */

#include <sst/core/sst_config.h> // This include is REQUIRED for all implementation files

#include "phnsw.h"

#ifndef _UTIL_H_
#define _UTIL_H_
#include "util.h"
#endif

#include <sst/core/interfaces/stringEvent.h>
#include <sst/core/realtimeAction.h>

using namespace SST;
using namespace phnsw;

/**
 * @description: Constructor
 *               Find and add parameters to local variables.
 *               Init output.
 *               Register a clock as clockTC.
 *               Tell SST to wait until we authorize it to exit.
 *               Initialize Standard Memory Interface.
 *               Initialize DMA.
 * @param {ComponentId_t} id comes from SST core.
 * @param {Params&} params come from SST core.
 * @return {*}
 */
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

    Params dmaparams;

    dma = loadUserSubComponent<phnswDMAAPI>("dma", SST::ComponentInfo::SHARE_NONE/* , dmaparams */, clockTC);

    sst_assert(dma, CALL_INFO, -1, "Unable to load dma subcomponent\n");

    // Load Instructions
    inst_file.open("instructions/instructions.asm");
}


/**
 * @description: Destructor (unused)
 * @return {*}
 */
Phnsw::~Phnsw() { }

/**
 * @description: lifecycle function: init,
 *               init memory and dma.
 * @param {unsigned int} phase from SST core
 * @return {*}
 */
void Phnsw::init(unsigned int phase) {
    memory->init(phase);
    dma->init(phase);
}

/**
 * @description: lifecycle function: setup (unused)
 * @return {*}
 */
void Phnsw::setup() {
    // output.verbose(CALL_INFO, 1, 0, "Component is being setup.\n");
}

/**
 * @description: lifecycle function: complete (unused)
 * @param {unsigned int} phase
 * @return {*}
 */
void Phnsw::complete(unsigned int phase) {
    // output.verbose(CALL_INFO, 1, 0, "Component is participating in phase %d of complete.\n", phase);
}

/**
 * @description: lifecycle function: finish (unuesd)
 * @return {*}
 */
void Phnsw::finish() {
    // output.verbose(CALL_INFO, 1, 0, "Component is being finished.\n");
}


/**
 * @description: Clock callback function
 *               executed at the end of every clock tick
 *               at some cycle (10 and 50), we send a request to memory
 * @param {Cycle_t} currentCycle, passed into this function by memory
 * @return {*}
 */
bool Phnsw::clockTick( SST::Cycle_t currentCycle ) {
    timestamp++;
    std::getline(inst_file, inst_line);
    uint8_t word_counts = 0;
    std::stringstream ss(inst_line);
    std::string word;
    while (ss >> word) {
        if (word.compare(";") == 0 | word.compare("\n") == 0) {
            break; // Remove comments
        }
        if (word_counts == 0) { // Recognize Operation
            for (size_t i=0; i<inst_struct_size; i++) {
                if (word.compare(Phnsw::inst_struct[i].asmop) == 0) {
                    (this->*(inst_struct[i].handeler))(); // Exe module function
                }
            }
        }
        std::cout << word;
        word_counts ++;
    }
    std::cout << std::endl;
    return false;
}

/**
 * @description: (not used now, move to phnswDMA.h/cc::phnswDMA::handleEvent( SST::Interfaces::StandardMem::Request *ev ))
 *               Memory response handler, execute when the memory returns a response.
 *               Whenever received an respone, **remove it from requests map**,
 *               and record some statistics.
 * @param {Request *} response returned by memory.
 * @return {*}
 */
void Phnsw::handleEvent(SST::Interfaces::StandardMem::Request * response) {
    std::unordered_map<uint64_t, SST::SimTime_t>::iterator i = requests.find(response->getID());
    sst_assert(i != requests.end(), CALL_INFO, -1, "Received response but request not found! ID = %" PRIu64 "\n", response->getID());
    requests.erase(i); // remove it from requests map
    // display what message returned from memory
    output.output("Time.%" PRIu64 " at %s\n", getCurrentSimCycle()/1000, response->getString().c_str());
    num_events_returned++;
    delete response;
}

const Phnsw::InstStruct Phnsw::inst_struct[] = {
    {"END", "end the simulation", &Phnsw::inst_end},
    {"dummy", "dummy inst", &Phnsw::inst_dummy}
};

const size_t Phnsw::inst_struct_size = sizeof(Phnsw::inst_struct) / sizeof(Phnsw::InstStruct);

int Phnsw::inst_end() {
    primaryComponentOKToEndSim();
    return 0;
}

int Phnsw::inst_dummy() {
    return 0;
}