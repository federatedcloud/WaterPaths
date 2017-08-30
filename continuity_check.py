import numpy as np
import pandas as pd
import fnmatch

folder = 'TestFiles/output/'

utilities = pd.read_csv(folder + 'Utilities_s0_r4.out', sep=',')
water_sources = pd.read_csv(folder + 'WaterSources_s0_r4.out', sep=',')

# for h in water_sources.columns.values:
# 	print h

wwater_headers = fnmatch.filter(list(utilities.columns.values), '?wastewater')
wwater_headers += fnmatch.filter(list(utilities.columns.values), '??wastewater')

demand_headers = fnmatch.filter(list(water_sources.columns.values), '?demand')
demand_headers += fnmatch.filter(list(water_sources.columns.values), '??demand')

catch_headers = fnmatch.filter(list(water_sources.columns.values), '?catch_inflow')
catch_headers += fnmatch.filter(list(water_sources.columns.values), '??catch_inflow')

evaporation_headers = fnmatch.filter(list(water_sources.columns.values), '?evap')
evaporation_headers += fnmatch.filter(list(water_sources.columns.values), '??evap')

stored_volume_headers = fnmatch.filter(list(water_sources.columns.values), '?volume')
stored_volume_headers += fnmatch.filter(list(water_sources.columns.values), '??volume')

output = fnmatch.filter(list(water_sources.columns.values), '11ds_spill')

demands = water_sources[demand_headers].sum(axis='columns')
wwater = utilities[wwater_headers].sum(axis='columns')
catch = water_sources[catch_headers].sum(axis='columns')
evap = water_sources[evaporation_headers].sum(axis='columns')
vol = np.append(0, np.diff(water_sources[stored_volume_headers].sum(axis='columns')))
output = water_sources[output].sum(axis='columns')

result = catch - evap - demands + wwater - vol - output

# print water_sources[catch_headers]
for i in range(2, len(output)):
	print str(catch[i]) + '\t\t' + \
		str(demands[i]) + '\t\t' + \
		str(evap[i]) + '\t\t' + \
		str(output[i]) + '\t\t' + \
		str(vol[i]) + '\t\t' + \
		str(result[i]) + '\t\t'
