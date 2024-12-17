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

#ifndef _PHNSWDMAAPI_SUBCOMPONENT_H
#define _PHNSWDMAAPI_SUBCOMPONENT_H

/*
 * This is an example of a simple subcomponent that can take a number and do a computation on it. 
 * This file happens to have multiple classes declaring both the SubComponentAPI 
 * as well as a few subcomponents that implement the API.
 *  
 * See 'basicSubComponent_component.h' for more information on how the example simulation works
 */
    
#include <sst/core/subcomponent.h>
#include <sst/core/interfaces/stdMem.h>

namespace SST {
namespace phnsw {

/*****************************************************************************************************/

// SubComponent API for a SubComponent that has two functions:
// 1) It does a specific computation on a number
//      Ex. Given 5, if the computation is 'x4', the subcomponent should return 20
// 2) Given a formula string, it will update the formula with the computation it does
//      Ex. Given '3+2', if the computation is 'x4', the subcomponent should return '(3+2)x4'

class phnswDMAAPI : public SST::SubComponent
{
public:
    /* 
     * Register this API with SST so that SST can match subcomponent slots to subcomponents 
     */
    SST_ELI_REGISTER_SUBCOMPONENT_API(SST::phnsw::phnswDMAAPI)
    
    phnswDMAAPI(ComponentId_t id, Params& params) : SubComponent(id) { }
    virtual ~phnswDMAAPI() { }

    // These are the two functions described in the comment above
    virtual void DMAread(SST::Interfaces::StandardMem::Addr addr, size_t size) =0;

    // Serialization
    phnswDMAAPI();
    ImplementVirtualSerializable(SST::phnsw::phnswDMAAPI);
};

/*****************************************************************************************************/

/* SubComponent that does an 'increment' computation */
class phnswDMA : public phnswDMAAPI {
public:
    
    // Register this subcomponent with SST and tell SST that it implements the 'phnswDMAAPI' API
    SST_ELI_REGISTER_SUBCOMPONENT(
            phnswDMA,     // Class name
            "phnsw",         // Library name, the 'lib' in SST's lib.name format
            "phnswDMA",   // Name used to refer to this subcomponent, the 'name' in SST's lib.name format
            SST_ELI_ELEMENT_VERSION(1,0,0), // A version number
            "SubComponent DMA that READ from DRAM", // Description
            SST::phnsw::phnswDMAAPI // Fully qualified name of the API this subcomponent implements
                                                            // A subcomponent can implment an API from any library
            )

    // Other ELI macros as needed for parameters, ports, statistics, and subcomponent slots
    SST_ELI_DOCUMENT_PARAMS( { "amount", "Amount to increment by", "1" } )

    phnswDMA(ComponentId_t id, Params& params);
    ~phnswDMA();

    void DMAread(SST::Interfaces::StandardMem::Addr addr, size_t size) override;
    void serialize_order(SST::Core::Serialization::serializer& ser) override;

private:
    int amount;
};

} } /* Namspaces */

#endif /* _phnswDMAAPI_SUBCOMPONENT_H */
