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
	int hour,							   // (input) current hour of the simulation
	const vector<vector<bool>> &pumps,	   // (input) pump[i][hour] = true if pump i is on in hour
	double &totalCost,					   // (output) total cost of energy consumed by pumps
	vector<vector<double>> &minPressures,  // (output) min pressure at each node in each hour
	vector<vector<double>> &maxPressures,  // (output) max pressure at each node in each hour
	vector<vector<double>> &minTankLevels, // (output) min tank level at each tank in each hour
	vector<vector<double>> &maxTankLevels  // (output) max tank level at each tank in each hour
)
{

	const int numPumps = pumps.size();
	int err = 0; // error code (0 if no error)

	// ... run EPANET for the specified hour

	// You should set the totalCost, minPressures, maxPressures, minTankLevels, and maxTankLevels.
	// By taking the values from the EPANET simulation, you can calculate the pressures and levels ranges.

	return err;
}

void CHK(int err, const std::string &message)
{
	if (err != 0)
	{
		std::cerr << "ERR: " << message << err << std::endl;
		exit(1);
	}
}

int main(int argc, char *argv[])
{
	//... check number of command line arguments
	if (argc < 3)
	{
		std::cout << "\nCorrect syntax is: epanet3 inpFile rptFile (outFile)\n";
		return 0;
	}

	//... retrieve file names from command line
	const char *inpFile = argv[1];
	const char *rptFile = argv[2];
	const char *outFile = "";

	if (argc > 3)
		outFile = argv[3];

	int t = 0, dt = 0;
	EN_Project p = EN_createProject();
	// open the report file
	CHK(EN_openReportFile(rptFile, p), "Open report file");
	// load the project
	CHK(EN_loadProject(inpFile, p), "Load project");			
	// open the output file
	CHK(EN_openOutputFile(outFile, p), "Open output file");
	// initialize the solver
	CHK(EN_initSolver(EN_INITFLOW, p), "Initialize solver");

	do
	{
		// run the solver
		CHK(EN_runSolver(&t, p), "Run solver");

		// Retrieve intermediate results
		int nodeCount, linkCount;		
		CHK(EN_getCount(EN_NODECOUNT, &nodeCount, p), "Get node count");
		CHK(EN_getCount(EN_LINKCOUNT, &linkCount, p), "Get link count");

		std::cout << "================================================" << std::endl;
		std::cout << "Time: " << t << std::endl;
		// loop through all nodes to get pressures
		for (int i = 0; i < nodeCount; i++)
		{
			double pressure;
			CHK(EN_getNodeValue(i, EN_PRESSURE, &pressure, p), "Get node pressure");			
			std::cout << "Node " << i << " Pressure: " << pressure << std::endl;
		}

		// loop through all links to get flows
		for (int i = 0; i < linkCount; i++)
		{
			double flow;
			CHK(EN_getLinkValue(i, EN_FLOW, &flow, p), "Get link flow");
			std::cout << "Link " << i << " Flow: " << flow << std::endl;
		}

		// advance the solver
		CHK(EN_advanceSolver(&dt, p), "Advance solver");	
	} while (dt > 0);

	// Close the solver and write the report
	CHK(EN_writeReport(p), "Write report");
	// delete the project
	EN_deleteProject(p);
	return 0;
}
