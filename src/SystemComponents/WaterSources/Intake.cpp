//
// Created by bernardo on 2/10/17.
//

#include "../../Utils/Constants.h"
#include "Intake.h"

Intake::Intake(const string &source_name, const int id, const double min_environmental_outflow,
               const vector<Catchment *> &catchments, bool online, double max_treatment_capacity)
        : WaterSource(source_name, id, min_environmental_outflow, catchments,
                      online, NONE, max_treatment_capacity, INTAKE) {

    /// Update total catchment inflow, demand, and available water volume for week 0;
    this->upstream_catchment_inflow = 0;
    for (Catchment *c : catchments) {
        this->upstream_catchment_inflow = c->getStreamflow(0);
    }

    demand = 0;
    available_volume = this->upstream_catchment_inflow - min_environmental_outflow;
}

/**
 *
 * @param intake
 */
Intake::Intake(const Intake &intake) : WaterSource(intake) {}

/**
 *
 * @param intake
 * @return
 */
Intake &Intake::operator=(const Intake &intake) {
    WaterSource::operator=(intake);
    return *this;
}

/**
 * Destructor.
 */
Intake::~Intake() {
    catchments.clear();
}

/**
 * Calculates the water volume that will be available for the next time step, as well as calculates continuity for the
 * current one. The update of the total outflow for the current time step is performed with simple mass
 * balance: outflow equals the sum of the inflows. The water volume that will be for sure available for the next time
 * step is the catchment inflows of next week plus the upstream sources' minimum environmental inflow minus the intake's
 * own environmental inflow. The actually available volume may be greater is the reservoirs above are overflowing, but
 * since the spillover for the next time step hasn't been calculated yet, this value cannot be used. This doesn't matter
 * anyways because if the reservoir is overflowing -- there is no need to say that intake will have a greater water
 * availability to save water from the upstream reservoirs if they themselves are overflowing.
 *
 * It may also look that the code assumes the intake operators know what the inflow will be in the following week. This
 * is not a problem because this information is actually used in the following week, when they are measuring the inflow
 * of the stream where the intake is located.
 *
 * @param week
 * @param upstream_source_inflow upstream sources current outflow.
 * @param demand current demand
 */
void Intake::applyContinuity(int week, double upstream_source_inflow, double demand) {

    /// Get all upstream catchment inflow.
    upstream_catchment_inflow = 0;
    for (Catchment *c : catchments)
        upstream_catchment_inflow += c->getStreamflow(week);

    /// Mass balance for current time step.
    this->total_outflow = upstream_source_inflow + upstream_catchment_inflow - demand;

    /// Water availability for next ime step.
    double next_upstream_catchment_inflow = 0;
    for (Catchment *c : catchments)
        next_upstream_catchment_inflow += c->getStreamflow(week + 1);

    this->available_volume = upstream_min_env_inflow + next_upstream_catchment_inflow - min_environmental_outflow;

    /// Records for the sake of output.
    this->demand = demand;
    this->upstream_source_inflow = upstream_source_inflow;
}

void Intake::setUpstream_min_env_flow(double upstream_min_env_flow) {
    this->upstream_min_env_inflow = upstream_min_env_flow;
}