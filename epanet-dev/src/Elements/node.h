#ifndef NODE_H_
#define NODE_H_

#include "Elements/element.h"
#include "Elements/qualsource.h"
#include <nlohmann/json.hpp> // Include nlohmann/json header
#include <string>

class Network;
class Emitter;
class MemPool;

class Node : public Element {
public:
  enum NodeType { JUNCTION, TANK, RESERVOIR };

  Node(std::string name_);
  virtual ~Node();

  static Node *factory(int type_, std::string name_, MemPool *memPool);

  virtual int type() = 0;
  virtual void convertUnits(Network *nw) = 0;
  virtual void initialize(Network *nw);

  // Overridden for Junction nodes
  virtual void findFullDemand(double multiplier, double patternFactor) {}
  virtual double findActualDemand(Network *nw, double h, double &dqdh) {
    return 0;
  }
  virtual double findEmitterFlow(double h, double &dqdh) { return 0; }
  virtual void setFixedGrade() { fixedGrade = false; }
  virtual bool isPressureDeficient(Network *nw) { return false; }
  virtual bool hasEmitter() { return false; }

  // Overridden for Tank nodes
  virtual void validate(Network *nw) {}
  virtual bool isReactive() { return false; }
  virtual bool isFull() { return false; }
  virtual bool isEmpty() { return false; }
  virtual bool isClosed(double flow) { return false; }
  virtual double getVolume() { return 0.0; }

  // Input Parameters
  bool rptFlag;           //!< true if results are reported
  double elev;            //!< elevation (ft)
  double xCoord;          //!< X-coordinate
  double yCoord;          //!< Y-coordinate
  double initQual;        //!< initial water quality concen.
  QualSource *qualSource; //!< water quality source information

  // Computed Variables
  bool fixedGrade;     //!< fixed grade status
  double head;         //!< hydraulic head (ft)
  double qGrad;        //!< gradient of outflow w.r.t. head (cfs/ft)
  double fullDemand;   //!< full demand required (cfs)
  double actualDemand; //!< actual demand delivered (cfs)
  double outflow;      //!< demand + emitter + leakage flow (cfs)
  double quality;      //!< water quality concen. (mass/ft3)

  //! Serialize to JSON
  nlohmann::json to_json() const override {
    return {{"name", name},
            {"fixedGrade", fixedGrade},
            {"head", head},
            {"qGrad", qGrad},
            {"fullDemand", fullDemand},
            {"actualDemand", actualDemand},
            {"outflow", outflow}};
  }

  //! Deserialize from JSON
  void from_json(const nlohmann::json &j) {
    fixedGrade = j.at("fixedGrade").get<bool>();
    head = j.at("head").get<double>();
    qGrad = j.at("qGrad").get<double>();
    fullDemand = j.at("fullDemand").get<double>();
    actualDemand = j.at("actualDemand").get<double>();
    outflow = j.at("outflow").get<double>();
  }
};

#endif
