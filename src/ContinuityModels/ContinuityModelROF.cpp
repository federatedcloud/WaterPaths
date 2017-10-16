//
// Created by bernardo on 1/26/17.
//

#include <iostream>
#include <algorithm>
#include "ContinuityModelROF.h"
#include "../Utils/Utils.h"

ContinuityModelROF::ContinuityModelROF(
        vector<WaterSource *> water_sources,
        const Graph &water_sources_graph,
        const vector<vector<int>> &water_sources_to_utilities,
        vector<Utility *> utilities,
        vector<MinEnvironFlowControl *> &min_env_flow_controls,
        const unsigned int realization_id) : ContinuityModel(water_sources,
                                                             utilities,
                                                             min_env_flow_controls,
                                                             water_sources_graph,
                                                             water_sources_to_utilities,
                                                             realization_id) {

    /// update utilities' total stored volume
    for (Utility *u : this->continuity_utilities) {
        u->updateTotalStoredVolume();
        u->setNoFinaicalCalculations();
    }

    storage_to_rof_realization = Matrix3D<double>(
            (unsigned int) continuity_utilities.size(),
            (unsigned int) NO_OF_STORAGE_TO_ROF_TABLE_TIERS,
            (unsigned int) ceil(WEEKS_IN_YEAR));
    storage_to_rof_table = new Matrix3D<double>(
            (unsigned int) continuity_utilities.size(),
            (unsigned int) NO_OF_STORAGE_TO_ROF_TABLE_TIERS,
            (unsigned int) ceil(WEEKS_IN_YEAR));

    /// Record which sources have no downstream sources.
    storage_wout_downstream = new bool[sources_topological_order.size()];
    for (int ws : sources_topological_order)
        storage_wout_downstream[ws] = downstream_sources[ws] != NON_INITIALIZED;
}

ContinuityModelROF::ContinuityModelROF(ContinuityModelROF &continuity_model_rof)
        : ContinuityModel(continuity_model_rof.continuity_water_sources,
                          continuity_model_rof.continuity_utilities,
                          continuity_model_rof.min_env_flow_controls,
                          continuity_model_rof.water_sources_graph,
                          continuity_model_rof.water_sources_to_utilities,
                          continuity_model_rof.realization_id) {
}

ContinuityModelROF::~ContinuityModelROF() = default;

/**
 * Runs one the full rof calculations for realization #realization_id for a given week.
 * @param week for which rof is to be calculated.
 */
vector<double> ContinuityModelROF::calculateROF(int week, int rof_type) {
    unsigned long n_utilities = continuity_utilities.size();

    // vector where risks of failure will be stored.
    vector<double> risk_of_failure(n_utilities, 0.0);
    vector<double> year_failure(n_utilities, 0.0);
    bool apply_demand_buffer;

    /// If this is the first week of the year, reset storage-rof table.
    if (rof_type == LONG_TERM_ROF)
       storage_to_rof_table->reset(NON_FAILURE);

    int week_of_the_year = Utils::weekOfTheYear(week);

    /// checks if new infrastructure became available and, if so, set the corresponding realization
    /// infrastructure online.
    updateOnlineInfrastructure(week);

    int n_weeks_rof;
    /// short-term rof calculations.
    if (rof_type == SHORT_TERM_ROF) {
        n_weeks_rof = WEEKS_ROF_SHORT_TERM;
        apply_demand_buffer = !APPLY_DEMAND_BUFFER;
        /// long-term rof calculations.
    } else {
        n_weeks_rof = WEEKS_ROF_LONG_TERM;
        apply_demand_buffer = APPLY_DEMAND_BUFFER;
    }

    /// perform a continuity simulation for NUMBER_REALIZATIONS_ROF (50) yearly realization.
    /// THIS LOOP ONLY CALCULATES BASE ROF - ROF OF CURRENT STORAGE
    for (int yr = 0; yr < NUMBER_REALIZATIONS_ROF; ++yr) {
        /// reset current reservoirs' and utilities' storage and combined storage, respectively,
        /// in the corresponding realization simulation.
        resetUtilitiesAndReservoirs(rof_type);

        for (int w = week; w < week + n_weeks_rof; ++w) {
            /// one week continuity time-step.
            continuityStep(w, yr, apply_demand_buffer);

            /// check total available storage for each utility and, if smaller than the fail ration,
            /// increase the number of failed years of that utility by 1 (FAILURE).
            for (int u = 0; u < n_utilities; ++u) {
                double utility_storage_base_rof = 0;
                double utility_capacity_base_rof = 0;

                for (int ws : non_priority_water_sources_online_to_utilities[u]) {
//                    if (yr == 1 & w == week)
//                        cout << continuity_water_sources[ws]->name << ", "
//                             << continuity_water_sources[ws]->getAllocatedTreatmentCapacity(u)
//                             << "," << continuity_water_sources[ws]->getAvailableAllocatedVolume(u)
//                             << ", " << continuity_water_sources[ws]->getAllocatedCapacity(u) << endl;

                    utility_storage_base_rof += continuity_water_sources[ws]->getAvailableAllocatedVolume(u);
                    utility_capacity_base_rof += continuity_water_sources[ws]->getAllocatedCapacity(u);
                }

                if (utility_storage_base_rof / utility_capacity_base_rof <= STORAGE_CAPACITY_RATIO_FAIL) {
                    year_failure[u] = FAILURE;
//                    if (u == 1) {
//                        cout << "Utility = " << u << ", Realization Week = " << w << ", Real Week = " << week
//                             << ", Realization Year = " << yr
//                             << ", Ratio = " << utility_storage_base_rof / utility_capacity_base_rof << endl;
//                    }
                }
            }
        }

        /// Count failures and reset failures counter.
        for (int uu = 0; uu < n_utilities; ++uu) {
            risk_of_failure[uu] += year_failure[uu];
            year_failure[uu] = NON_FAILURE;
        }
    }

    /// perform a continuity simulation for NUMBER_REALIZATIONS_ROF (50) yearly realization.
    /// ONLY RUN THIS LOOP DURING SHORT-TERM ROF CALCULATIONS
    if (rof_type == SHORT_TERM_ROF) {
        for (int yr = 0; yr < NUMBER_REALIZATIONS_ROF; ++yr) {
            /// RESET REALIZATION-LEVEL FAILURE COUNTER
            storage_to_rof_realization.reset(NON_FAILURE);

            /// reset current reservoirs' and utilities' storage and combined storage, respectively,
            /// in the corresponding realization simulation. For calculating ROF-to-storage table,
            /// begin realization with full reservoirs.
            resetUtilitiesAndReservoirs(LONG_TERM_ROF);

            for (int w = week; w < week + n_weeks_rof; ++w) {
                /// one week continuity time-step.
                continuityStep(w, yr, apply_demand_buffer);

                /// calculated week of storage-rof table
                updateStorageToROFTable(week_of_the_year, week, yr);
            }

            /// update storage-rof table
            *storage_to_rof_table += storage_to_rof_realization / NUMBER_REALIZATIONS_ROF;
        }
    }

//    cout << "Week " << week_of_the_year << endl;
//    storage_to_rof_table->print(week_of_the_year);
//    cout << endl;

    /// Finish ROF calculations
    for (int i = 0; i < n_utilities; ++i) {
        risk_of_failure[i] /= NUMBER_REALIZATIONS_ROF;
//        cout << "Utility = " << i << ", Week = " << week << ", ROF = " << risk_of_failure[i] << endl;
    }

    return risk_of_failure;
}

void ContinuityModelROF::updateStorageToROFTable(int week_of_the_year, int week, int year) {

    int n_water_sources = (int) continuity_water_sources.size();
    int n_utilities = (int) continuity_utilities.size();

    double available_volumes[n_water_sources];
    for (int ws = 0; ws < n_water_sources; ++ws)
        available_volumes[ws] =
                continuity_water_sources[ws]->getAvailable_volume();

    /// loops over the percent storage levels to populate table. The loop
    /// begins from one level above the level  where at least one failure was
    /// observed in the last iteration. This saves a lot of computational time.
//    for (int s = beginning_res_level; s >= 0; --s) {
    for (int s = NO_OF_STORAGE_TO_ROF_TABLE_TIERS; s > -1; --s) {

        double available_volumes_shifted[n_water_sources];
        std::copy(available_volumes,
                  available_volumes + n_water_sources,
                  available_volumes_shifted);

        /// Shift storages.
        shiftStorages(available_volumes_shifted);

        /// Checks for utilities failures.
        int count_fails = 0;
        for (unsigned int u = 0; u < n_utilities; ++u) {
            double utility_storage = 0;
            double utility_capacity = 0;

            /// Calculate combined stored volume for each utility based on shifted storages.
            for (int ws : non_priority_water_sources_online_to_utilities[u]) {
                utility_storage += available_volumes_shifted[ws] *
                        continuity_water_sources[ws]->getAllocatedFraction(u);
                utility_capacity += continuity_water_sources[ws]->getAllocatedCapacity(u);

//                if (week == 51 & year == 1 & s == 10 & u == 1)
//                    cout << continuity_water_sources[ws]->name << ", "
//                         << available_volumes_shifted[ws] * continuity_water_sources[ws]->getAllocatedFraction(u)
//                         << ", " << continuity_water_sources[ws]->getAllocatedCapacity(u) << endl;
            }

            /// Register failure in the table for each utility meeting failure criteria.
            double adjusted_fail_threshold = STORAGE_CAPACITY_RATIO_FAIL +
                    ((double)NO_OF_STORAGE_TO_ROF_TABLE_TIERS - (double) s) / (double)NO_OF_STORAGE_TO_ROF_TABLE_TIERS;

//            if (week == 51 & year == 1 & s == 10)
//                cout << u << ", " << utility_storage << ", " << utility_capacity << endl;

            if ((utility_storage / utility_capacity) <= adjusted_fail_threshold) {
                for (int tier = (s-1); tier > -1; --tier)
                    storage_to_rof_realization(u, tier, week_of_the_year) = FAILURE;
                count_fails++;
            }
        }

        /// If all utilities have failed, stop dropping level.
        if (count_fails == n_utilities) {
            break;
        }
    }
}

/// shiftStorages function that accepts one parameter
/// and calculates estimated week volume plus potential spillage
void ContinuityModelROF::shiftStorages(double *available_volumes_shifted) {
    for (int ws : sources_topological_order) {
        double potential_spillage =
                continuity_water_sources[ws]->getTotal_outflow() -
                continuity_water_sources[ws]->getMin_environmental_outflow();
        double spillage_retrieved = min(potential_spillage,
                                        water_sources_capacities[ws] - available_volumes_shifted[ws]);
        available_volumes_shifted[ws] += spillage_retrieved;
        if (storage_wout_downstream[ws])
            available_volumes_shifted[downstream_sources[ws]] -= spillage_retrieved;
        if (available_volumes_shifted[ws] > water_sources_capacities[ws])
            available_volumes_shifted[ws] = water_sources_capacities[ws];
    }
}


/**
 * reset reservoirs' and utilities' storage and last release, and combined storage, respectively, they
 * currently have in the corresponding realization simulation.
 */
void ContinuityModelROF::resetUtilitiesAndReservoirs(int rof_type) {

    /// update water sources info. If short-term rof, return to current storage; if long-term, make them full.
    if (rof_type == SHORT_TERM_ROF)
        for (int i = 0; i < continuity_water_sources.size();
             ++i) {   // Current available volume
            continuity_water_sources[i]->setAvailableAllocatedVolumes
                    (realization_water_sources[i]->getAvailable_allocated_volumes(),
                     realization_water_sources[i]->getAvailable_volume());
            continuity_water_sources[i]->setOutflow_previous_week(realization_water_sources[i]->getTotal_outflow());
        }
    else
        for (int i = 0; i < continuity_water_sources.size();
             ++i) {   // Full capacity
            continuity_water_sources[i]->setFull();
            continuity_water_sources[i]->setOutflow_previous_week(
                    realization_water_sources[i]->getTotal_outflow());
        }

    /// update utilities combined storage.
    for (Utility *u : this->continuity_utilities) {
        u->updateTotalStoredVolume();
    }
}

/**
 * Pass to the rof continuity model the locations of the utilities of the realization it calculated rofs
 * for. This is done so that the rof calculations can reset the reservoir and utilities states for each of
 * the rofs' year realizations.
 * @param water_sources_realization
 */
void ContinuityModelROF::setRealization_water_sources(
        const vector<WaterSource *> &water_sources_realization) {
    ContinuityModelROF::realization_water_sources = water_sources_realization;
}

/**
 * Checks if new infrastructure became online.
 */
void ContinuityModelROF::updateOnlineInfrastructure(int week) {
    for (int i = 0; i < realization_water_sources.size(); ++i)
        for (int u = 0; u < continuity_utilities.size(); ++u)
            if (realization_water_sources[i]->isOnline() &&
                !continuity_water_sources[i]->isOnline()) {
                for (int u : utilities_to_water_sources[i]) {
                    water_sources_online_to_utilities[u].push_back(i);
                    continuity_utilities[u]
                            ->setWaterSourceOnline((unsigned int) i);
                }
            }


    if (Utils::isFirstWeekOfTheYear(week) || week == 0)
        for (int u = 0; u < continuity_utilities.size(); ++u) {
            utilities_capacities[u] =
                    continuity_utilities[u]->getTotal_storage_capacity();
        }
}

const Matrix3D<double> *ContinuityModelROF::getStorage_to_rof_table() const {
    return storage_to_rof_table;
}

