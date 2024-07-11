/* EPANET 3
 *
 * Copyright (c) 2016 Open Water Analytics
 * Distributed under the MIT License (see the LICENSE file for details).
 *
 */

 //! \file main.cpp
 //! \brief The main function used to run EPANET from the command line.

#include "epanet3.h"

#include <iostream>
#include <vector>

using namespace std;

static int call_epanet(
	int hour, // (input) current hour of the simulation
	const vector<vector<bool>>& pumps, // (input) pump[i][hour] = true if pump i is on in hour
	double& totalCost, // (output) total cost of energy consumed by pumps
	vector<vector<double>>& minPressures, // (output) min pressure at each node in each hour
	vector<vector<double>>& maxPressures, // (output) max pressure at each node in each hour
	vector<vector<double>>& minTankLevels, // (output) min tank level at each tank in each hour
	vector<vector<double>>& maxTankLevels // (output) max tank level at each tank in each hour
) {

	const int numPumps = pumps.size();
	int err = 0; // error code (0 if no error)

	// ... run EPANET for the specified hour


	// You should set the totalCost, minPressures, maxPressures, minTankLevels, and maxTankLevels.
	// By taking the values from the EPANET simulation, you can calculate the pressures and levels ranges.

	return err;
}

int main(int argc, char* argv[])
{
	//... check number of command line arguments
	if (argc < 3)
	{
		std::cout << "\nCorrect syntax is: epanet3 inpFile rptFile (outFile)\n";
		return 0;
	}

	//... retrieve file names from command line
	const char* inpFile = argv[1];
	const char* rptFile = argv[2];
	const char* outFile = "";

	if (argc > 3) outFile = argv[3];

	int t = 0, dt = 0;
	EN_Project p = EN_createProject();
	EN_openReportFile(rptFile, p);
	EN_loadProject(inpFile, p);
	EN_openOutputFile("", p);
	EN_initSolver(EN_NOINITFLOW, p);
	do {
		EN_runSolver(&t, p);
		EN_advanceSolver(&dt, p);
	} while (dt > 0);
	EN_writeReport(p);
	EN_deleteProject(p);
	
	return 0;
}
