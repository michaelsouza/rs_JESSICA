/* EPANET 3
 *
 * Copyright (c) 2016 Open Water Analytics
 * Licensed under the terms of the MIT License (see the LICENSE file for
 * details).
 *
 */

//! \file qualengine.h
//! \brief Describes the QualEngine class.

#ifndef QUALENGINE_H_
#define QUALENGINE_H_

#include <nlohmann/json.hpp> // Include the JSON library
#include <vector>

class Network;
class QualSolver;
// class JuncMixer;
// class TankMixer;

//! \class QualEngine
//! \brief Simulates extended period water quality in a network.
//!
//! The QualEngine class carries out an extended period water quality simulation
//! on a pipe network, calling on its QualSolver object to solve the reaction,
//! transport and mixing equations at each time step.

class QualEngine {
public:
  // Constructor/Destructor

  QualEngine();
  ~QualEngine();

  // Public Methods

  void open(Network *nw);
  void init();
  void solve(int tstep);
  void close();

  //! Serialize to JSON for QualEngine
  nlohmann::json to_json() const {
    return {{"engineState", static_cast<int>(engineState)},
            {"qualTime", qualTime},
            {"qualStep", qualStep},
            {"sortedLinks", sortedLinks},
            {"flowDirection",
             std::vector<char>(flowDirection.begin(), flowDirection.end())}};
  }

  //! Deserialize from JSON for QualEngine
  void from_json(const nlohmann::json &j) {
    engineState = static_cast<EngineState>(j.at("engineState").get<int>());
    qualTime = j.at("qualTime").get<int>();
    qualStep = j.at("qualStep").get<int>();
    sortedLinks = j.at("sortedLinks").get<std::vector<int>>();
    flowDirection = j.at("flowDirection").get<std::vector<char>>();
  }

private:
  // Engine state

  enum EngineState { CLOSED, OPENED, INITIALIZED };
  EngineState engineState;

  // Engine components

  Network *network;       //!< network being analyzed
  QualSolver *qualSolver; //!< single time step water quality solver

  // Engine properties

  int nodeCount;                   //!< number of network nodes
  int linkCount;                   //!< number of network links
  int qualTime;                    //!< current simulation time (sec)
  int qualStep;                    //!< hydraulic time step (sec)
  std::vector<int> sortedLinks;    //!< topologically sorted links
  std::vector<char> flowDirection; //!< direction (+/-) of link flow

  // Simulation sub-tasks

  bool flowDirectionsChanged();
  void setFlowDirections();
  void sortLinks();
  void setSourceQuality();
};

#endif
