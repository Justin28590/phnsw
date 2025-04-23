import sst
import sys
sys.path.append('../tests/')
from mhlib import componentlist

DEBUG_SCRATCH = 0
DEBUG_MEM = 0

comp_cpu = sst.Component("phnsw", "phnsw.phnsw")
comp_cpu.addParams({
    "printFrequency" : "5",
    "repeats" : "15",
    "scratchSize" : 1024,   # 1K scratch
    "maxAddr" : 4096,       # 4K mem
    "scratchLineSize" : 64,
    "memLineSize" : 64,
    "clock" : "1GHz",
    "maxOutstandingRequests" : 16,
    "maxRequestsPerCycle" : 2,
    "reqsToIssue" : 2,
    "verbose" : 1
    })

dma = comp_cpu.setSubComponent("dma", "phnsw.phnswDMA")
dma.addParams({
    "printFrequency" : "5",
    "repeats" : "15",
    "scratchSize" : 1024,   # 1K scratch
    "maxAddr" : 4096,       # 4K mem
    "scratchLineSize" : 64,
    "memLineSize" : 64,
    "clock" : "1GHz",
    "maxOutstandingRequests" : 16,
    "maxRequestsPerCycle" : 2,
    "reqsToIssue" : 2,
    "verbose" : 1
    })
iface_dma = dma.setSubComponent("memory", "memHierarchy.standardInterface")
comp_scratch_dma = sst.Component("scratch_dma", "memHierarchy.Scratchpad")
comp_scratch_dma.addParams({
    "debug" : DEBUG_SCRATCH,
    "debug_level" : 10,
    "clock" : "2GHz",
    "size" : "1KiB",
    "scratch_line_size" : 64,
    "memory_line_size" : 64,
    "backing" : "malloc",
    "initBacking" : 1
})
scratch_conv_dma = comp_scratch_dma.setSubComponent("backendConvertor", "memHierarchy.simpleMemScratchBackendConvertor")
scratch_back_dma = scratch_conv_dma.setSubComponent("backend", "memHierarchy.simpleMem")
scratch_back_dma.addParams({
    "access_time" : "10ps",
    "mem_size" : "2MiB"
})
scratch_conv_dma.addParams({
    "debug_location" : 0,
    "debug_level" : 10,
})
memctrl_dma = sst.Component("memory0", "memHierarchy.MemController")
memctrl_dma.addParams({
      "debug" : DEBUG_MEM,
      "debug_level" : 10,
      "clock" : "1GHz",
      "addr_range_start" : 0,
      "backing" : "mmap",
      "memory_file" : '../src/datasetx/sift/sift_base.fvecs'
})
memory_dma = memctrl_dma.setSubComponent("backend", "memHierarchy.simpleMem")
memory_dma.addParams({
    "access_time" : "1 ns", # TODO
    "mem_size" : "1024MiB"
})


# Define the simulation links
link_dma_scratch = sst.Link("link_dma_scratch")
link_dma_scratch.connect( (iface_dma, "port", "10ps"), (comp_scratch_dma, "cpu", "10ps") )
link_dma_scratch_mem = sst.Link("link_dma_scratch_mem")
link_dma_scratch_mem.connect( (comp_scratch_dma, "memory", "10ps"), (memctrl_dma, "direct_link", "10ps") )


#########################################################################
## Statistics
#########################################################################

# Enable SST Statistics Outputs for this simulation
# Generate statistics in CSV format
sst.setStatisticOutput("sst.statoutputcsv")

# Send the statistics to a fiel called 'stats.csv'
sst.setStatisticOutputOptions( { "filepath"  : "stats_phnsw.csv" })

# Print statistics of level 5 and below (0-5)
sst.setStatisticLoadLevel(5)

# Enable statistics for all the component
sst.enableAllStatisticsForAllComponents()

print ("\nCompleted configuring the phnsw model\n")

################################ The End ################################
