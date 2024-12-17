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


// This include is ***REQUIRED*** 
// for ALL SST implementation files
// #include "sst_config.h"

#include "phnswDMA.h"


using namespace SST;
using namespace phnsw;

/***********************************************************************************/
// Since the classes are brief, this file has the implementation for all four 
// basicSubComponentAPI subcomponents declared in basicSubComponent_subcomponent.h
/***********************************************************************************/
// phnswDMADRAM

phnswDMADRAM::phnswDMADRAM(ComponentId_t id, Params& params) :
    phnswDMAAPI(id, params) 
{
    amount = params.find<int>("amount",  1);
}

phnswDMADRAM::~phnswDMADRAM() { }

int phnswDMADRAM::compute( int num )
{
    return num + amount;
}

std::string phnswDMADRAM::compute ( std::string comp )
{
    return "(" + comp + ")" + " + " + std::to_string(amount);
}

void phnswDMADRAM::serialize_order(SST::Core::Serialization::serializer& ser) {
    SubComponent::serialize_order(ser);

    SST_SER(amount);
}

