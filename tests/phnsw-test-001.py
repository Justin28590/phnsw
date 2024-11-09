import sst

obj = sst.Component("phnsw", "phnsw.phnsw")
obj.addParams({
    "printFrequency" : "5",
    "repeats" : "15"
    })

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
