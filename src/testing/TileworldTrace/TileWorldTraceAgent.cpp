#include "TileWorldTraceAgent.h"
#include <spdlog/spdlog.h>
#include <unistd.h>
#include <fstream>

TileWorldTraceAgent::TileWorldTraceAgent(const unsigned long startTime, const unsigned long endTime,
                                         unsigned long agentId)
    : Agent(startTime, endTime, agentId) {
  string dir = "trace/";
  string filename = to_string(agent_id());
  string suffix = ".txt";
  ifstream trace_s(dir + filename + suffix);
  string line;
  while (getline(trace_s, line)) {
    istringstream iss(line);
    int type;
    unsigned long ts;
    string rem;

    iss >> type;
    iss.ignore(1);
    iss >> ts;
    iss.ignore(1);
    iss >> rem;

    tuple<unsigned long, int, string> trace_item = make_tuple(ts, type, rem);
    trace_list_.emplace_back(trace_item);
  }
  spdlog::info("Trace loaded, agent {}, {} in total", agent_id(), trace_list_.size());

}


void TileWorldTraceAgent::Cycle() {
  unsigned long curr_ts = this->GetLVT();
  auto message_tuple = find_trace(curr_ts);
  if (get<1>(message_tuple) == -1) {
    // reached the end
    if (this->GetLVT() >= 100) {
      return;
    }
    this->time_wrap(100 - this->GetLVT());
    return;
  }
  unsigned long ts = get<0>(message_tuple);
  string m_body = get<2>(message_tuple);
  istringstream m_body_ss(m_body);
  switch (get<1>(message_tuple)) {
    case 10://rq
    {
      int ax, ay, bx, by;
      m_body_ss >> ax;
      m_body_ss.ignore(1);
      m_body_ss >> ay;
      m_body_ss.ignore(1);
      m_body_ss >> bx;
      m_body_ss.ignore(1);
      m_body_ss >> by;
      this->RangeQueryPoint(Point(ax, ay), Point(bx, by), ts);
    }
      break;
    case 14://read
    {
      unsigned long ssv_id;
      m_body_ss >> ssv_id;
      this->ReadPoint(ssv_id, ts);
    }
      break;
    case 18://write
    {
      unsigned long ssv_id;
      int px, py;
      m_body_ss >> ssv_id;
      m_body_ss.ignore(1);
      m_body_ss >> px;
      m_body_ss.ignore(1);
      m_body_ss >> py;
      this->WritePoint(ssv_id, Point(px, py), ts);
    }
      break;
    default:
      break;
  }
  if (this->GetGVT() < this->GetLVT() - 10 && this->agent_id() % 100 == 1) {
    SendGVTMessage();
  }
}

tuple<unsigned long, int, string> TileWorldTraceAgent::find_trace(unsigned long timestamp) {
  for (auto &it:trace_list_) {
    if (get<0>(it) >= timestamp) {
      return it;
    }
  }
  return make_tuple(0, -1, "");
}

