/*
 * Initialisor.cpp
 *
 *  Created on: 18 Aug 2011
 *      Author: Dr B.G.W. Craenen <b.g.w.craenen@cs.bham.ac.uk>
 */
#include <fstream>
#include <stdlib.h>
#include <string.h>

#include "SingleReadMessage.h"
#include "SingleReadResponseMessage.h"
#include "SingleReadAntiMessage.h"
#include "RangeQueryMessage.h"
#include "RangeQueryAntiMessage.h"
#include "WriteMessage.h"
#include "WriteResponseMessage.h"
#include "WriteAntiMessage.h"
#include "GvtControlMessage.h"
#include "GvtRequestMessage.h"
#include "GvtValueMessage.h"
#include "RollbackMessage.h"
#include "StateMigrationMessage.h"
#include "RangeUpdateMessage.h"
#include "EndMessage.h"
#include "Initialisor.h"
#include "Log.h"
#include "Clp.h"
#include <spdlog/spdlog.h>

#define MAX(a, b) (a>b?a:b)
#define MIN(a, b) (a<b?a:b)
using namespace std;
using namespace pdesmas;

Initialisor::Initialisor(int number_of_clp, int number_of_alp, unsigned long start_time, unsigned long end_time)
    : fHasInitInt(false), fHasInitLong(false), fHasInitDouble(false), fHasInitPoint(false), fHasInitString(false),
      number_of_clp_(number_of_clp), number_of_alp_(number_of_alp), start_time_(start_time), end_time_(end_time) {

}


Initialisor::~Initialisor() {
  fClpIdRangeMap.clear();
  fClpIdSsvIdMap.clear();
  fAlpToClpMap.clear();
  fClpSsvIdValueMap.clear();
}

void Initialisor::attach_alp_to_clp(int alp, int clp) {
  fAlpToClpMap[alp] = clp;
}

void Initialisor::preload_variable(const string &type, unsigned long variable_id, const string &v, unsigned int clpId) {
  AbstractValue *value;
  if (type.compare("INT") == 0) {
    value = valueClassMap->CreateObject(VALUEINT);
    value->SetValue(v);
  } else if (type.compare("LONG") == 0) {
    value = valueClassMap->CreateObject(VALUELONG);
    value->SetValue(v);
  } else if (type.compare("DOUBLE") == 0) {
    value = valueClassMap->CreateObject(VALUEDOUBLE);
    value->SetValue(v);
  } else if (type.compare("POINT") == 0) {
    value = valueClassMap->CreateObject(VALUEPOINT);
    value->SetValue(v);
  } else if (type.compare("STRING") == 0) {
    value = valueClassMap->CreateObject(VALUESTRING);
    value->SetValue(v);
  } else {
    LOG(logERROR) << "Initialisor::ParseSSV# Unrecognised SSV type: " << type;
    return;
  }
  auto ssvID = SsvId(variable_id);
  fClpSsvIdValueMap.insert(make_pair(ssvID, value));
  if (fClpIdSsvIdMap.find(clpId) == fClpIdSsvIdMap.end()) {
    fClpIdSsvIdMap.insert(make_pair(clpId, list<SsvId>()));
  }
  fClpIdSsvIdMap[clpId].push_back(ssvID);

  if (type.compare("POINT") == 0) {
    // update range
    Point pv = ((Value<Point> *) value)->GetValue();

    if (fClpIdRangeMap.find(clpId) == fClpIdRangeMap.end()) {
      fClpIdRangeMap[clpId] = Range(pv, pv);
    } else {
      Point max = fClpIdRangeMap[clpId].GetMaxRangeValue();
      Point min = fClpIdRangeMap[clpId].GetMinRangeValue();
      // Point newMax, newMin;

      //see if the point is the new min/max
      if (max.GetX() < pv.GetX() || max.GetY() < pv.GetY()) {
        fClpIdRangeMap[clpId].SetMaxRangeValue(Point(MAX(max.GetX(), pv.GetX()), MAX(max.GetY(), pv.GetY())));
      } else if (min.GetX() > pv.GetX() || max.GetY() > pv.GetY()) {
        fClpIdRangeMap[clpId].SetMinRangeValue(Point(MIN(min.GetX(), pv.GetX()), MIN(min.GetY(), pv.GetY())));
      }
    }
  }
}


const map<unsigned int, Range> &Initialisor::GetClpToRangeMap() const {
  return fClpIdRangeMap;
}

const map<unsigned int, list<SsvId> > &Initialisor::GetClpToSsvMap() const {
  return fClpIdSsvIdMap;
}

const map<unsigned int, unsigned int> &Initialisor::GetAlpToClpMap() const {
  return fAlpToClpMap;
}

const map<SsvId, AbstractValue *> &Initialisor::GetClpSsvIdValueMap() const {
  return fClpSsvIdValueMap;
}


void Initialisor::InitEverything() {

  Value<int>();
  fHasInitInt = true;

  Value<double>();
  fHasInitDouble = true;

  Value<Point>();
  fHasInitPoint = true;

  Value<string>();
  fHasInitString = true;

  Value<long>();
  fHasInitLong = true;


  SingleReadMessage();
  SingleReadResponseMessage();
  SingleReadAntiMessage();
  RangeQueryMessage();
  RangeQueryAntiMessage();
  WriteMessage();
  WriteResponseMessage();
  WriteAntiMessage();
  GvtControlMessage();
  GvtRequestMessage();
  GvtValueMessage();
  RollbackMessage();
  StateMigrationMessage();
  RangeUpdateMessage();
  EndMessage();
}

void Initialisor::Finalise() {
  for (int i = 0; i < number_of_clp_; ++i) {
    if (fClpIdRangeMap.find(i) == fClpIdRangeMap.end()) {
      fClpIdRangeMap[i] = Range(Point(INT_MAX, INT_MAX), Point(INT_MAX, INT_MAX));
    }
  }
}


