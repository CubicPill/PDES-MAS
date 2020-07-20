//
// Created by pill on 7/18/2020.
//

#ifndef HASAGENTTIMEHISTORYRECORD_H_
#define HASAGENTTIMEHISTORYRECORD_H_

#include "Serialisable.h"
#include "SerialisableMap.h"

#include <list>
#include <map>

using std::list;
namespace pdesmas {


  class HasAgentTimeHistoryRecord {
  private:
    class AgentTimeHistoryRecord : public Serialisable {
    protected:
      map<unsigned long, list<unsigned long> > agentTimeHistoryRecord;

    public:

      void Serialise(ostream &) const override;

      void Deserialise(istream &) override;

      map<unsigned long, list<unsigned long> > GetAgentTimeHistoryRecord() const;

      void SetAgentTimeHistoryRecord(map<unsigned long, list<unsigned long> >);

      bool AddAgentTimeHistoryRecord(unsigned long agentId, const list<unsigned long> &record);
    };

  protected:
    AgentTimeHistoryRecord agentTimeHistoryRecord;
    //TODO find a new way to serialize data. This is a temp workaround
  public:
    map<unsigned long, list<unsigned long>> GetAgentTimeHistoryRecord() const;

    void
    SetAgentTimeHistoryRecord(map<unsigned long, list<unsigned long> > agentTimeHistoryRecord);

    bool AddAgentTimeHistoryRecord(unsigned long agentId, const list<unsigned long> &record);

    unsigned long GetMinimumCycleBegin(unsigned long time) const;
  };

}

#endif
