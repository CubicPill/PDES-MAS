//
// Created by pill on 19-6-26.
//

#include "TileWorldAgent.h"
#include <spdlog/spdlog.h>
#include <unistd.h>

TileWorldAgent::TileWorldAgent(const unsigned long startTime, const unsigned long endTime, unsigned long agentId,
                               unsigned int world_size_w, unsigned int world_size_h, unsigned int sense_range,
                               unsigned int seed) : Agent(
    startTime, endTime, agentId), kSenseRange(sense_range), world_size_w(world_size_w), world_size_h(world_size_h),
                                                    rand_seed(seed) {
//  this->AddPrivateVariable(IS_TILE_CARRYING);
//  this->AddPrivateVariable(IS_EN_ROUTE);
//  this->WritePrivateInt(IS_TILE_CARRYING, 0);
//  this->WritePrivateInt(IS_EN_ROUTE, 0);

}


static inline int GetFirstDigit(int number) {
  while (number >= 10) {
    number = (number - (number % 10)) / 10;
  }
  return number;
}

void TileWorldAgent::Cycle() {
  //spdlog::warn("Cycle begin");

  spdlog::info("Agent {}, GVT {} LVT {}, {} advance", this->agent_id(), this->GetGVT(), this->GetLVT(),
               (int) this->GetLVT() - (int) this->GetGVT());
  // where am i?
  //spdlog::debug("Agent {0}, read {0}",this->agent_id());

  Point my_position = this->ReadPoint(this->agent_id(), this->GetLVT());
  int curr_x = my_position.GetX();
  int curr_y = my_position.GetY();
  //spdlog::info("Agent {0}, ({1},{2}), LVT {3}", this->agent_id(), curr_x, curr_y, this->GetLVT());
  //sense
  SerialisableMap<SsvId, Value<Point> > results = this->RangeQueryPoint(
      Point(curr_x - kSenseRange, curr_y - kSenseRange),
      Point(curr_x + kSenseRange, curr_y + kSenseRange),
      this->GetLVT());
//  SerialisableMap<SsvId, Value<Point> > results = this->RangeQueryPoint(
//      Point(-100,-100),
//      Point(100,100),
//      this->GetLVT());
  //spdlog::info("Agent {0}, RQ result size {1}, LVT {3}", this->agent_id(), results.size(), kSenseRange, this->GetLVT());
  //spdlog::debug("Agent {0}, Agent LVT {1}, preparing to read id {2}", this->agent_id(), this->GetLVT(), 1);
  // by default, random move the agent
//  int rand_x = (rand() * this->agent_id() * this->GetLVT()) % 3 - 1;  // [-1, 1]
//  int rand_y = (rand() * this->agent_id() * this->GetLVT()) % 3 - 1;  // [-1, 1]
//
//
//  Point rand_p = Point(abs(curr_x + rand_x) % world_size_w, abs(curr_y + rand_y) % world_size_h);
//
//    if (rand_x != 0 && rand_y != 0) {
//      WritePoint(this->agent_id(), rand_p, this->GetLVT()); // moving to the position
//    }
//  if (this->agent_id() % 10 < 2 && this->GetLVT() > this->GetGVT() + 5) {
//    if (gvtCount >= 5) {
//      this->SendGVTMessage();
//    }
//    gvtCount %= 5;
//    gvtCount++;
//    //spdlog::warn("GVT {} send by {}", this->GetGVT(), agent_id());
//  }
//  if (this->GetLVT() - this->GetGVT() > 10) {
//    this_thread::sleep_for(chrono::milliseconds(1000));
//    // too much! throttle to yield some resources to others
//  }
//  return;


  if (results.empty()) {
    // nothing
//    if (rand() % 10 == 0) {
//      this->time_wrap(1);
//    }
  } else {
    map<SsvId, Point> agent_in_range = map<SsvId, Point>();
    map<SsvId, Point> hole_in_range = map<SsvId, Point>();
    map<SsvId, Point> tile_in_range = map<SsvId, Point>();
    map<SsvId, Point> obstacle_in_range = map<SsvId, Point>();
    for (auto &i :results) {
      SsvId ssv_id = i.first;
      Point p = i.second.GetValue();
      switch (GetObjectTypeFromSsvId(ssv_id)) {
        case AGENT:
          agent_in_range.insert(make_pair(ssv_id, p));
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
    // by default, random move the agent
    int rand_x = (rand() * this->agent_id() * this->GetLVT()) % 3 - 1;  // [-1, 1]
    int rand_y = (rand() * this->agent_id() * this->GetLVT()) % 3 - 1;  // [-1, 1]

//    // but try to move away from others
//    int dx = 0, dy = 0;
//    for (auto &i:agent_in_range) {
//      dx += curr_x - i.second.GetX();
//      dy += curr_y - i.second.GetY();
//    }
//    if (dx != 0) {
//      rand_x = dx > 0 ? 1 : -1;
//    }
//    if (dy != 0) {
//      rand_y = dy > 0 ? 1 : -1;
//    }

    //this->time_wrap(1); // thinking


    //spdlog::info("Agent {}, rand_x = {}, rand_y = {}， LVT {}", agent_id(), rand_x, rand_y, this->GetLVT());
    Point rand_p = Point(abs(curr_x + rand_x) % world_size_w, abs(curr_y + rand_y) % world_size_h);

    if (tile_in_range.empty()) {
      // no tile, make a random move
      if (rand_x != 0 && rand_y != 0) {
        WritePoint(this->agent_id(), rand_p, this->GetLVT()); // moving to the position
        //spdlog::info("Agent {0}, to ({1},{2}), LVT {3}", this->agent_id(), rand_p.GetX(), rand_p.GetY(), this->GetLVT());
      }
    } else {

      SsvId my_tile_ssv_id;
      for (auto &i:tile_in_range) {
        SsvId ssv_id = i.first;
        Point p = i.second;
        if (this->tile_carry == false) {
          //pick up tile
          this->tile_carry = true;

          //WritePoint(this->agent_id(), p, this->GetLVT() + 1); // moving to the position

          WritePoint(ssv_id.id(), Point(-100, -100), this->GetLVT()); // pick up tile
          //WritePoint(ssv_id.id(), Point(rand()%world_size_h,rand()%world_size_w),this->GetLVT());// generate new tile
          //spdlog::info("Agent {0}, to tile ({1},{2}), LVT {3}", this->agent_id(), p.GetX(), p.GetY(), this->GetLVT());

          my_tile_ssv_id = ssv_id;
          break;
        }

      }

      if (hole_in_range.empty()) {
        // random move
        if (rand_x != 0 && rand_y != 0) {
          WritePoint(this->agent_id(), rand_p, this->GetLVT()); // moving to the position
        }        //spdlog::info("Agent {0}, to ({1},{2}) with tile, LVT {3}", this->agent_id(), rand_p.GetX(), rand_p.GetY(),
        //this->GetLVT());
//        if (my_tile_ssv_id.id() != 0 && rand_x != 0 && rand_y != 0) {
//          WritePoint(my_tile_ssv_id.id(), Point(-100, -100), this->GetLVT());
//        }
        //spdlog::info("Agent {0}, dropped tile at ({1},{2}), LVT {3}", this->agent_id(), rand_p.GetX(), rand_p.GetY(),
        //this->GetLVT());

      } else {

        for (auto &i:hole_in_range) {
          SsvId ssv_id = i.first;
          Point p = i.second;
          if (this->tile_carry) {
            // have tile, move to hole
            this->tile_carry = false;

            //WritePoint(this->agent_id(), p, this->GetLVT() + 1);
            //spdlog::info("Agent {0}, to hole ({1},{2}), LVT {3}", this->agent_id(), p.GetX(), p.GetY(), this->GetLVT());
            //WritePoint(ssv_id.id(), Point(rand()%world_size_h,rand()%world_size_w),this->GetLVT());// generate new hole
            WritePoint(ssv_id.id(), Point(-100,-100),this->GetLVT());

//          spdlog::info("Agent {0}, dropped tile at hole ({1},{2}), LVT {3}", this->agent_id(), p.GetX(), p.GetY(),
//                       this->GetLVT());

            break;
          }
        }
      }
    }
  }
//  if (agent_id() == 1010001) {
  // GVT initiator
  if (this->agent_id() % 10 < 2 && this->GetLVT() > this->GetGVT() + 5) {
    if (gvtCount >= 5) {
      this->SendGVTMessage();
    }
    gvtCount %= 5;
    gvtCount++;
    //spdlog::warn("GVT {} send by {}", this->GetGVT(), agent_id());
  }
//  if (this->GetLVT() - this->GetGVT() > 20) {
//    this_thread::sleep_for(chrono::milliseconds(100));
//    // too much! throttle to yield some resources to others
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
