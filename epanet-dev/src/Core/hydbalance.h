/* EPANET 3
 *
 * Copyright (c) 2016 Open Water Analytics
 * Licensed under the terms of the MIT License (see the LICENSE file for details).
 *
 */

//! \file hydbalance.h
//! \brief Describes the HydBalance class.

#ifndef HYDBALANCE_H_
#define HYDBALANCE_H_

#include <string>
#include <vector>

class Network;

//! \class HydBalance
//! \brief Computes the degree to which a network solution is unbalanced.
//!
//! The HydBalance class determines the error in satisfying the head loss
//! equation across each link and the flow continuity equation at each node
//! of the network for an incremental change in nodal heads and link flows.

struct HydBalance
{
    double    maxFlowErr;         //!< max. flow error (cfs)
    double    maxHeadErr;         //!< max. head loss error (ft)
    double    maxFlowChange;      //!< max. flow change (cfs)
    double    totalFlowChange;    //!< (summed flow changes) / (summed flows)

    int       maxHeadErrLink;     //!< link with max. head loss error
    int       maxFlowErrNode;     //!< node with max. flow error
    int       maxFlowChangeLink;  //!< link with max. flow change

    double    evaluate(
                  double lamda, double dH[], double dQ[], double xQ[], Network* nw);
    double    findHeadErrorNorm(
                  double lamda, double dH[], double dQ[], double xQ[], Network* nw);
    double    findFlowErrorNorm(double xQ[], Network* nw);

    void snapshot(std::vector<std::string>& lines) const {
        lines.push_back("{");
        lines.push_back("\"maxFlowErr\": " + std::to_string(maxFlowErr) + ",");
        lines.push_back("\"maxHeadErr\": " + std::to_string(maxHeadErr) + ",");
        lines.push_back("\"maxFlowChange\": " + std::to_string(maxFlowChange) + ",");
        lines.push_back("\"totalFlowChange\": " + std::to_string(totalFlowChange) + ",");
        lines.push_back("\"maxHeadErrLink\": " + std::to_string(maxHeadErrLink) + ",");
        lines.push_back("\"maxFlowErrNode\": " + std::to_string(maxFlowErrNode) + ",");
        lines.push_back("\"maxFlowChangeLink\": " + std::to_string(maxFlowChangeLink));
        lines.push_back("}");
    }
};

#endif
