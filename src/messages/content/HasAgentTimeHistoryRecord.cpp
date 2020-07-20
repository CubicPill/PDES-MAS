//
// Created by pill on 7/18/2020.
//

#include "HasAgentTimeHistoryRecord.h"
#include "spdlog/spdlog.h"
#include <sstream>
#include <climits>
#include <algorithm>

using namespace pdesmas;
using namespace std;

void HasAgentTimeHistoryRecord::AgentTimeHistoryRecord::Serialise(ostream &pOstream) const {
  SerialisableMap<unsigned long, string> m;
  for (const auto &i:agentTimeHistoryRecord) {
    list<unsigned long> history = i.second;

    ostringstream ss;
    const unsigned int size = history.size();
    ss << size;
    list<unsigned long>::const_iterator iter;
    for (iter = history.begin(); iter != history.end(); ++iter) {
      ss << '$' << *iter << '#';
    }
    m.insert(make_pair(i.first, ss.str()));
  }
  pOstream << m;
}

void HasAgentTimeHistoryRecord::AgentTimeHistoryRecord::Deserialise(istream &pIstream) {
  this->agentTimeHistoryRecord = map<unsigned long, list<unsigned long>>();
  SerialisableMap<unsigned long, string> m;
  //spdlog::warn("GCALC AgentTimeHistoryRecord dec");
  unsigned int mapSize;
  pIstream >> mapSize;

  for (unsigned int i = 0; i < mapSize; ++i) {
    pIstream.ignore(numeric_limits<streamsize>::max(), DELIM_LIST_LEFT);
    unsigned long agentId;
    pIstream >> agentId;
    pIstream.ignore(numeric_limits<streamsize>::max(), DELIM_LIST_LEFT);

    string content;
    char c = '\0';
    while (c != DELIM_LIST_RIGHT) {
      pIstream.get(c);
      content.push_back(c);
    }
    m.insert(make_pair(agentId, content));

  }

  //spdlog::warn("GCALC m size {}", m.size());
  for (const auto &i:m) {
    list<unsigned long> history;

    stringstream ss;
    ss << i.second;
    unsigned int size;
    ss >> size;
    for (unsigned int counter = 0; counter < size; ++counter) {
      ss.ignore(numeric_limits<streamsize>::max(), '$');
      unsigned long theValue;
      ss >> theValue;
      history.push_back(theValue);
      ss.ignore(numeric_limits<streamsize>::max(), '#');
    }

    this->agentTimeHistoryRecord.insert(make_pair(i.first, history));
  }
}

map<unsigned long, list<unsigned long>>
HasAgentTimeHistoryRecord::AgentTimeHistoryRecord::GetAgentTimeHistoryRecord() const {
  return this->agentTimeHistoryRecord;
}

void HasAgentTimeHistoryRecord::AgentTimeHistoryRecord::SetAgentTimeHistoryRecord(
    map<unsigned long, list<unsigned long>> agentTimeHistoryRecord) {
  this->agentTimeHistoryRecord = agentTimeHistoryRecord;
}

bool HasAgentTimeHistoryRecord::AgentTimeHistoryRecord::AddAgentTimeHistoryRecord(unsigned long agentId,
                                                                                  const list<unsigned long> &record) {
  if (this->agentTimeHistoryRecord.find(agentId) != this->agentTimeHistoryRecord.end()) {
    return false;
  }
  this->agentTimeHistoryRecord.insert(make_pair(agentId, record));
  return true;
}


map<unsigned long, list<unsigned long>> HasAgentTimeHistoryRecord::GetAgentTimeHistoryRecord() const {
  return this->agentTimeHistoryRecord.GetAgentTimeHistoryRecord();
}

void HasAgentTimeHistoryRecord::SetAgentTimeHistoryRecord(
    map<unsigned long, list<unsigned long>> agentTimeHistoryRecord) {
  this->agentTimeHistoryRecord.SetAgentTimeHistoryRecord(agentTimeHistoryRecord);
}

bool HasAgentTimeHistoryRecord::AddAgentTimeHistoryRecord(unsigned long agentId, const list<unsigned long> &record) {
  return this->agentTimeHistoryRecord.AddAgentTimeHistoryRecord(agentId, record);
}

unsigned long HasAgentTimeHistoryRecord::GetMinimumCycleBegin(unsigned long time) const {
  //spdlog::warn("GCALC Entering HasAgentTimeHistoryRecord::GetMinimumCycleBegin({})", time);
  //spdlog::warn("GCALC Size of history map: {}", this->agentTimeHistoryRecord.GetAgentTimeHistoryRecord().size());
  if (this->agentTimeHistoryRecord.GetAgentTimeHistoryRecord().size() == 0) {
    return ULONG_MAX;
  }
  unsigned long minimum = ULONG_MAX;
  for (const auto &i:this->agentTimeHistoryRecord.GetAgentTimeHistoryRecord()) {
    auto pos = upper_bound(i.second.begin(), i.second.end(), time);
    //spdlog::warn("GCALC Search {}, found {}", time, *pos);
    if (pos == i.second.begin()) {
      if (*pos == 0) {
        return 0;
      }
      continue;
    }
    unsigned long agentMinimum = *(--pos);
    // spdlog::warn("GCALC Agent minimum {}", agentMinimum);
    if (agentMinimum < minimum) {
      minimum = agentMinimum;
    }
  }

  return minimum;
}

