/* EPANET 3
 *
 * Copyright (c) 2016 Open Water Analytics
 * Distributed under the MIT License (see the LICENSE file for details).
 *
 */

//! \file main.cpp
//! \brief The main function used to run EPANET from the command line.

#include "epanet3.h"
#include "Core/network.h"
#include "Core/project.h"
#include "Elements/pattern.h"
#include "Elements/pump.h"

#include <iostream>
#include <vector>
#include <algorithm>

using namespace std;
using namespace Epanet;

void CHK(int err, const std::string &message)
{
	if (err != 0)
	{
		std::cerr << "ERR: " << message << err << std::endl;
		exit(1);
	}
}


void calc_actuations_csum(int *actuations_csum, const std::vector<int> &x, int h){
	// Set actuations_csum to 0
	std::fill(actuations_csum, actuations_csum + 3, 0);

	// Calculate the cumulative actuations up to hour h
	for (int i = 1; i < h; i++){
		const int* x_old = &x[3 * (i - 1)];
		const int* x_new = &x[3 * i];		
		for( int j = 0; j < 3; j++){
			if( x_new[j] > x_old[j])
				++actuations_csum[j];
		}
	}
}

bool update_x(std::vector<int> &x, const std::vector<int> &y, int h, int max_actuations, bool verbose = false){
	const int& y_old = y[h - 1];
	const int& y_new = y[h];
	int *x_old = &x[3 * (h - 1)];
	int *x_new = &x[3 * h];

	// Start by copying the previous state
	std::copy(x_old, x_old + 3, x_new);

	if (y_new == y_old){
		return true;
	}

	// Calculate the cumulative actuations up to hour h
	int actuations_csum[3];
	calc_actuations_csum(actuations_csum, x, h);


	// Get the sorted indices of the actuations_csum
	int pumps_sorted[3] = {0, 1, 2};
	std::sort(pumps_sorted, pumps_sorted + 3, [&](int i, int j) { return actuations_csum[i] < actuations_csum[j]; });

	if (verbose){
		std::cout << "h: " << h << ", max_actuations: " << max_actuations << ", y_old: " << y_old << ", y_new: " << y_new << std::endl;
		std::cout << "actuations_csum: [" << actuations_csum[0] << ", " << actuations_csum[1] << ", " << actuations_csum[2] << "]" << std::endl;
		std::cout << "pumps_sorted: [" << pumps_sorted[0] << ", " << pumps_sorted[1] << ", " << pumps_sorted[2] << "]" << std::endl;
	}
	if (y_new > y_old){
		int num_actuations = y_new - y_old;

		if (verbose){
			std::cout << "num_actuations: " << num_actuations << std::endl;
		}
		
		// Identify pumps that are not currently actuating
		for (int i = 0; i < 3 && num_actuations > 0; i++){
			const int pump = pumps_sorted[i];
			if (x_new[pump] == 0){
				if (actuations_csum[pump] >= max_actuations)
					return false;
				x_new[pump] = 1;
				--num_actuations;
			}
		}
		return num_actuations == 0;
	}

	if (y_new < y_old){
		int num_deactuations = y_old - y_new;

		if (verbose){
			std::cout << "num_deactuations: " << num_deactuations << std::endl;
		}

		for (int i = 0; i < 3 && num_deactuations > 0; i++){
			const int pump = pumps_sorted[i];
			if (x_new[pump] == 1){
				x_new[pump] = 0;
				--num_deactuations;
			}
		}
		return num_deactuations == 0;
	}

	return true;
}

void show_pattern(Pattern *p, std::string name){
	std::string type_name;
	switch (p->type)
	{
	case Pattern::FIXED_PATTERN:
		type_name = "FIXED";
		break;
	case Pattern::VARIABLE_PATTERN:
		type_name = "VARIABLE";
		break;
	default:
		type_name = "UNKNOWN";
		break;
	}
	std::cout << name << "[" << type_name << ", " << p->size() << "]: [";
	for (int i = 0; i < p->size(); i++){
		std::cout << p->factor(i) << " ";
	}
	std::cout << "]" << std::endl;
}

void show_x(const std::vector<int> &x, int h){	
	for (int i = 1; i < h; i++){
		const int* xi = &x[3 * i];	
		printf("x[%2d]: (%d, %d, %d)\n", i, xi[0], xi[1], xi[2]);
	}
}

void update_pumps_pattern_speed(std::vector<Pump*> pumps, const std::vector<int> &x, int h){
	for (int i = 1; i < h; i++){
		for(int pump_id = 0; pump_id < pumps.size(); pump_id++){
			Pump *pump = pumps[pump_id];
			FixedPattern *pattern = (FixedPattern*)pump->speedPattern;
			const int* xi = &x[3 * i];
			double factor_new = (double)xi[pump_id];			
			const int factor_id = i - 1; // pattern index is 0-based
			const double factor_old = pattern->factor(factor_id);
			pattern->setFactor(factor_id, factor_new);
			if (factor_new != factor_old){
				std::cout << "h[" << i << "]: pump[" << pump_id << "]: " << factor_old << " -> " << factor_new << std::endl;
			}
		}
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

	EN_Project p = EN_createProject();
	Project* prj = (Project*)p;
	Network* nw = prj->getNetwork();

	// open the report file
	// CHK(EN_openReportFile(rptFile, p), "Open report file");
	// load the project
	CHK(EN_loadProject(inpFile, p), "Load project");			
	// open the output file
	//CHK(EN_openOutputFile(outFile, p), "Open output file");
	// initialize the solver
	CHK(EN_initSolver(EN_INITFLOW, p), "Initialize solver");

	std::vector<std::string> pump_names = {"111", "222", "333"};
	std::vector<Pump*> pumps;
	for (const std::string &name : pump_names){
		Pump* pump = (Pump*)nw->link(name);
		if (pump == nullptr)
			std::cout << "Pump " << name << " not found" << std::endl;
		else
		{
			pumps.push_back(pump);						
		}
	}

	int t = 0, dt = 0, h = 0, max_actuations = 3;
	// Added 0 at the beginning to match the 0-based index of the pattern
	std::vector<int> y = {0, 1, 2, 1, 2, 1, 1, 1, 1, 0, 0, 2, 2, 2, 2, 2, 1, 2, 1, 0, 0, 0, 2, 1, 0};
	std::vector<int> x(3 * y.size(), 0);	

	for(h = 1; h < y.size(); h++){
		update_x(x, y, h, max_actuations, false);
		int *xi = &x[3 * h];
		int sum_xi = xi[0] + xi[1] + xi[2];
		if (sum_xi != y[h]){
			std::cout << "Error: h[" << h << "]: sum_xi[" << sum_xi << "] != y[" << y[h] << "]" << std::endl;
			exit(1);
		}
	}
	update_pumps_pattern_speed(pumps, x, h);

	do
	{
		std::cout << "t: " << t << ", dt: " << dt << std::endl;
		// run the solver
		CHK(EN_runSolver(&t, p), "Run solver");

		// Retrieve intermediate results
		int nodeCount, linkCount;		
		CHK(EN_getCount(EN_NODECOUNT, &nodeCount, p), "Get node count");
		CHK(EN_getCount(EN_LINKCOUNT, &linkCount, p), "Get link count");
		
		// loop through all nodes to get pressures
		for (int i = 0; i < nodeCount; i++)
		{
			double pressure;
			CHK(EN_getNodeValue(i, EN_PRESSURE, &pressure, p), "Get node pressure");			
			// std::cout << "Node " << i << " Pressure: " << pressure << std::endl;
		}

		// loop through all links to get flows
		for (int i = 0; i < linkCount; i++)
		{
			double flow;
			CHK(EN_getLinkValue(i, EN_FLOW, &flow, p), "Get link flow");
			// std::cout << "Link " << i << " Flow: " << flow << std::endl;
		}	

		// advance the solver
		CHK(EN_advanceSolver(&dt, p), "Advance solver");
		
		// if(t > 7200) break;

	} while (dt > 0);

	double totalCost = 0;
	for (int i = 0; i < pumps.size(); i++){
		totalCost += pumps[i]->pumpEnergy.getCost();		
	}
	std::cout << "Total cost: " << totalCost << std::endl;	
	
	// delete the project
	EN_deleteProject(p);
	return EXIT_SUCCESS;
}
