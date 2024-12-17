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

using namespace SST;
using namespace phnsw;

/***********************************************************************************/
// Since the classes are brief, this file has the implementation for all four 
// basicSubComponentAPI subcomponents declared in basicSubComponent_subcomponent.h
/***********************************************************************************/
// phnswDMADRAM

phnswDMA::phnswDMA(ComponentId_t id, Params& params) :
    phnswDMAAPI(id, params) 
{
    amount = params.find<int>("amount",  1);
}

phnswDMA::~phnswDMA() { }

void phnswDMA::DMAread(SST::Interfaces::StandardMem::Addr addr, size_t size)
{
    std::cout << "<File: phnswDMA.cc> <Function: phnswDMA::DMAread()> DMA read called with addr " << addr << " and size " << size << std::endl;
}

void phnswDMA::serialize_order(SST::Core::Serialization::serializer& ser) {
    SubComponent::serialize_order(ser);

    SST_SER(amount);
}

