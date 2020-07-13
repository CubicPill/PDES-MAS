//
// Created by pill on 19-6-26.
//

#include "TileWorldAgent.h"
#include <spdlog/spdlog.h>
#include <unistd.h>
#include <random>

TileWorldAgent::TileWorldAgent(const unsigned long startTime, const unsigned long endTime, unsigned long agentId,
                               unsigned int world_size_w, unsigned int world_size_h, unsigned int sense_range) : Agent(
    startTime, endTime, agentId), kSenseRange(sense_range), world_size_w(world_size_w), world_size_h(world_size_h) {
  this->WritePrivateInt(IS_TILE_CARRYING, 0);
  this->WritePrivateInt(IS_EN_ROUTE, 0);

}

static inline int GetFirstDigit(int number) {
  while (number >= 10) {
    number = (number - (number % 10)) / 10;
  }
  return number;
}

void TileWorldAgent::Cycle() {
  //spdlog::warn("Cycle begin");
  spdlog::info("Agent {}, GVT {} LVT {} - {}", this->agent_id(), this->GetGVT(), this->GetLVT(),
               (int) this->GetLVT() - (int) this->GetGVT());
  // if close to endtime, proceed only one timestamp per cycle and sync GVT to avoid LVT exceeding endtime.
  if (this->GetLVT() > GetEndTime() - 10) {
    this->SendGVTMessage();
    //spdlog::info("Agent {}, GVT {} endtime {}", this->agent_id(), this->GetGVT(), GetEndTime());

    Point my_position = this->ReadPoint(this->agent_id(), this->GetLVT() + 1);
    int curr_x = my_position.GetX();
    int curr_y = my_position.GetY();
    //spdlog::info("Agent {0}, ({1},{2}), {3}", this->agent_id(), curr_x, curr_y, this->GetLVT());
    return;
  }
  // where am i?
  //spdlog::debug("Agent {0}, read {0}",this->agent_id());
  //Point my_position=Point(0,0);
  Point my_position = this->ReadPoint(this->agent_id(), this->GetLVT() + 1);
  int curr_x = my_position.GetX();
  int curr_y = my_position.GetY();
  //spdlog::info("Agent {0}, ({1},{2}), {3}", this->agent_id(), curr_x, curr_y, this->GetLVT());
  //sense
  SerialisableMap<SsvId, Value<Point> > results = this->RangeQueryPoint(
      Point(curr_x - kSenseRange, curr_y - kSenseRange),
      Point(curr_x + kSenseRange, curr_y + kSenseRange),
      this->GetLVT() + 1);
//  SerialisableMap<SsvId, Value<Point> > results = this->RangeQueryPoint(
//      Point(-100,-100),
//      Point(100,100),
//      this->GetLVT());
  //spdlog::info("Agent {0}, RQ result size {1}, {3}", this->agent_id(), results.size(), kSenseRange, this->GetLVT());
  //spdlog::debug("Agent {0}, Agent LVT {1}, preparing to read id {2}", this->agent_id(), this->GetLVT(), 1);
  map<SsvId, Point> hole_in_range = map<SsvId, Point>();
  map<SsvId, Point> tile_in_range = map<SsvId, Point>();
  map<SsvId, Point> obstacle_in_range = map<SsvId, Point>();
  for (auto &i :results) {
    SsvId ssv_id = i.first;
    Point p = i.second.GetValue();
    switch (GetObjectTypeFromSsvId(ssv_id)) {
      case AGENT:
        // no use for now
        break;
      case HOLE:
        // fill if carrying tiles
        // record for further use
        hole_in_range.insert(make_pair(ssv_id, p));
        break;
      case TILE:
        // pick if no tiles carrying
        // record for further use
        tile_in_range.insert(make_pair(ssv_id, p));

        break;
      case OBSTACLE:
        // temporarily of no use, just to check reachability
        obstacle_in_range.insert(make_pair(ssv_id, p));

        break;
      default:
        break;
    }
  }
  // TODO: find closest object maybe?
  for (auto &i:obstacle_in_range) {
    SsvId ssv_id = i.first;
    Point p = i.second;
    //we should ues A* here, but seems it makes no difference, just pass
  }

  // by default, random move the agent
  int rand_x = rand() % 5 - 2;  // [-2, 2]
  int rand_y = rand() % 5 - 2;  // [-2, 2]
  Point rand_p = Point(abs(curr_x + rand_x) % world_size_w, abs(curr_y + rand_y) % world_size_h);

  if (tile_in_range.empty()) {
    // no tile, make a random move

    WritePoint(this->agent_id(), rand_p, this->GetLVT() + 1); // moving to the position
    //spdlog::info("Agent {0}, to ({1},{2}), {3}", this->agent_id(), rand_p.GetX(), rand_p.GetY(), this->GetLVT());
  } else {

    SsvId my_tile_ssv_id;
    for (auto &i:tile_in_range) {
      SsvId ssv_id = i.first;
      Point p = i.second;
      if (ReadPrivateInt(IS_TILE_CARRYING) == 0) {
        //pick up tile
        WritePrivateInt(IS_TILE_CARRYING, 1);

        WritePoint(this->agent_id(), p, this->GetLVT() + 1); // moving to the position
        //spdlog::info("Agent {0}, to tile ({1},{2}), {3}", this->agent_id(), p.GetX(), p.GetY(), this->GetLVT());
        // FIXME: count the distance with A*

        my_tile_ssv_id = ssv_id;
        break;
      }

    }

    if (hole_in_range.empty()) {
      // random move with tile
      WritePoint(this->agent_id(), rand_p, this->GetLVT() + 1); // moving to the position
      //spdlog::info("Agent {0}, to ({1},{2}) with tile, {3}", this->agent_id(), rand_p.GetX(), rand_p.GetY(),
      //this->GetLVT());
      WritePoint(my_tile_ssv_id.id(), rand_p, this->GetLVT() + 1);
      //spdlog::info("Agent {0}, dropped tile at ({1},{2}), {3}", this->agent_id(), rand_p.GetX(), rand_p.GetY(),
      //this->GetLVT());

    } else {

      for (auto &i:hole_in_range) {
        SsvId ssv_id = i.first;
        Point p = i.second;
        //    if (ReadPrivateInt(IS_TILE_CARRYING) == 1) {
        // have tile, move to hole
        WritePrivateInt(IS_TILE_CARRYING, 0);

        WritePoint(this->agent_id(), p, this->GetLVT() + 1);
        //spdlog::info("Agent {0}, to hole ({1},{2}), {3}", this->agent_id(), p.GetX(), p.GetY(), this->GetLVT());

        WritePoint(my_tile_ssv_id.id(), p, this->GetLVT() + 1);
//        spdlog::info("Agent {0}, dropped tile at hole ({1},{2}), {3}", this->agent_id(), p.GetX(), p.GetY(),
//                     this->GetLVT());

        break;
        //    }
      }
    }
  }

//  if (agent_id() == 1010001) {
  // GVT initiator
  if (this->GetLVT() > this->GetGVT() + 20) {
    this->SendGVTMessage();
    //spdlog::warn("agent {}, GVT {}", this->agent_id(), this->GetGVT());


  }
//  }
  //this->Sleep(100);

  //spdlog::warn("Cycle end");

}

inline TileWorldAgent::Object TileWorldAgent::GetObjectTypeFromSsvId(SsvId &ssv_id) {
  if (GetFirstDigit(ssv_id.id()) == 1) {
    return AGENT;
  } else if (GetFirstDigit(ssv_id.id()) == 2) {
    return HOLE;
  } else if (GetFirstDigit(ssv_id.id()) == 3) {
    return TILE;
  } else if (GetFirstDigit(ssv_id.id()) == 4) {
    return OBSTACLE;
  }
  return NUL;

}