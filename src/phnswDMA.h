/*
 * @Author: Zeng GuangYi tgy_scut2021@outlook.com
 * @Date: 2024-12-17 16:46:55
 * @LastEditors: Zeng GuangYi tgy_scut2021@outlook.com
 * @LastEditTime: 2025-03-24 01:30:29
 * @FilePath: /phnsw/src/phnswDMA.h
 * @Description: phnsw DMA Component header
 * 
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved. 
 */

#ifndef _PHNSWDMAAPI_SUBCOMPONENT_H
#define _PHNSWDMAAPI_SUBCOMPONENT_H

/*
 * This is an example of a simple subcomponent that can take a number and do a computation on it. 
 * This file happens to have multiple classes declaring both the SubComponentAPI 
 * as well as a few subcomponents that implement the API.
 *  
 * See 'basicSubComponent_component.h' for more information on how the example simulation works
 */

#define SPM_NEIGHBOR_ADDR 0x0
#define SPM_NEIGHBOR_SIZE 0x80 // 0x80(16) = 128(10) in bytes = 32 * 4(bytes)
#define SPM_RAW_BASE SPM_NEIGHBOR_SIZE
#define MEM_ADDR_BASE 0x400 // 0x400(16) = 1024(10)
#define MEM_NEIGHBOR ADDR 0X0
#define MEM_NEIGHBOR_SIZE 1280000
#define MEM_RAW_BASE 0x138800
    
#include <sst/core/subcomponent.h>
#include <sst/core/interfaces/stdMem.h>
#include <sst/core/params.h>

namespace SST {
namespace phnsw {

/*****************************************************************************************************/

class phnswDMAAPI : public SST::SubComponent
{
public:
    /* 
     * Register this API with SST so that SST can match subcomponent slots to subcomponents 
     */
    SST_ELI_REGISTER_SUBCOMPONENT_API(SST::phnsw::phnswDMAAPI,TimeConverter*)
    
    phnswDMAAPI(ComponentId_t id, Params& params, TimeConverter *time) : SubComponent(id) { }
    virtual ~phnswDMAAPI() { }

    // These are the two functions described in the comment above
    virtual void DMAread(SST::Interfaces::StandardMem::Addr addr, size_t size, void *res, size_t res_size) =0;
    virtual void DMAwrite(SST::Interfaces::StandardMem::Addr addr, size_t size, std::vector<uint8_t>* data) =0;
    virtual void Resset(void *res, size_t res_size) =0;

    // Serialization
    phnswDMAAPI();
    ImplementVirtualSerializable(SST::phnsw::phnswDMAAPI);
};

/*****************************************************************************************************/

class phnswDMA : public phnswDMAAPI {
public:
    
    // Register this subcomponent with SST and tell SST that it implements the 'phnswDMAAPI' API
    SST_ELI_REGISTER_SUBCOMPONENT(
            phnswDMA,                                   // Class name
            "phnsw",                                    // Library name, the 'lib' in SST's lib.name format
            "phnswDMA",                                 // Name used to refer to this subcomponent, the 'name' in SST's lib.name format
            SST_ELI_ELEMENT_VERSION(1,0,0),             // A version number
            "SubComponent DMA that READ from DRAM",     // Description
            SST::phnsw::phnswDMAAPI                     // Fully qualified name of the API this subcomponent implements
                                                        // A subcomponent can implment an API from any library
            )

    // Other ELI macros as needed for parameters, ports, statistics, and subcomponent slots
    SST_ELI_DOCUMENT_PARAMS(
    { "amount",                  "Amount to increment by", "1" },
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

    /* Document subcomponent slots (optional if no subcomponent slots declared)
     *  Format: { "slotname", "description", "subcomponentAPI" }
     */
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
        {"memory", "Interface to memory (e.g., caches)", "SST::Interfaces::StandardMem"}
    )


    phnswDMA(ComponentId_t id, Params& params, TimeConverter *time);
    // phnswDMA(const phnswDMA&) = delete;
    ~phnswDMA();

    // SST lifecycle functions (optional if not used)
    virtual void init(unsigned int phase) override;
    // virtual void setup() override;
    // virtual void complete(unsigned int phase) override;
    // virtual void finish() override;

    void DMAread(SST::Interfaces::StandardMem::Addr addr, size_t size, void *res, size_t res_size) override;
    void DMAwrite(SST::Interfaces::StandardMem::Addr addr, size_t size, std::vector<uint8_t>* data) override;
    void serialize_order(SST::Core::Serialization::serializer& ser) override;

    void handleEvent( SST::Interfaces::StandardMem::Request *ev );
    void Resset(void *res, size_t res_size);

private:
    int amount;
    SST::Output output;

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

    uint64_t timestamp;     // current timestamp
    uint64_t num_events_issued;      // number of events that have been issued at a given time
    uint64_t num_events_returned;    // number of events that have returned

    void *res;
    size_t res_size;
};

} } /* Namspaces */

#endif /* _phnswDMAAPI_SUBCOMPONENT_H */
