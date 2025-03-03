/*
 * @Author: Zeng GuangYi tgy_scut2021@outlook.com
 * @Date: 2024-11-10 00:22:53
 * @LastEditors: Zeng GuangYi tgy_scut2021@outlook.com
 * @LastEditTime: 2025-03-03 13:05:38
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

    // Initialize local variables
    Phnsw::timestamp = 0;
    Phnsw::num_events_issued = Phnsw::num_events_returned = 0;

    Params dmaparams;

    dma = loadUserSubComponent<phnswDMAAPI>("dma", SST::ComponentInfo::SHARE_NONE/* , dmaparams */, clockTC);

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
    size_t raw1_size, raw2_size;
    std::array<uint8_t, 128> *raw1 = (std::array<uint8_t, 128> *) Phnsw::Registers.find_match("raw1", raw1_size);
    std::array<uint8_t, 128> *raw2 = (std::array<uint8_t, 128> *) Phnsw::Registers.find_match("raw2", raw2_size);
    for (auto &&i : *raw1) {
        i = 0;
    }
    for (auto &&i : *raw2) {
        i = 10;
    }
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
        if (i.rd != "nord") {
            // std::cout << "pc=" << Phnsw::pc << " ";
            if (*i.stage_now == i.stages) {
                // std::cout << "pipeline end";
                size_t rd_size;
                void *rd = Registers.find_match(i.rd, rd_size);
                std::memcpy(rd, i.rd_temp, rd_size);
                *i.stage_now = 0;
            } else if (*i.stage_now != 0) {
                // std::cout << "pipeline stage " << *i.stage_now;
                *i.stage_now = *i.stage_now + 1;
            }
            // std::cout << std::endl;
        }
    }

    Phnsw::inst_count = 0;
    inst_now = Phnsw::img[pc];
    for (auto &inst : inst_now) {
        for(auto &&i : inst_struct) {
            if (inst[0].compare(i.asmop) == 0) {
                (this->*(i.handeler))(i.rd_temp, i.stage_now); // Exe instruction function
            }
        }
        inst_count ++;
    }
    inst_time ++;
    pc ++;
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

const std::vector<Phnsw::InstStruct> Phnsw::inst_struct = {
    {"END",     "end the simulation",       &Phnsw::inst_end,   "nord",     1},
    {"MOV",     "move data between regs",   &Phnsw::inst_mov,   "nord",     1},
    {"ADD",     "add two numbers",          &Phnsw::inst_add,   "alu_res",  1},
    {"CMP",     "cmp two numbers",          &Phnsw::inst_cmp,   "cmp_res",  1},
    {"DIST",    "calc distance",            &Phnsw::inst_dist,  "dist_res", 1},
    {"INFO",    "print reg info" ,          &Phnsw::inst_info,  "nord",     1},
    {"dummy",   "dummy inst",               &Phnsw::inst_dummy, "nord",     1}
};

int Phnsw::inst_end(void *rd_temp_ptr, uint32_t *stage_now) {
    std::cout << "pc=" << Phnsw::pc << " " << "inst: " << "END" << std::endl;
    primaryComponentOKToEndSim();
    return 0;
}

int Phnsw::inst_mov(void *rd_temp_ptr, uint32_t *stage_now) {
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

int Phnsw::inst_add(void *rd_temp_ptr, uint32_t *stage_now) {
    *stage_now = 1;
    uint8_t *src1_ptr, *src2_ptr, *rd_ptr;
    size_t src1_size, src2_size, rd_size;
    src1_ptr = (uint8_t *) Phnsw::Registers.find_match("num1", src1_size);
    src2_ptr = (uint8_t *) Phnsw::Registers.find_match("num2", src2_size);
    // rd_ptr = (uint8_t *) Phnsw::Registers.find_match("alu_res", rd_size);
    rd_ptr = (uint8_t *) rd_temp_ptr;
    // std::cout << "pc=" << Phnsw::pc << " ";
    // std::cout << "before rd=" << std::bitset<sizeof(uint8_t) * 8>(*(uint8_t *) rd_ptr) << "; ";
    // std::cout << "inst: " << "ADD ";
    // std::cout << "reg1: num1; ";
    // std::cout << "reg2: num2; ";
    // std::cout << "rd: rd_temp; ";
    *rd_ptr = *src1_ptr + *src2_ptr;
    // std::cout << "after add rd=" << std::bitset<sizeof(uint8_t) * 8>(*(uint8_t *) rd_ptr) << std::endl;
    return 0;
}

int Phnsw::inst_cmp(void *rd_temp_ptr, uint32_t *stage_now) {
    *stage_now = 1;
    uint32_t src1, src2;
    uint8_t src1_tmp, src2_tmp;
    uint8_t *src1_ptr_8, *src2_ptr_8, *rd_ptr;
    size_t src1_size, src2_size, rd_size;
    std::string cmp_mode = inst_now[inst_count][1];
    std::string src1_name = inst_now[inst_count][2];
    std::string src2_name = inst_now[inst_count][3];

    src1_ptr_8 = (uint8_t *) Phnsw::Registers.find_match(src1_name, src1_size);
    src1_tmp = *src1_ptr_8;
    src1 = (uint32_t) src1_tmp;
    src2_ptr_8 = (uint8_t *) Phnsw::Registers.find_match(src2_name, src2_size);
    src2_tmp = *src2_ptr_8;
    src2 = (uint32_t) src2_tmp;

    rd_ptr = (uint8_t *) rd_temp_ptr;
    if (cmp_mode == "EQ") {
        *rd_ptr = (src1_tmp == src2_tmp);
    } else if (cmp_mode == "NE") {
        *rd_ptr = (src1_tmp != src2_tmp);
    } else if (cmp_mode == "GT") {
        *rd_ptr = (src1_tmp > src2_tmp);
    } else if (cmp_mode == "LT") {
        *rd_ptr = (src1_tmp < src2_tmp);
    } else if (cmp_mode == "GE") {
        *rd_ptr = (src1_tmp >= src2_tmp);
    } else if (cmp_mode == "LE") {
        *rd_ptr = (src1_tmp <= src2_tmp);
    }
    return 0;
}

int Phnsw::inst_dist(void *rd_temp_ptr, uint32_t *stage_now) {
    *stage_now = 1;
    std::array<uint8_t,  128> *src1_ptr, *src2_ptr;
    uint32_t *rd_ptr;
    size_t src1_size, src2_size, rd_size;
    src1_ptr = (std::array<uint8_t, 128> *) Phnsw::Registers.find_match("raw1", src1_size);
    src2_ptr = (std::array<uint8_t, 128> *) Phnsw::Registers.find_match("raw2", src2_size);
    rd_ptr = (uint32_t *) rd_temp_ptr;
    *rd_ptr = 0;
    for(size_t i=0; i<src1_ptr->size(); i++) {
        *rd_ptr += ((*src1_ptr)[i] * (*src1_ptr)[i] >> 1)
                 - (*src2_ptr)[i] * (*src1_ptr)[i]
                 + ((*src2_ptr)[i] * (*src2_ptr)[i] >> 1);
    }
    std::cout << "pc=" << Phnsw::pc << " ";
    std::cout << "inst: " << "DIST" << "; ";
    std::cout << "raw1[0]" << (uint32_t) (*src1_ptr)[0] << "; ";
    std::cout << "raw2[0]" << (uint32_t) (*src2_ptr)[0] << "; ";
    std::cout << "Value " << *rd_ptr << std::endl;
    return 0;
}

int Phnsw::inst_info(void *rd_temp_ptr, uint32_t *stage_now) {
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
        std::cout<< ", Value " << (uint64_t) *((uint64_t *) rd_ptr);
    } else if (rd_size == sizeof(std::array<uint8_t, 128>)) {
        tmp_value = (uint64_t) (*(std::array<uint8_t, 128> *) rd_ptr)[0];
    } else if (rd_size == sizeof(std::array<uint32_t, 10>)) {
        tmp_value = (uint64_t) (*(std::array<uint32_t, 10> *) rd_ptr)[0];
    }
    std::cout<< ", Value " << tmp_value << "(" << std::bitset<sizeof(uint64_t) * 8>(tmp_value) << ")";
    std::cout << ", Size " << rd_size
    << std::endl;
    return 0;
}

int Phnsw::inst_dummy(void *rd_temp_ptr, uint32_t *stage_now) {
    return 0;
}

void Phnsw::load_inst_creat_img() {
    Phnsw::pc = 0; // reset pc
    std::ifstream img_file;
    img_file.open("instructions/dummy.asm");
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
    size_t display_time = 0;
    for (auto &&i : Phnsw::img) {
        std::cout << "time=" <<  display_time << "; ";
        for (auto &&j : i) {
            for (auto &&word : j) {
                std::cout << word << " ";
            }
            std::cout << "; ";
        }
        display_time ++;
        std::cout << std::endl;
    }
}