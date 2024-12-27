/* EPANET 3
 *
 * Copyright (c) 2016 Open Water Analytics
 * Licensed under the terms of the MIT License (see the LICENSE file for details).
 *
 */

//! \file hydsolver.h
//! \brief Describes the HydSolver class.

#ifndef HYDSOLVER_H_
#define HYDSOLVER_H_

#include "Solvers/matrixsolver.h"
#include <string>
#include <vector>

class Network;
class MatrixSolver;

//! \class HydSolver
//! \brief Interface for an equilibrium network hydraulic solver.
//!
//! This is an abstract class that defines an interface for a
//! specific algorithm used for solving pipe network hydraulics at a
//! given instance in time.

class HydSolver
{
  public:

    enum StatusCode {
        SUCCESSFUL,
        FAILED_NO_CONVERGENCE,
        FAILED_ILL_CONDITIONED
    };

    HydSolver(Network* nw, MatrixSolver* ms);
    virtual ~HydSolver();
    static  HydSolver* factory(const std::string name, Network* nw, MatrixSolver* ms);
    virtual int solve(double tstep, int& trials) = 0;
    
//! Serialize to JSON for HydSolver
virtual nlohmann::json to_json() const {
    return {
        {"matrixSolver", matrixSolver ? matrixSolver->to_json() : nullptr}
    };
}

//! Deserialize from JSON for HydSolver
virtual void from_json(const nlohmann::json& j) {
    if (!j.at("matrixSolver").is_null()) {
        matrixSolver->from_json(j.at("matrixSolver"));
    }
}


  protected:

    Network*       network;
    MatrixSolver*  matrixSolver;

};

#endif
