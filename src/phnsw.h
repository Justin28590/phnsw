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

#ifndef _PHNSW_H
#define _PHNSW_H

#include <sst/core/component.h>
#include <sst/core/interfaces/stdMem.h>
#include <sst/core/params.h>
#include <sst/core/rng/marsaglia.h>

#include <unordered_map>

// Include file for the SubComponent API we'll use
#include "phnswDMA.h"

namespace SST {
namespace phnsw {

class Phnsw : public SST::Component {

public:
/* SST ELI macros */
    /* Register component */
    SST_ELI_REGISTER_COMPONENT(
    	Phnsw,              // Class name
    	"phnsw",            // Name of library
    	"phnsw",            // Lookup name for component
    	SST_ELI_ELEMENT_VERSION( 1, 0, 0 ), // Component version
        "Demonstration of an External Element for SST", // Description
    	COMPONENT_CATEGORY_PROCESSOR    // Other options: 
        //      COMPONENT_CATEGORY_MEMORY, 
        //      COMPONENT_CATEGORY_NETWORK, 
        //      COMPONENT_CATEGORY_UNCATEGORIZED
    )

    /* 
     * Document parameters. 
     *  Required parameter format: { "paramname", "description", NULL }
     *  Optional parameter format: { "paramname", "description", "default value"}
     */
    SST_ELI_DOCUMENT_PARAMS(
    { "printFrequency",          "How frequently to print a message from the component", "5" },
    { "repeats",                 "Number of repetitions to make", "10" },
    { "scratchSize",             "(uint) Size of the scratchpad in bytes"},
    { "maxAddr",                 "(uint) Maximum address to generate (i.e., scratchSize + size of memory)"},
    { "rngseed",                 "(int) Set a seed for the random generator used to create requests", "7"},
    { "scratchLineSize",         "(uint) Line size for scratch, max request size for scratch", "64"},
    { "memLineSize",             "(uint) Line size for memory, max request size for memory", "64"},
    { "clock",                   "(string) Clock frequency in Hz or period in s", "1GHz"},
    { "maxOutstandingRequests",  "(uint) Maximum number of requests outstanding at a time", "8"},
    { "maxRequestsPerCycle",     "(uint) Maximum number of requests to issue per cycle", "2"},
    { "reqsToIssue",             "(uint) Number of requests to issue before ending simulation", "1000"}
    )


    /* Document ports (optional if no ports declared)
     *  Format: { "portname", "description", { "eventtype0", "eventtype1" } }
     */
    SST_ELI_DOCUMENT_PORTS(
        {"mem_link", "Connection to spm", { "memHierarchy.MemEventBase" } }
    )

    /* Document statistics (optional if no statistics declared)
     *  Format: { "statisticname", "description", "units", "enablelevel" }
     */
    SST_ELI_DOCUMENT_STATISTICS( )

    /* Document subcomponent slots (optional if no subcomponent slots declared)
     *  Format: { "slotname", "description", "subcomponentAPI" }
     */
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
        {"memory", "Interface to memory (e.g., caches)", "SST::Interfaces::StandardMem"}
    )

/* Class members */
    // Constructor
    Phnsw( SST::ComponentId_t id, SST::Params& params );
	
    // Destructor
    ~Phnsw();
        
    // SST lifecycle functions (optional if not used)
    virtual void init(unsigned int phase) override;
    virtual void setup() override;
    virtual void complete(unsigned int phase) override;
    virtual void finish() override;

    // Clock handler
    bool clockTick( SST::Cycle_t currentCycle );
    void handleEvent( SST::Interfaces::StandardMem::Request *ev );

private:
    SST::Output output;
    SST::Cycle_t printFreq;
    SST::Cycle_t maxRepeats;
    SST::Cycle_t repeats;

    // Parameters
    uint64_t scratchSize;       // Size of scratchpad
    uint64_t maxAddr;           // Max address of memory
    uint64_t scratchLineSize;   // Line size for scratchpad -> controls maximum request size
    uint64_t memLineSize;       // Line size for memory -> controls maximum request size
    uint64_t log2ScratchLineSize;
    uint64_t log2MemLineSize;

    uint32_t reqPerCycle;   // Up to this many requests can be issued in a cycle
    uint32_t reqQueueSize;  // Maximum number of outstanding requests
    uint64_t reqsToIssue;   // Number of requests to issue before ending simulation

    // Local variables
    SST::Interfaces::StandardMem * memory;         // scratch interface
    std::unordered_map<uint64_t, SST::SimTime_t> requests; // Request queue (outstanding requests)
    SST::TimeConverter *clockTC;                 // Clock object
    SST::Clock::HandlerBase *clockHandler;       // Clock handler
    SST::RNG::MarsagliaRNG rng;             // Random number generator for addresses, etc.

    uint64_t timestamp;     // current timestamp
    uint64_t num_events_issued;      // number of events that have been issued at a given time
    uint64_t num_events_returned;    // number of events that have returned
};

} } // namespace phnsw
#endif
