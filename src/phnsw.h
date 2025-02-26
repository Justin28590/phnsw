/*
 *                        _oo0oo_
 *                       o8888888o
 *                       88" . "88
 *                       (| -_- |)
 *                       0\  =  /0
 *                     ___/`---'\___
 *                   .' \\|     |// '.
 *                  / \\|||  :  |||// \
 *                 / _||||| -:- |||||- \
 *                |   | \\\  - /// |   |
 *                | \_|  ''\---/''  |_/ |
 *                \  .-\__  '-'  ___/-. /
 *              ___'. .'  /--.--\  `. .'___
 *           ."" '<  `.___\_<|>_/___.' >' "".
 *          | | :  `- \`.;`\ _ /`;.`/ - ` : | |
 *          \  \ `_.   \_ __\ /__ _/   .-` /  /
 *      =====`-.____`.___ \_____/___.-`___.-'=====
 *                        `=---='
 * 
 * 
 *      ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * 
 *            佛祖保佑     永不宕机     永无BUG
 * 
 *        佛曰:  
 *                写字楼里写字间，写字间里程序员；  
 *                程序人员写程序，又拿程序换酒钱。  
 *                酒醒只在网上坐，酒醉还来网下眠；  
 *                酒醉酒醒日复日，网上网下年复年。  
 *                但愿老死电脑间，不愿鞠躬老板前；  
 *                奔驰宝马贵者趣，公交自行程序员。  
 *                别人笑我忒疯癫，我笑自己命太贱；  
 *                不见满街漂亮妹，哪个归得程序员？
 */

/*
 * @Author: Zeng GuangYi tgy_scut2021@outlook.com
 * @Date: 2024-11-10 00:22:53
 * @LastEditors: Zeng GuangYi tgy_scut2021@outlook.com
 * @LastEditTime: 2024-12-30 16:27:01
 * @FilePath: /phnsw/src/phnsw.h
 * @Description: phnsw Core Component header
 * 
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved. 
 */

#ifndef _PHNSW_H
#define _PHNSW_H

#include <fstream>
#include <sstream>

#include <sst/core/component.h>
#include <sst/core/interfaces/stdMem.h>
#include <sst/core/params.h>
#include <sst/core/rng/marsaglia.h>

#include <unordered_map>
#include <bitset>

// Include file for the SubComponent API we'll use
#include "phnswDMA.h"

#include "Register/Register.h"

namespace SST {
namespace phnsw {

class Phnsw : public SST::Component {

public:
/* SST ELI macros */
    /* Register component */
    SST_ELI_REGISTER_COMPONENT(
    	Phnsw,                                          // Class name
    	"phnsw",                                        // Name of library
    	"phnsw",                                        // Lookup name for component
    	SST_ELI_ELEMENT_VERSION( 1, 0, 0 ),             // Component version
        "Demonstration of an External Element for SST", // Description
    	COMPONENT_CATEGORY_PROCESSOR                    // Other options: 
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
        {"memory", "Interface to memory (e.g., caches)", "SST::Interfaces::StandardMem"},
        {"dma", "Interface to DMA", "phnsw::phnswDMAAPI"}
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

    SST::phnsw::phnswDMAAPI *dma;
  
    static Register Registers;

    // instructions
    std::ifstream inst_file;
    int inst_time;
    /*
    most inside: 1 instruction (1 or 3 string elements)
    middle inside: instructions for this clk (maybe 1 or 2 vec elements)
    out: instructions fro all clks (very lot vec elements)
    */
    std::vector<std::vector<std::vector<std::string>>> img;
    std::vector<std::vector<std::string>> inst_now;
    int inst_count; // only will be 0 or 1
    void load_inst_creat_img();
    void display_img();

public:
    int pc;
    struct InstStruct {
        std::string asmop;
        std::string description;
        int (Phnsw::*handeler) (void *rd_temp_ptr, uint32_t *stage_now);
        std::string rd;
        void *rd_temp;
        uint32_t stages;
        uint32_t *stage_now;

        InstStruct(std::string asmop, std::string description, int (Phnsw::*handeler) (void *rd_temp_ptr, uint32_t *stage_now), std::string rd, uint32_t stages) :
            asmop(asmop), description(description), handeler(handeler), rd(rd), stages(stages) {
                stage_now = new uint32_t(0);
                rd_temp = new char[Phnsw::Registers.find_size(rd)];
            }
    };
    static const std::vector<InstStruct> inst_struct;
    static const size_t inst_struct_size;
    // module functions
    int inst_end(void *rd_temp_ptr, uint32_t *stage_now);
    int inst_mov(void *rd_temp_ptr, uint32_t *stage_now);
    int inst_add(void *rd_temp_ptr, uint32_t *stage_now);
    int inst_info(void *rd_temp_ptr, uint32_t *stage_now);
    int inst_dummy(void *rd_temp_ptr, uint32_t *stage_now);
};

} } // namespace phnsw
#endif
