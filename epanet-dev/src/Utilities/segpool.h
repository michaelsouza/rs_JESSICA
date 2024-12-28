/* EPANET 3
 *
 * Copyright (c) 2016 Open Water Analytics
 * Licensed under the terms of the MIT License (see the LICENSE file for
 * details).
 *
 */

//! \file segpool.h
//! \brief Describes the SegPool class used for water quality transport.

#ifndef SEGPOOL_H_
#define SEGPOOL_H_

#include <nlohmann/json.hpp>

class MemPool;

struct Segment //!< Volume segment
{
  double v;             //!< volume (ft3)
  double c;             //!< constituent concentration (mass/ft3)
  struct Segment *next; //!< next upstream volume segment

  //! Serialize to JSON for Segment
  nlohmann::json to_json() const {
    return {{"v", v}, {"c", c}, {"next", next ? next->to_json() : nullptr}};
  }

  //! Deserialize from JSON for Segment
  void from_json(const nlohmann::json &j) {
    v = j.at("v").get<double>();
    c = j.at("c").get<double>();

    if (!j.at("next").is_null()) {
      if (!next)
        next = new Segment();
      next->from_json(j.at("next"));
    } else {
      delete next;
      next = nullptr;
    }
  }
};

class SegPool {
public:
  SegPool();
  ~SegPool();
  void init();
  Segment *getSegment(double v, double c);
  void freeSegment(Segment *seg);

  //! Serialize to JSON for SegPool
  nlohmann::json to_json() const {
    return {{"segCount", segCount},
            {"freeSeg", freeSeg ? freeSeg->to_json() : nullptr}};
  }

  //! Deserialize from JSON for SegPool
  void from_json(const nlohmann::json &j) {
    segCount = j.at("segCount").get<int>();

    if (!j.at("freeSeg").is_null()) {
      if (!freeSeg)
        freeSeg = new Segment();
      freeSeg->from_json(j.at("freeSeg"));
    } else {
      delete freeSeg;
      freeSeg = nullptr;
    }
  }

private:
  int segCount;     // number of volume segments allocated
  Segment *freeSeg; // first unused segment
  MemPool *memPool; // memory pool for volume segments
};

#endif // SEGPOOL_H_
