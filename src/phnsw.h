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

#ifndef _SIMPLE_EXTERNAL_ELEMENT_H
#define _SIMPLE_EXTERNAL_ELEMENT_H

#include <sst/core/component.h>

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
		{ "printFrequency", "How frequently to print a message from the component", "5" },
		{ "repeats", "Number of repetitions to make", "10" },
                {"scratchSize",             "(uint) Size of the scratchpad in bytes"},
                {"maxAddr",                 "(uint) Maximum address to generate (i.e., scratchSize + size of memory)"},
                {"rngseed",                 "(int) Set a seed for the random generator used to create requests", "7"},
                {"scratchLineSize",         "(uint) Line size for scratch, max request size for scratch", "64"},
                {"memLineSize",             "(uint) Line size for memory, max request size for memory", "64"},
                {"clock",                   "(string) Clock frequency in Hz or period in s", "1GHz"},
                {"maxOutstandingRequests",  "(uint) Maximum number of requests outstanding at a time", "8"},
                {"maxRequestsPerCycle",     "(uint) Maximum number of requests to issue per cycle", "2"},
                {"reqsToIssue",             "(uint) Number of requests to issue before ending simulation", "1000"}
        )


        /* Document ports (optional if no ports declared)
         *  Format: { "portname", "description", { "eventtype0", "eventtype1" } }
         */
        SST_ELI_DOCUMENT_PORTS( )

        /* Document statistics (optional if no statistics declared)
         *  Format: { "statisticname", "description", "units", "enablelevel" }
         */
        SST_ELI_DOCUMENT_STATISTICS( )

        /* Document subcomponent slots (optional if no subcomponent slots declared)
         *  Format: { "slotname", "description", "subcomponentAPI" }
         */
        SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS( )

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

private:
	SST::Output output;
	SST::Cycle_t printFreq;
	SST::Cycle_t maxRepeats;
	SST::Cycle_t repeats;

};


#endif
