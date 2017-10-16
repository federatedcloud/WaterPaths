//
// Created by bernardo on 1/26/17.
//

#ifndef TRIANGLEMODEL_CONTINUITYMODELROF_H
#define TRIANGLEMODEL_CONTINUITYMODELROF_H


#include "Base/ContinuityModel.h"
#include "../Utils/Matrices.h"

class ContinuityModelROF : public ContinuityModel {
private:
    Matrix3D<double> *storage_to_rof_table;
    Matrix3D<double> storage_to_rof_realization;
    bool *storage_wout_downstream;

protected:
    int beginning_res_level;
    vector<WaterSource *> realization_water_sources;

public:
    ContinuityModelROF(
            vector<WaterSource *> water_sources,
            const Graph &water_sources_graph,
            const vector<vector<int>> &water_sources_to_utilities,
            vector<Utility *> utilities,
            vector<MinEnvironFlowControl *> &min_env_flow_controls,
            const unsigned int realization_id);

    ContinuityModelROF(ContinuityModelROF &continuity_model_rof);

    vector<double> calculateROF(int week, int rof_type);

    void resetUtilitiesAndReservoirs(int rof_type);

    void setRealization_water_sources(const vector<WaterSource *> &water_sources_realization);

    void updateOnlineInfrastructure(int week);

    virtual ~ContinuityModelROF();

    void updateStorageToROFTable(int week_of_the_year, int week, int year);

    void shiftStorages(double *storages);

    const Matrix3D<double> *getStorage_to_rof_table() const;
};


#endif //TRIANGLEMODEL_CONTINUITYMODELROF_H
