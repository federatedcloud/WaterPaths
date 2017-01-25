#include <iostream>
#include "WaterSources/Reservoir.h"
#include "Utility.h"
#include "Aux.h"
#include "ContinuityModel.h"


int regionUtilityTwoReservoirs();

using namespace std;
using namespace Constants;

int reservoirAndCatchmentTest() {
    cout << "BEGINNING RESERVOIR TEST" << endl << endl;

    double **streamflows_test = Aux::parse2DCsvFile("/home/bernardo/ClionProjects/TriangleModel/TestFiles/"
                                                            "inflowsLakeTest.csv", 2, 52);
    Catchment catchment_test(streamflows_test[0]);

    Reservoir r(string("Lake Michie"), 0, 0.5, catchment_test, ONLINE, 10.0);

    r.updateAvailableVolume(0, 2.5, 2.0);
    cout << r.getOutflow_previous_week() << " expected: " << 1.0 << endl;
    cout << r.getAvailable_volume() << " expected: " << 10 << endl;

    cout << endl << "END OF RESERVOIR TEST" << endl << "------------------------------------" << endl << endl;

    return 1;
}

int testReadCsv() {

    cout << "BEGINNING CSV READ TEST" << endl << endl;

    // 2D
    cout << "2-D " << endl;
    char const *file_name_2d = "/home/bernardo/ClionProjects/TriangleModel/TestFiles/inflowsLakeTest.csv";
    double **test_data_2d = Aux::parse2DCsvFile(file_name_2d, 2, 5);

    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 4; j++) {
            cout << test_data_2d[i][j] << " ";
        }
        cout << endl;
    }
    cout << endl;

    // 1D
    cout << "1-D " << endl;
    char const *file_name_1d = "/home/bernardo/ClionProjects/TriangleModel/TestFiles/demands.csv";
    double *test_data_1d = Aux::parse1DCsvFile(file_name_1d, 52);

    for (int i = 0; i < 5; i++) {
        cout << test_data_1d[i] << endl;
    }

    cout << endl << "END OF CSV READ TEST" << endl << "------------------------------------" << endl << endl;

    return 0;
}

//int regionContinuityTest() {
//
//    cout << "BEGINNING 2 RESERVOIRS 2 UTILITIES READ TEST" << endl << endl;
//
//    double **streamflows_test = Aux::parse2DCsvFile("/home/bernardo/ClionProjects/TriangleModel/TestFiles/"
//                                                            "inflowsLakeTest.csv", 2, 52);
//
//    int n_weeks = 52;
//
//    Catchment c1(streamflows_test[0]);
//    Catchment c2(streamflows_test[1]);
//
//    Reservoir r1("R1", 0, 1.0, c1, ONLINE, 15.0);
//    Reservoir r2("R2", 1, 0.5, c2, ONLINE, 10.0);
//
//    Utility u1("U1", 0, "/home/bernardo/ClionProjects/TriangleModel/TestFiles/demands.csv", n_weeks);
//    Utility u2("U2", 1, "/home/bernardo/ClionProjects/TriangleModel/TestFiles/demands.csv", n_weeks);
//
//    vector<Reservoir> reservoirs;
//    reservoirs.push_back(r1);
//    reservoirs.push_back(r2);
//
//    vector<Utility> utilities;
//    utilities.push_back(u1);
//    utilities.push_back(u2);
//
//    vector<vector<int>> reservoir_connectivity_matrix = {
//            {0,  1},
//            {-1, 0},
//    };
//
//    vector<vector<int>> reservoir_utility_connectivity_matrix = {
//            {1, 0},
//            {0, 1}
//    };
//
//    ContinuityModel rg(0, reservoirs, reservoir_connectivity_matrix, utilities, reservoir_utility_connectivity_matrix, 4);
//
//    rg.runSimpleContinuitySimulation(4);
//
//    cout << endl << "END OF 2 RESERVOIRS 2 UTILITIES TEST" << endl << "---------------------"
//            "---------------" << endl << endl;
//
//    return 0;
//}

int regionUtilityTwoReservoirs() {

    cout << "BEGINNING 2 RESERVOIRS 1 UTILITY READ TEST" << endl << endl;

    double **streamflows_test = Aux::parse2DCsvFile("/home/bernardo/ClionProjects/TriangleModel/TestFiles/"
                                                            "inflowsLakeTest.csv", 2, 52);
    int n_weeks = 52;

    Catchment c1(streamflows_test[0]);
    Catchment c2(streamflows_test[1]);

    Reservoir r1("R1", 0, 1.0, c1, ONLINE, 15.0);
    Reservoir r2("R2", 1, 0.5, c2, ONLINE, 10.0);

    Utility u1("U1", 0, "/home/bernardo/ClionProjects/TriangleModel/TestFiles/demands.csv", n_weeks);

    vector<Reservoir> reservoirs;
    reservoirs.push_back(r1);
    reservoirs.push_back(r2);

    vector<Utility> utilities;
    utilities.push_back(u1);

    vector<vector<int>> reservoir_connectivity_matrix = {
            {0,  1},
            {-1, 0},
    };

    vector<vector<int>> reservoir_utility_connectivity_matrix = {
            {1, 1},
    };

    ContinuityModel rg(0, reservoirs, reservoir_connectivity_matrix, utilities, reservoir_utility_connectivity_matrix,
                       4);

    rg.runSimpleContinuitySimulation(4);

    cout << "Check results with spreadsheet ContinuityTest.ods" << endl;
    cout << endl << "END OF 2 RESERVOIRS 1 UTILITY TEST" << endl << "---------------------"
            "---------------" << endl << endl;

    return 0;
}


int main() {
//
// Uncomment the lines below to run the tests.
//
//    ::reservoirAndCatchmentTest();
//    ::testReadCsv();
//    ::regionContinuityTest();
    ::regionUtilityTwoReservoirs();


    return 0;
}