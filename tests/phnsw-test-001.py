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

iface = comp_cpu.setSubComponent("memory", "memHierarchy.standardInterface")
comp_scratch = sst.Component("scratch", "memHierarchy.Scratchpad")
comp_scratch.addParams({
    "debug" : DEBUG_SCRATCH,
    "debug_level" : 10,
    "clock" : "2GHz",
    "size" : "1KiB",
    "scratch_line_size" : 64,
    "memory_line_size" : 64,
    "backing" : "malloc"
})
scratch_conv = comp_scratch.setSubComponent("backendConvertor", "memHierarchy.simpleMemScratchBackendConvertor")
scratch_back = scratch_conv.setSubComponent("backend", "memHierarchy.simpleMem")
scratch_back.addParams({
    "access_time" : "10ps",
    "mem_size" : "1MiB"
})
scratch_conv.addParams({
    "debug_location" : 0,
    "debug_level" : 10,
})

memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
      "debug" : DEBUG_MEM,
      "debug_level" : 10,
      "clock" : "1GHz",
      "addr_range_start" : 0,
})
memory = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
memory.addParams({
    "access_time" : "1000 ns",
    "mem_size" : "512MiB"
})

# Define the simulation links
link_cpu_scratch = sst.Link("link_cpu_scratch")
link_cpu_scratch.connect( (iface, "port", "1000ps"), (comp_scratch, "cpu", "1000ps") )
link_scratch_mem = sst.Link("link_scratch_mem")
link_scratch_mem.connect( (comp_scratch, "memory", "100ps"), (memctrl, "direct_link", "100ps") )

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
