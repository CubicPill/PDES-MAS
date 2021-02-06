#ifndef PDES_MAS_TILEWORLDAGENT_H
#define PDES_MAS_TILEWORLDAGENT_H

#include "interface/Agent.h"

using namespace pdesmas;
using namespace std;

class TileWorldTraceAgent : public Agent {
public:

  TileWorldTraceAgent(unsigned long startTime, unsigned long endTime, unsigned long agent_id);

  void Cycle() override;



private:
  vector<tuple<unsigned long ,int,string>> trace_list_;
  tuple<unsigned long ,int,string> find_trace(unsigned long timestamp);
};


#endif //PDES_MAS_TILEWORLDAGENT_H
