// src/CLI/Helper.h
#pragma once

#include "BBConstraints.h"
#include "BBCounter.h"

#include "Core/network.h"
#include "Core/project.h"
#include "Elements/pattern.h"
#include "Elements/pump.h"
#include "Utils.h"
#include "epanet3.h"
#include <map>
#include <string>
#include <vector>

// Function declarations
bool process_node(const char *inpFile, BBCounter &counter, BBConstraints &cntrs, double &cost, bool verbose,
                  bool save_project);
