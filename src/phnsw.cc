/*
 * @Author: Zeng GuangYi tgy_scut2021@outlook.com
 * @Date: 2024-11-10 00:22:53
 * @LastEditors: Zeng GuangYi tgy_scut2021@outlook.com
 * @LastEditTime: 2025-05-05 20:12:06
 * @FilePath: /phnsw/src/phnsw.cc
 * @Description: phnsw Core Component
 * 
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved. 
 */

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <ostream>
#include <math.h>
#include <sst/core/sst_config.h> // This include is REQUIRED for all implementation files

#include "phnsw.h"
#include "phnswDMA.h"

#ifndef _UTIL_H_
#define _UTIL_H_
#include "util.h"
#endif

#include <sst/core/interfaces/stringEvent.h>
#include <sst/core/realtimeAction.h>

#include <bitset>

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

    // Initialize local variables
    Phnsw::timestamp = 0;
    Phnsw::num_events_issued = Phnsw::num_events_returned = 0;

    Params dmaparams;

    dma = loadUserSubComponent<phnswDMAAPI>("dma", SST::ComponentInfo::SHARE_NONE, clockTC);

    sst_assert(dma, CALL_INFO, -1, "Unable to load dma subcomponent\n");

    // Load Instructions
    inst_time = 0;
    Phnsw::load_inst_creat_img();
    output.verbose(CALL_INFO, 1, 0, "img created!\n");
}

Register Phnsw::Registers;

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
    // memory->init(phase);
    dma->init(phase);
}

/**
 * @description: lifecycle function: setup (unused)
 * @return {*}
 */
void Phnsw::setup() {
    // size_t raw1_size, raw2_size;
    // std::array<uint8_t, 128> *raw1 = (std::array<uint8_t, 128> *) Phnsw::Registers.find_match("raw1", raw1_size);
    // std::array<uint8_t, 128> *raw2 = (std::array<uint8_t, 128> *) Phnsw::Registers.find_match("raw2", raw2_size);
    // for (auto &&i : *raw1) {
    //     i = 0;
    // }
    // for (auto &&i : *raw2) {
    //     i = 10;
    // }
    // size_t list_size, list_index_size;
    // std::array<uint32_t, 10> *list = (std::array<uint32_t, 10> *) Phnsw::Registers.find_match("list", list_size);
    // std::array<uint32_t, 10> *list_index = (std::array<uint32_t, 10> *) Phnsw::Registers.find_match("list_index", list_index_size);
    // for (size_t i=0; i<10; i++) {
    //     (*list)[i] = i*10;
    //     (*list_index)[i] = i;
    // }
    // output.verbose(CALL_INFO, 1, 0, "Component is being setup.\n");
}

/**
 * @description: lifecycle function: complete (unused)
 * @param {unsigned int} phase
 * @return {*}
 */
void Phnsw::complete(unsigned int phase) {
    // size_t C_dist_size, C_index_size;
    // std::array<uint32_t, 40> *list = (std::array<uint32_t, 40> *) Phnsw::Registers.find_match("W_dist", C_dist_size);
    // std::array<uint32_t, 40> *list_index = (std::array<uint32_t, 40> *) Phnsw::Registers.find_match("W_index", C_index_size);
    // for (size_t i=0; i<40; i++) {
    //     std::cout << "W_dist[" << i << "]=" << (*list)[i] << "; ";
    //     std::cout << "W_index[" << i << "]=" << (*list_index)[i] << std::endl;
    // }
    // list = (std::array<uint32_t, 10> *) Phnsw::Registers.find_match("C_dist[10]", C_dist_size);
    // list_index = (std::array<uint32_t, 10> *) Phnsw::Registers.find_match("C_index[10]", C_index_size);
    // for (size_t i=0; i<10; i++) {
    //     std::cout << "C_dist[10][" << i << "]=" << (*list)[i] << "; ";
    //     std::cout << "C_index[10][" << i << "]=" << (*list_index)[i] << std::endl;
    // }
    // output.verbose(CALL_INFO, 1, 0, "Component is participating in phase %d of complete.\n", phase);
}

/**
 * @description: lifecycle function: finish (unuesd)
 * @return {*}
 */
void Phnsw::finish() {
    std::cout << std::endl;
    // output.verbose(CALL_INFO, 1, 0, "Component is being finished.\n");
}


/**
 * @description: Clock callback function
 * @param {Cycle_t} currentCycle, passed into this function by memory
 * @return {*}
 */
bool Phnsw::clockTick( SST::Cycle_t currentCycle ) {
    timestamp++;
    for (auto &i : inst_struct) {
        // std::cout << "inst: " << i.asmop
        // << " stage_now: " << *i.stage_now
        // << " stages: " << i.stages
        // << std::endl;
        if (*i.stage_now >= i.stages) {
            *i.stage_now = 0;
            if (i.rd != "nord") {
                size_t rd_size;
                void *rd = Registers.find_match(i.rd, rd_size);
                std::memcpy(rd, i.rd_temp, rd_size);
                // if (i.rd == "dist_res") std::cout << "dist_res: " << *(float *)rd << std::endl;
            }
            if (i.rd2 != "nord") {
                size_t rd2_size;
                void *rd2 = Registers.find_match(i.rd2, rd2_size);
                std::memcpy(rd2, i.rd2_temp, rd2_size);
            }
        } else {
            if (*i.stage_now != 0) (*i.stage_now) ++;
        }
    }

    
    Phnsw::inst_count = 0;
    // std::cout << pc << std::endl;
    if (dma->stopFlag == false) {
        inst_now = Phnsw::img[pc];
        for (auto &inst : inst_now) {
            for(auto &&i : inst_struct) {
                if (inst[0].compare(i.asmop) == 0) {
                    (this->*(i.handeler))(i.rd_temp, i.rd2_temp, i.stage_now); // Exe instruction function
                }
            }
            inst_count ++;
        }
        inst_time ++;
        pc ++;
    }
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
    std::cout << "<File: phnswDMA.cc> <Function: phnswDMA::handleEvent()> time=" << getCurrentSimTime()
    << "; respone: " << response->getString()
    << std::endl;
    delete response;
}

const std::vector<Phnsw::InstStruct> Phnsw::inst_struct = {
    {"END", "end the simulation", &Phnsw::inst_end, "nord", "nord", 1},
    {"JMP", "jump to a pc", &Phnsw::inst_jmp, "nord", "nord", 1},
    {"MOV", "move data between regs", &Phnsw::inst_mov, "nord", "nord", 1},
    {"ADD", "add two numbers", &Phnsw::inst_add, "alu_res", "nord", 1},
    {"SUB", "sub two numbers", &Phnsw::inst_sub, "alu_res", "nord", 1},
    {"CMP", "cmp two numbers", &Phnsw::inst_cmp, "cmp_res", "nord", 1},
    {"DIST", "calc distance", &Phnsw::inst_dist, "dist_res", "nord", 1},
    {"LOOK", "look up", &Phnsw::inst_look, "look_res_index", "nord", 1},
    {"PUSH", "push element to list", &Phnsw::inst_push, "nord", "nord", 1},
    {"RMC", "remove element from C", &Phnsw::inst_rmc, "C_dist", "C_index", 8},
    {"RMW", "remove element from W", &Phnsw::inst_rmw, "W_dist", "W_index", 8},
    {"DMA", "Access read from mem", &Phnsw::inst_dma, "nord", "nord", 1},
    {"VST", "Access write to mem", &Phnsw::inst_vst, "vst_res", "nord", 1},
    {"RAW", "Load RAW From SPM to RAW1", &Phnsw::inst_raw, "nord", "nord", 1},
    {"NEI", "Load N[i] from SPM to DAMindex", &Phnsw::inst_nei, "nord", "nord", 1},
    {"INFO", "print reg info", &Phnsw::inst_info, "nord", "nord", 1},
    {"dummy", "dummy inst", &Phnsw::inst_dummy, "nord", "nord", 1}};

int Phnsw::inst_end(void *rd_temp_ptr, void *rd2_temp_ptr, uint32_t *stage_now) {
    std::cout << "pc=" << Phnsw::pc << " " << "inst: " << "END" << std::endl;
    primaryComponentOKToEndSim();
    return 0;
}

int Phnsw::inst_jmp(void *rd_temp_ptr, void *rd2_temp_ptr, uint32_t *stage_now) {
    std::string imm_name = inst_now[inst_count][1];
    int imm;
    if (imm_name.back() == ']' && imm_name[0] == '[') {
        imm = std::stoull(imm_name.substr(1, imm_name.size() - 2));
    } else {
        output.fatal(CALL_INFO, -1, "ERROR: %s", "jmp imm must be in [imm] format");
    }
    uint8_t *cmp_res;
    size_t cmp_size;
    try {
        cmp_res = (uint8_t *) Phnsw::Registers.find_match("cmp_res", cmp_size);
    } catch (char *e) {
        output.fatal(CALL_INFO, -1, "ERROR: %s cmp_res", e);
    }

    if (*cmp_res == 1) {
        std::cout << "jmp to " << imm << std::endl;
        Phnsw::pc = imm;
        Phnsw::pc --;
    } else {
        std::cout << "no jmp" << std::endl;
    }
    return 0;
}

int Phnsw::inst_mov(void *rd_temp_ptr, void *rd2_temp_ptr, uint32_t *stage_now) {
    uint64_t imm;
    std::string src_name = inst_now[inst_count][1];
    std::string rd_name = inst_now[inst_count][2];
    size_t src_size;
    size_t rd_size;
    void* src_ptr;
    void* rd_ptr;
    void* imm_ptr = &imm;
    if (src_name.back() == ']' && src_name[0] == '[') { // is imm
        // std::cout << "is imm" << std::endl;
        src_ptr = imm_ptr;
        src_size = sizeof(imm);
        imm = std::stoull(src_name.substr(1, src_name.size() - 2));
    } else {
        // std::cout << "is reg" << std::endl;
        try {
            src_ptr = Phnsw::Registers.find_match(src_name, src_size);
        } catch (char *e) {
            output.fatal(CALL_INFO, -1, "ERROR: %s %s", e, src_name.c_str());
        }
    }
    try {
        rd_ptr = Phnsw::Registers.find_match(rd_name, rd_size);
    } catch (char *e) {
        output.fatal(CALL_INFO, -1, "ERROR: %s %s", e, rd_name.c_str());
    }
    if (rd_size > src_size) { // if rd_size > src_size, reset rd
        std::memset(rd_ptr, 0, rd_size);
    }
    // std::cout << "pc=" << Phnsw::pc << " ";
    // std::cout << "before rd=" << *(uint32_t *) rd_ptr << "; ";
    // std::cout << "inst: " << "MOV ";
    // std::cout << "reg1: " << inst_now[inst_count][1] << "; ";
    // std::cout << "reg2: " << inst_now[inst_count][2] << "; ";
    // uint8_t temp = 't';
    // std::memcpy(src_ptr, &temp, src_size);
    // std::cout << "src init as " << std::bitset<sizeof(uint32_t) * 8>(*(uint32_t *) src_ptr) << "; ";
    std::memcpy(rd_ptr, src_ptr, min(src_size, rd_size));
    // std::cout << " inst_now length=" << inst_now[inst_count].size() << "; ";
    // std::cout << "after copy rd=" << std::bitset<sizeof(uint8_t) * 8>(*(uint8_t *) rd_ptr) << std::endl;
    return 0;
}

int Phnsw::inst_add(void *rd_temp_ptr, void *rd2_temp_ptr, uint32_t *stage_now) {
    *stage_now = 1;
    uint8_t *src1_ptr, *src2_ptr, *rd_ptr;
    uint8_t imm1, imm2;
    size_t src1_size, src2_size, rd_size;
    std::string src1_name = inst_now[inst_count][1];
    std::string src2_name = inst_now[inst_count][2];
    if (src1_name.back() == ']' && src1_name[0] == '[') {
        imm1 = std::stoull(src1_name.substr(1, src1_name.size() - 2));
        src1_ptr = &imm1; src1_size = sizeof(imm1);
    } else {
        src1_ptr = (uint8_t *) Phnsw::Registers.find_match(src1_name, src1_size);
    }
    if (src2_name.back() == ']' && src2_name[0] == '[') {
        imm2 = std::stoull(src2_name.substr(1, src2_name.size() - 2));
        src2_ptr = &imm1; src2_size = sizeof(imm2);
    } else {
        src2_ptr = (uint8_t *) Phnsw::Registers.find_match(src2_name, src2_size);
    }

    rd_ptr = (uint8_t *) rd_temp_ptr;
    *rd_ptr = *src1_ptr + *src2_ptr;
    return 0;
}

int Phnsw::inst_sub(void *rd_temp_ptr, void *rd2_temp_ptr, uint32_t *stage_now) {
    *stage_now = 1;
    uint8_t *src1_ptr, *src2_ptr, *rd_ptr;
    uint8_t imm1, imm2;
    size_t src1_size, src2_size, rd_size;
    std::string src1_name = inst_now[inst_count][1];
    std::string src2_name = inst_now[inst_count][2];
    if (src1_name.back() == ']' && src1_name[0] == '[') {
        imm1 = std::stoull(src1_name.substr(1, src1_name.size() - 2));
        src1_ptr = &imm1; src1_size = sizeof(imm1);
    } else {
        src1_ptr = (uint8_t *) Phnsw::Registers.find_match(src1_name, src1_size);
    }
    if (src2_name.back() == ']' && src2_name[0] == '[') {
        imm2 = std::stoull(src2_name.substr(1, src2_name.size() - 2));
        src2_ptr = &imm1; src2_size = sizeof(imm2);
    } else {
        src2_ptr = (uint8_t *) Phnsw::Registers.find_match(src2_name, src2_size);
    }

    rd_ptr = (uint8_t *) rd_temp_ptr;
    *rd_ptr = *src1_ptr - *src2_ptr;
    return 0;
}

int Phnsw::inst_cmp(void *rd_temp_ptr, void *rd2_temp_ptr, uint32_t *stage_now) { // TODO test this
    *stage_now = 1;
    uint32_t src1=0, src2=0;
    uint32_t src1_tmp=0, src2_tmp=0;
    void *src1_ptr_8, *src2_ptr_8;
    uint8_t *rd_ptr;
    size_t src1_size=0, src2_size=0, rd_size=0;
    std::string cmp_mode = inst_now[inst_count][1];
    std::string src1_name = inst_now[inst_count][2];
    std::string src2_name = inst_now[inst_count][3];

    if (src1_name.back() == ']' && src1_name[0] == '[') { // is imm
        src1_tmp = std::stoull(src1_name.substr(1, src1_name.size() - 2));
    } else {
        src1_ptr_8 = Phnsw::Registers.find_match(src1_name, src1_size);
        std::memcpy(&src1_tmp, src1_ptr_8, src1_size);
    }
    src1 = (uint32_t) src1_tmp;
    if (src2_name.back() == ']' && src2_name[0] == '[') { // is imm
        src2_tmp = std::stoull(src2_name.substr(1, src2_name.size() - 2));
    } else {
        src2_ptr_8 = Phnsw::Registers.find_match(src2_name, src2_size);
        std::memcpy((void *) &src2_tmp, src2_ptr_8, src2_size);
    }
    src2 = (uint32_t) src2_tmp;

    rd_ptr = (uint8_t *) rd_temp_ptr;
    if (cmp_mode == "EQ") {
        *rd_ptr = (src1 == src2);
    } else if (cmp_mode == "NE") {
        *rd_ptr = (src1 != src2);
    } else if (cmp_mode == "GT") {
        *rd_ptr = (src1 > src2);
    } else if (cmp_mode == "LT") {
        *rd_ptr = (src1 < src2);
    } else if (cmp_mode == "GE") {
        *rd_ptr = (src1 >= src2);
    } else if (cmp_mode == "LE") {
        *rd_ptr = (src1 <= src2);
    }
    return 0;
}

int Phnsw::inst_dist(void *rd_temp_ptr, void *rd2_temp_ptr, uint32_t *stage_now) {
    *stage_now = 1;
    std::array<float,  128> *src1_ptr, *src2_ptr;
    uint32_t *rd_ptr;
    size_t src1_size, src2_size, rd_size;
    src1_ptr = (std::array<float, 128> *) Phnsw::Registers.find_match("raw1", src1_size);
    src2_ptr = (std::array<float, 128> *) Phnsw::Registers.find_match("raw2", src2_size);
    rd_ptr = (uint32_t *) rd_temp_ptr;
    *rd_ptr = 0;
    float dist_tmp = 0;
    for(size_t i=0; i<src1_ptr->size(); i++) {
        float t = src1_ptr->at(i) - src2_ptr->at(i);
        // std::cout << "t^2=" << t * t << "; ";
        dist_tmp += pow(t, 2);
    }
    *rd_ptr = (uint32_t) dist_tmp;
    std::cout << std::endl;
    std::cout << "pc=" << Phnsw::pc << " ";
    std::cout << "inst: " << "DIST" << "; ";
    std::cout << "raw1[0] " << (float) (*src1_ptr)[0] << "; ";
    std::cout << "raw2[0] " << (float) (*src2_ptr)[0] << "; ";
    std::cout << "Value " << *rd_ptr << std::endl;
    return 0;
}

int Phnsw::inst_look(void *rd_temp_ptr, void *rd2_temp_ptr, uint32_t *stage_now) {
    *stage_now = 1;
    size_t list_size, list_index_size;
    std::array<uint32_t, 10> *list = (std::array<uint32_t, 10> *) Phnsw::Registers.find_match("list", list_size);
    std::array<uint32_t, 10> *list_index = (std::array<uint32_t, 10> *) Phnsw::Registers.find_match("list_index", list_index_size);
    uint32_t *rd_index_ptr = (uint32_t *) rd_temp_ptr;
    size_t rd_dist_size;
    uint32_t *rd_dist_ptr = (uint32_t *) Phnsw::Registers.find_match("look_res_dist", rd_dist_size);
    uint32_t res_index;
    if (inst_now[inst_count][1] == "MAX") {
        *rd_dist_ptr = *max_element(list->begin(), list->end());
        res_index = (*list_index)[std::distance(list->begin(), max_element(list->begin(), list->end()))];
    } else if (inst_now[inst_count][1] == "MIN") {
        *rd_dist_ptr = *min_element(list->begin(), list->end());
        res_index = (*list_index)[std::distance(list->begin(), min_element(list->begin(), list->end()))];
    } else {
        output.fatal(CALL_INFO, -1, "ERROR: look mode not found");
    }
    *rd_index_ptr = res_index;
    return 0;
}

int Phnsw::inst_push(void *rd_temp_ptr, void *rd2_temp_ptr, uint32_t *stage_now) {
    size_t src_size;
    std::string src_dist_name = inst_now[inst_count][1];
    std::string src_index_name = inst_now[inst_count][2];
    std::string rd = inst_now[inst_count][3];
    uint32_t *src_dist_ptr;
    try {
        src_dist_ptr = (uint32_t *) Phnsw::Registers.find_match(src_dist_name, src_size);
    } catch (char *e) {
        output.fatal(CALL_INFO, -1, "ERROR: %s %s", e, src_dist_name.c_str());
    }
    uint32_t *src_index_ptr;
    try {
        src_index_ptr = (uint32_t *) Phnsw::Registers.find_match(src_index_name, src_size);
    } catch (char *e) {
        output.fatal(CALL_INFO, -1, "ERROR: %s %s", e, src_index_name.c_str());
    }
    size_t X_size_size;
    uint32_t *X_size;
    try {
        X_size = (uint32_t *) Phnsw::Registers.find_match(rd + "_size", X_size_size);
    } catch (char *e) {
        output.fatal(CALL_INFO, -1, "ERROR: %s %s", e, rd.c_str());
    }
    int max_len = rd == "C" ? 60 : 40;
    std::array<uint32_t, 60> *X_dist, *X_index;
    uint32_t insert_pos = 0;
    if (*X_size < max_len) {
        try {
            X_dist = (std::array<uint32_t, 60> *) Phnsw::Registers.find_match(rd + "_dist", src_size);
            X_index = (std::array<uint32_t, 60> *) Phnsw::Registers.find_match(rd + "_index", src_size);
        } catch (char *e) {
            output.fatal(CALL_INFO, -1, "ERROR: %s %s", e, rd.c_str());
        }
    } else {
        output.fatal(CALL_INFO, -1, "ERROR: %s_size too large", rd.c_str());
    }
    uint32_t new_dist = *src_dist_ptr;
    uint32_t new_index = *src_index_ptr;

    // Find insert pos
    while (insert_pos  < *X_size && (*X_dist)[insert_pos] < new_dist) {
        insert_pos++;
    }
    // Shift elements to make new space for insertion
    for (uint32_t i = insert_pos; i < *X_size; i++) {
        (*X_dist)[i + 1] = (*X_dist)[i];
        (*X_index)[i + 1] = (*X_index)[i];
    }
    // insert at right position
    (*X_dist)[insert_pos] = *((uint32_t *) src_dist_ptr);
    (*X_index)[insert_pos] = *((uint32_t *) src_index_ptr);

    std::cout << "push " << src_dist_name << " = " << *src_dist_ptr << " "
    << src_index_name << " = " << *src_index_ptr << " "
    << rd << " "
    << "insert_pos = " << insert_pos << " "
    << "after insertion dist = " << (*X_dist)[insert_pos] << " "
    << "after insertion index = " << (*X_index)[insert_pos] << " "
    << std::endl;
    *X_size = *X_size + 1;
    return 0;
}

int Phnsw::inst_rmc(void *rd_temp_ptr, void *rd2_temp_ptr, uint32_t *stage_now) {
    *stage_now = 1;
    size_t X_dist_size, X_index_size, idx_size, rd_size, rd2_size;
    std::array<uint32_t, 60> *X_dist_ptr, *X_index_ptr, *rd_ptr, *rd2_ptr;
    std::string idx = inst_now[inst_count][1];
    std::string mode = inst_now[inst_count][2]; // R(eal) or C
    uint32_t index_to_rm;
    try {
        index_to_rm = *(uint32_t *) Phnsw::Registers.find_match(idx, idx_size);
        if (idx_size != sizeof(uint32_t)) {
            output.fatal(CALL_INFO, -1, "ERROR: %s size not match!", idx.c_str());
        }
    } catch (char *e) {
        output.fatal(CALL_INFO, -1, "ERROR: %s %s", e, idx.c_str());
    }
    try {
        X_dist_ptr = (std::array<uint32_t, 60> *) Phnsw::Registers.find_match("C_dist", X_dist_size);
        X_index_ptr = (std::array<uint32_t, 60> *) Phnsw::Registers.find_match("C_index", X_index_size);
    } catch (char *e) {
        output.fatal(CALL_INFO, -1, "ERROR: %s %s", e, "C_index");
    }
    uint32_t *rmc_dist, *rmc_index;
    size_t  rmc_dist_size, rmc_index_size;
    try {
        rmc_dist = (uint32_t *) Phnsw::Registers.find_match("rmc_dist", rmc_dist_size);
        rmc_index = (uint32_t *) Phnsw::Registers.find_match("rmc_index", rmc_index_size);
    } catch (char *e) {
        output.fatal(CALL_INFO, -1, "ERROR: %s%s", e, "rmc_index or rmc_dist not found");
    }
    rd_ptr = (std::array<uint32_t, 60> *) rd_temp_ptr;
    rd2_ptr = (std::array<uint32_t, 60> *) rd2_temp_ptr;
    *rd_ptr = *X_dist_ptr;
    *rd2_ptr = *X_index_ptr;
    size_t loops = 60, target_addr = 60;
    std::cout << "rmc index = " << index_to_rm << std::endl;
    uint32_t index_ref;
    for (size_t i=0; i<loops; i++) {
        if (mode == "C") {
            if (i == index_to_rm) {
                *rmc_dist = (*X_dist_ptr)[i];
                *rmc_index = (*X_index_ptr)[i];
                std::cout  << "rmc index = " << i
                << " rmc dist = " << (*X_dist_ptr)[i]
                << " rmc index = " << (*X_index_ptr)[i]
                << std::endl;
                (*rd_ptr)[i] = 0;
                (*rd2_ptr)[i] = 0;
                target_addr = i;
                break;
            }
        } else if (mode == "R") {
            if ((*X_index_ptr)[i] == index_to_rm) {
                *rmc_dist = (*X_dist_ptr)[i];
                *rmc_index = (*X_index_ptr)[i];
                std::cout  << "rmc index = " << i
                << " rmc dist = " << (*X_dist_ptr)[i]
                << " rmc index = " << (*X_index_ptr)[i]
                << std::endl;
                (*rd_ptr)[i] = 0;
                (*rd2_ptr)[i] = 0;
                target_addr = i;
                break;
            }
        }
    }
    for(size_t i=target_addr; i<loops-1; i++) { // shift left
        (*rd_ptr)[i] = (*rd_ptr)[i+1];
        (*rd2_ptr)[i] = (*rd2_ptr)[i+1];
    }
    (*rd_ptr)[loops - 1] = 0;
    (*rd2_ptr)[loops - 1] = 0;
    uint32_t *X_size = (uint32_t *) Phnsw::Registers.find_match("C_size", rd_size);
    *X_size = *X_size - 1;
    return 0;
}

int Phnsw::inst_rmw(void *rd_temp_ptr, void *rd2_temp_ptr, uint32_t *stage_now) {
    *stage_now = 1;
    size_t X_dist_size, X_index_size, idx_size, rd_size, rd2_size;
    std::array<uint32_t, 40> *X_dist_ptr, *X_index_ptr, *rd_ptr, *rd2_ptr;
    std::string idx = inst_now[inst_count][1];
    std::string mode = inst_now[inst_count][2]; // R(eal) or W
    uint32_t index_to_rm;
    try {
        index_to_rm = *(uint32_t *) Phnsw::Registers.find_match(idx, idx_size);
        if (idx_size != sizeof(uint32_t)) {
            output.fatal(CALL_INFO, -1, "ERROR: %s size not match!", idx.c_str());
        }
    } catch (char *e) {
        output.fatal(CALL_INFO, -1, "ERROR: %s %s", e, idx.c_str());
    }
    try {
        X_dist_ptr = (std::array<uint32_t, 40> *) Phnsw::Registers.find_match("W_dist", X_dist_size);
        X_index_ptr = (std::array<uint32_t, 40> *) Phnsw::Registers.find_match("W_index", X_index_size);
    } catch (char *e) {
        output.fatal(CALL_INFO, -1, "ERROR: %s %s", e, "W_index");
    }
    uint32_t *rmw_dist, *rmw_index;
    size_t  rmw_dist_size, rmw_index_size;
    try {
        rmw_dist = (uint32_t *) Phnsw::Registers.find_match("rmw_dist", rmw_dist_size);
        rmw_index = (uint32_t *) Phnsw::Registers.find_match("rmw_index", rmw_index_size);
    } catch (char *e) {
        output.fatal(CALL_INFO, -1, "ERROR: %s%s", e, "rmw_index or rmw_dist not found");
    }
    rd_ptr = (std::array<uint32_t, 40> *) rd_temp_ptr;
    rd2_ptr = (std::array<uint32_t, 40> *) rd2_temp_ptr;
    *rd_ptr = *X_dist_ptr;
    *rd2_ptr = *X_index_ptr;
    size_t loops = 60, target_addr = 40;
    std::cout << "rmw index = " << index_to_rm << std::endl;
    uint32_t index_ref;
    for (size_t i=0; i<loops; i++) {
        if (mode == "W") {
            if (i == index_to_rm) {
                *rmw_dist = (*X_dist_ptr)[i];
                *rmw_index = (*X_index_ptr)[i];
                std::cout  << "rmw index = " << i
                << " rmw dist = " << (*X_dist_ptr)[i]
                << " rmw index = " << (*X_index_ptr)[i]
                << std::endl;
                (*rd_ptr)[i] = 0;
                (*rd2_ptr)[i] = 0;
                target_addr = i;
                break;
            }
        } else if (mode == "R") {
            if ((*X_index_ptr)[i] == index_to_rm) {
                *rmw_dist = (*X_dist_ptr)[i];
                *rmw_index = (*X_index_ptr)[i];
                std::cout  << "rmw index = " << i
                << " rmw dist = " << (*X_dist_ptr)[i]
                << " rmw index = " << (*X_index_ptr)[i]
                << std::endl;
                (*rd_ptr)[i] = 0;
                (*rd2_ptr)[i] = 0;
                target_addr = i;
                break;
            }
        }
    }
    for(size_t i=target_addr; i<loops-1; i++) { // shift left
        (*rd_ptr)[i] = (*rd_ptr)[i+1];
        (*rd2_ptr)[i] = (*rd2_ptr)[i+1];
    }
    (*rd_ptr)[loops - 1] = 0;
    (*rd2_ptr)[loops - 1] = 0;
    uint32_t *X_size = (uint32_t *) Phnsw::Registers.find_match("W_size", rd_size);
    *X_size = *X_size - 1;
    return 0;
}

int Phnsw::inst_dma(void *rd_temp_ptr, void *rd2_temp_ptr, uint32_t *stage_now) {
    dma->stopFlag = true;
    uint64_t *dma_addr, *dma_size, *rd;
    size_t addr_size, size_size, rd_size;
    try {
        dma_addr = (uint64_t *)Phnsw::Registers.find_match("dma_addr", addr_size);
    }
    catch (char *e) {
        output.fatal(CALL_INFO, -1, "ERROR: %s %s", e, "dma_addr");
    }
    try {
        dma_size = (uint64_t *)Phnsw::Registers.find_match("dma_offset", addr_size);
    }
    catch (char *e) {
        output.fatal(CALL_INFO, -1, "ERROR: %s %s", e, "dma_offset");
    }
    try {
        rd = (uint64_t *)Phnsw::Registers.find_match("dma_res", rd_size);
    } catch (char *e) {
        output.fatal(CALL_INFO, -1, "ERROR: %s %s", e, "dma_res");
    }
    std::string option = inst_now[inst_count][1];
    size_t index_size;
    uint32_t *index = (uint32_t *) Phnsw::Registers.find_match("DMAindex", index_size);
    // std::cout << "DMAindex=" << *index << std::endl;
    if (option == "R") {
        // std::cout << "DMA R" << std::endl;
        *dma_addr = MEM_ADDR_BASE + MEM_RAW_BASE + *index * 128 * 4; // dim = 128
        *dma_size = 128 * 4;
        SST::Interfaces::StandardMem::Addr dstspmAddr = SPM_RAW_BASE;
        dma->DMAget((SST::Interfaces::StandardMem::Addr) *dma_addr,
                        dstspmAddr,
                        (uint32_t) *dma_size);
    } else if (option == "N") {
        // std::cout << "DMA N" << std::endl;
        *dma_addr = MEM_ADDR_BASE + *index * 32 * 4; // neighbor_list.size() = 32
        // std::cout << "dma_addr=" << *dma_addr << std::endl;
        *dma_size = 32 * 4;
        SST::Interfaces::StandardMem::Addr dstspmAddr = 0;
        dma->DMAget((SST::Interfaces::StandardMem::Addr) *dma_addr,
                        dstspmAddr,
                        (uint32_t) *dma_size);
    } else if (option == "A") {
        *dma_addr = *dma_addr;
        std::cout << "time=" << getCurrentSimTime() << " inst=DMA"
        << " size=" << *dma_size << std::endl;
        if (*dma_addr < 1024 /* TODO */ && *dma_size > 2) {
            std::cout << "long read" << std::endl;
            dma->DMAspmrd((SST::Interfaces::StandardMem::Addr) *dma_addr,
                            (size_t) *dma_size,
                            (void *) rd,
                            rd_size);
        } else {
            std::cout << "normal read" << std::endl;
            dma->DMAread((SST::Interfaces::StandardMem::Addr) *dma_addr,
            (size_t) *dma_size,
            (void *) rd, rd_size);
        }
        return 0;
    } else {
        output.fatal(CALL_INFO, -1, "ERROR: %s is invalid dma_option", option.c_str());
    }

    // Read R: mem -> ?
    // Read N: mem -> spm
    std::cout << "time=" << getCurrentSimTime() << " inst=DMA"
    << " dma addr=" << *dma_addr
    << " size=" << *dma_size
    << std::endl;
    return 0;
}

int Phnsw::inst_vst(void *rd_temp_ptr, void *rd2_temp_ptr, uint32_t *stage_now) {
    uint32_t *vst_index;
    uint8_t *vst_res;
    size_t addr_size, res_size;
    try {
        vst_index = (uint32_t *)Phnsw::Registers.find_match("vst_index", addr_size);
    }
    catch (char *e) {
        output.fatal(CALL_INFO, -1, "ERROR: %s %s", e, "vst_index");
    }
    try {
        vst_res = (uint8_t *)Phnsw::Registers.find_match("vst_res", res_size);
    }
    catch (char *e) {
        output.fatal(CALL_INFO, -1, "ERROR: %s %s", e, "vst_res");
    }
    SST::Interfaces::StandardMem::Addr spm_addr = *vst_index / 8;
    uint32_t  spm_offset = *vst_index % 8;
    std::cout << "time=" << getCurrentSimTime()
    << " inst=VST"
    << " addr=" << spm_addr
    << " offset=" << spm_offset
    << std::endl;

    if (inst_now[inst_count][1] == "R") {
        std::cout << "VST R" << std::endl;
        dma->stopFlag = true;
        dma->is_vst = true;
        dma->vst_offset = spm_offset;
        dma->DMAread(spm_addr, 1, (void *) vst_res, res_size);
    } else if (inst_now[inst_count][1] == "W") {
        dma->is_vst = true;
        dma->is_vst_write = true;
        dma->vst_offset = spm_offset;
        int wr_size = 1;
        std::vector<uint8_t> data(wr_size, 0x1 << spm_offset);
        std::cout << "VST W data=" << (uint32_t) data[0] << std::endl;
        dma->DMAvst(spm_addr, 1, vst_res, res_size);
    } else {
        output.fatal(CALL_INFO, -1, "ERROR: vst mode not found");
    }

    return 0;
}

int Phnsw::inst_raw(void *rd_temp_ptr, void *rd2_temp_ptr, uint32_t *stage_now) {
    dma->stopFlag = true;
    size_t rd_size = 0;
    std::array<float, 128> *rd;
    rd = (std::array<float, 128> *) Phnsw::Registers.find_match("raw1", rd_size);

    dma->DMAspmrd(SPM_RAW_BASE, SPM_RAW_SIZE, (void *) rd, rd_size);
    return 0;
}

int Phnsw::inst_nei(void *rd_temp_ptr, void *rd2_temp_ptr, uint32_t *stage_now) {
    dma->stopFlag = true;
    size_t rd_size = 0, i_size = 0;
    uint32_t *rd = (uint32_t *) Phnsw::Registers.find_match("DMAindex", rd_size);
    uint32_t *i = (uint32_t *) Phnsw::Registers.find_match("i", rd_size);
    uint32_t addr_of_nei = SPM_NEIGHBOR_ADDR + (*i * 4);

    dma->DMAread(addr_of_nei, 4, (void *) rd, rd_size);
    return 0;
}

int Phnsw::inst_info(void *rd_temp_ptr, void *rd2_temp_ptr, uint32_t *stage_now) {
    size_t rd_size;
    uint64_t tmp_value;
    void *rd_ptr = Phnsw::Registers.find_match(inst_now[inst_count][1], rd_size);
    std::cout << "pc=" << Phnsw::pc << " ";
    std::cout << "inst: " << "INFO" << "; ";
    std::cout << "RegName: " << inst_now[inst_count][1];
    if (rd_size == sizeof(uint8_t)) {
        tmp_value = (uint64_t) *((uint8_t *) rd_ptr);
    } else if (rd_size == sizeof(uint32_t)) {
        tmp_value = (uint64_t) *((uint32_t *) rd_ptr);
    } else if (rd_size == sizeof(uint64_t)) {
        tmp_value = (uint64_t) *((uint64_t*) rd_ptr);
    } else if (rd_size == sizeof(std::array<uint8_t, 128>)) {
        tmp_value = (uint64_t) (*(std::array<uint8_t, 128> *) rd_ptr)[0];
    } else if (rd_size == sizeof(std::array<uint32_t, 10>)) {
        tmp_value = (uint64_t) (*(std::array<uint32_t, 10> *) rd_ptr)[0];
    }
    std::cout<< ", Value " << std::hex << tmp_value << std::dec << "(" << std::bitset<sizeof(uint64_t) * 8>(tmp_value) << ")";
    std::cout << ", Size " << rd_size
    << std::endl;
    return 0;
}

int Phnsw::inst_dummy(void *rd_temp_ptr, void *rd2_temp_ptr, uint32_t *stage_now) {
    return 0;
}

void Phnsw::load_inst_creat_img() {
    Phnsw::pc = 0; // reset pc
    std::ifstream img_file;
    img_file.open("instructions/dummy2.asm");
    assert(img_file);
    std::string inst_line;
    std::vector<std::vector<std::string>> inst_single_cycle;
    while (std::getline(img_file, inst_line)) {
        std::vector<std::string> inst_this;
        uint8_t word_counts = 0;
        std::stringstream ss(inst_line);
        std::string word;
        std::string operand1, operand2;
        if (inst_line[0] == '\r' | inst_line[0] == ';' | inst_line.empty()) {
            if (inst_single_cycle.empty()) {
                continue; // filter EOF empty lines
            }
            
            Phnsw::img.push_back(inst_single_cycle);
            inst_single_cycle.clear();
            continue; // skip empty line or comment line
        }
        while (ss >> word) {
            if (word.compare(";") == 0 | word.compare("\n") == 0) {
                break; // Remove comments
            } else if (word.compare(",") == 0) {
                continue; // Remove single comma
            } else if (word.back() == ',') {
                word.pop_back(); // Remove str back comma
            }
            inst_this.push_back(word);
            word_counts ++;
        }
        inst_single_cycle.push_back(inst_this);
    }
    if (!inst_single_cycle.empty()) { // if EOF not empty or only 1 empty line
        Phnsw::img.push_back(inst_single_cycle);
    }
    Phnsw::display_img();
    img_file.close();
}

void Phnsw::display_img() {
    size_t display_pc = 0;
    for (auto &&i : Phnsw::img) {
        std::cout << "pc=" <<  display_pc << "; ";
        for (auto &&j : i) {
            for (auto &&word : j) {
                std::cout << word << " ";
            }
            std::cout << "; ";
        }
        display_pc ++;
        std::cout << std::endl;
    }
}