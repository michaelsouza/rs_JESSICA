#ifndef LINK_H_
#define LINK_H_

#include "Elements/node.h"

#include <iostream>
#include <nlohmann/json.hpp> // Include the JSON library
#include <string>

class Network;
class MemPool;

//! \class Link
//! \brief A conveyance element that connects two nodes together.
class Link : public Element {
public:
  enum LinkType { PIPE, PUMP, VALVE };
  enum LinkStatus { LINK_CLOSED, LINK_OPEN, VALVE_ACTIVE, TEMP_CLOSED };
  enum LinkReaction { BULK, WALL };

  Link(std::string name_);
  virtual ~Link();

  static Link *factory(int type_, std::string name_, MemPool *memPool);

  virtual int type() = 0;
  virtual std::string typeStr() = 0;
  virtual void convertUnits(Network *nw) = 0;
  virtual double convertSetting(Network *nw, double s) { return s; }
  virtual void validate(Network *nw) {}
  virtual bool isReactive() { return false; }

  // Initializes hydraulic settings
  virtual void initialize(bool initFlow);
  virtual void setInitFlow() {}
  virtual void setInitStatus(int s) {}
  virtual void setInitSetting(double s) {}
  virtual void setResistance(Network *nw) {}

  // Retrieves hydraulic variables
  virtual double getVelocity() { return 0.0; }
  virtual double getRe(const double q, const double viscos) { return 0.0; }
  virtual double getResistance() { return 0.0; }
  virtual double getUnitHeadLoss();
  virtual double getSetting(Network *nw) { return setting; }

  // Computes head loss, energy usage, and leakage
  virtual void findHeadLoss(Network *nw, double q) = 0;
  virtual double updateEnergyUsage(Network *nw, int dt) { return 0.0; }
  virtual bool canLeak() { return false; }
  virtual double findLeakage(Network *nw, double h, double &dqdh) {
    return 0.0;
  }

  // Determines special types of links
  virtual bool isPRV() { return false; }
  virtual bool isPSV() { return false; }
  virtual bool isHpPump() { return false; }

  // Used to update and adjust link status/setting
  virtual void updateStatus(double q, double h1, double h2) {}
  virtual bool changeStatus(int newStatus, bool makeChange,
                            const std::string reason, std::ostream &msgLog) {
    return false;
  }
  virtual bool changeSetting(double newSetting, bool makeChange,
                             const std::string reason, std::ostream &msgLog) {
    return false;
  }
  virtual void validateStatus(Network *nw, double qTol) {}
  virtual void applyControlPattern(std::ostream &msgLog) {}
  std::string writeStatusChange(int oldStatus);

  // Used for water quality routing
  virtual double getVolume() { return 0.0; }

  // Properties
  bool rptFlag;       //!< true if results are reported
  Node *fromNode;     //!< pointer to the link's start node
  Node *toNode;       //!< pointer to the link's end node
  int initStatus;     //!< initial Open/Closed status
  double diameter;    //!< link diameter (ft)
  double lossCoeff;   //!< minor head loss coefficient
  double initSetting; //!< initial pump speed or valve setting

  // Computed Variables
  int status;     //!< current status
  double flow;    //!< flow rate (cfs)
  double leakage; //!< leakage rate (cfs)
  double hLoss;   //!< head loss (ft)
  double hGrad;   //!< head loss gradient (ft/cfs)
  double setting; //!< current setting
  double quality; //!< avg. quality concen. (mass/ft3)

  //! Serialize to JSON
  virtual nlohmann::json to_json() const {
    return {{"rptFlag", rptFlag},
            {"fromNode", fromNode ? fromNode->to_json() : nullptr},
            {"toNode", toNode ? toNode->to_json() : nullptr},
            {"initStatus", initStatus},
            {"diameter", diameter},
            {"lossCoeff", lossCoeff},
            {"initSetting", initSetting},
            {"status", status},
            {"flow", flow},
            {"leakage", leakage},
            {"hLoss", hLoss},
            {"hGrad", hGrad},
            {"setting", setting},
            {"quality", quality}};
  }

  //! Deserialize from JSON
  virtual void from_json(const nlohmann::json &j) {
    rptFlag = j.at("rptFlag").get<bool>();
    initStatus = j.at("initStatus").get<int>();
    diameter = j.at("diameter").get<double>();
    lossCoeff = j.at("lossCoeff").get<double>();
    initSetting = j.at("initSetting").get<double>();
    status = j.at("status").get<int>();
    flow = j.at("flow").get<double>();
    leakage = j.at("leakage").get<double>();
    hLoss = j.at("hLoss").get<double>();
    hGrad = j.at("hGrad").get<double>();
    setting = j.at("setting").get<double>();
    quality = j.at("quality").get<double>();

    if (!j.at("fromNode").is_null()) {
      fromNode->from_json(j.at("fromNode"));
    }

    if (!j.at("toNode").is_null()) {
      toNode->from_json(j.at("toNode"));
    }
  }
};

#endif
