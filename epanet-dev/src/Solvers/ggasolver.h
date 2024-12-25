/* EPANET 3
 *
 * Copyright (c) 2016 Open Water Analytics
 * Licensed under the terms of the MIT License (see the LICENSE file for details).
 *
 */

//! \file ggasolver.h
//! \brief Describes the GGASolver class.

#ifndef GGASOLVER_H_
#define GGASOLVER_H_

#include "Solvers/hydsolver.h"
#include "Core/hydbalance.h"
#include "Utilities/utilities.h"

#include <vector>
#include <string>
class HydSolver;

//! \class GGASolver
//! \brief A hydraulic solver based on Todini's Global Gradient Algorithm.

class GGASolver : public HydSolver
{
  public:

    GGASolver(Network* nw, MatrixSolver* ms);
    ~GGASolver();
    int solve(double tstep, int& trials);

    void snapshot(std::vector<std::string>& lines) const{
      lines.push_back("{" );
      lines.push_back("\"nodeCount\": " + std::to_string(nodeCount) + ","); 
      lines.push_back("\"linkCount\": " + std::to_string(linkCount) + ",");
      lines.push_back("\"hLossEvalCount\": " + std::to_string(hLossEvalCount) + ",");
      lines.push_back("\"stepSizing\": " + std::to_string(stepSizing) + ",");
      lines.push_back("\"trialsLimit\": " + std::to_string(trialsLimit) + ",");
      lines.push_back("\"reportTrials\": " + std::to_string(reportTrials) + ",");
      lines.push_back("\"headErrLimit\": " + std::to_string(headErrLimit) + ",");
      lines.push_back("\"flowErrLimit\": " + std::to_string(flowErrLimit) + ",");
      lines.push_back("\"flowChangeLimit\": " + std::to_string(flowChangeLimit) + ",");
      lines.push_back("\"flowRatioLimit\": " + std::to_string(flowRatioLimit) + ",");
      lines.push_back("\"tstep\": " + std::to_string(tstep) + ","); 
      lines.push_back("\"theta\": " + std::to_string(theta) + ",");
      lines.push_back("\"errorNorm\": " + std::to_string(errorNorm) + ",");
      lines.push_back("\"oldErrorNorm\": " + std::to_string(oldErrorNorm) + ",");
      snapshot_vector_double(lines, "\"dH\"", &dH[0], dH.size());
      lines.push_back(",");
      snapshot_vector_double(lines, "\"dQ\"", &dQ[0], dQ.size());
      lines.push_back(",");
      snapshot_vector_double(lines, "\"xQ\"", &xQ[0], xQ.size());
      lines.push_back(",");
      lines.push_back("\"hydBalance\":");
      hydBalance.snapshot(lines);      
      lines.push_back("}" );
    }

  private:

    int        nodeCount;         // number of network nodes
    int        linkCount;         // number of network links
    int        hLossEvalCount;    // number of head loss evaluations
    int        stepSizing;        // Newton step sizing method

    int        trialsLimit;       // limit on number of trials
    bool       reportTrials;      // report summary of each trial
    double     headErrLimit;      // allowable head error (ft)
    double     flowErrLimit;      // allowable flow error (cfs)
    double     flowChangeLimit;   // allowable flow change (cfs)
    double     flowRatioLimit;    // allowable total flow change / total flow
    double     tstep;             // time step (sec)
    double     theta;             // time weighting constant

    double     errorNorm;         // solution error norm
    double     oldErrorNorm;      // previous error norm
    HydBalance hydBalance;        // hydraulic balance results

    std::vector<double> dH;       // head change at each node (ft)
    std::vector<double> dQ;       // flow change in each link (cfs)
    std::vector<double> xQ;       // node flow imbalances (cfs)

    // Functions that assemble linear equation coefficients
    void   setFixedGradeNodes();
    void   setMatrixCoeffs();
    void   setLinkCoeffs();
    void   setNodeCoeffs();
    void   setValveCoeffs();

    // Functions that update the hydraulic solution
    int    findHeadChanges();
    void   findFlowChanges();
    double findStepSize(int trials);
    void   updateSolution(double lamda);

    // Functions that check for convergence
    void   setConvergenceLimits();
    double findErrorNorm(double lamda);
    bool   hasConverged();
    bool   linksChangedStatus();
    void   reportTrial(int trials, double lamda);
};

#endif
