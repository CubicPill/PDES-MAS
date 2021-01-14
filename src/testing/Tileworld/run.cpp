#include "Simulation.h"
#include "TileWorldAgent.h"
#include <iostream>
#include <random>
#include "spdlog/spdlog.h"

#define AGENT_ID(rank, i) (1000000 + (rank) * 10000 + 1 + (i))

using namespace std;
using namespace pdesmas;
int endTime = 100;

int main(int argc, char **argv) {
  spdlog::set_level(spdlog::level::info);
  spdlog::set_pattern("%f [%P] %+");
  Simulation sim = Simulation();
  if (argc != 5) {
    fprintf(stderr, "Incorrect number of parameters\n");
    fprintf(stderr, "Parameters: tileworld <numAgent> <randSeed> <worldSize> <numTile>\n");
    exit(1);
  }

  uint64_t numAgents = std::atoll(argv[1]);

  int randSeed = std::atoi(argv[2]);
  endTime = 100;
  int numMPI;
  sim.InitMPI(&numMPI);
  int worldSize = std::atoi(argv[3]); //2048
  uint64_t numTile = std::atoi(argv[4]); // 1024
  srand(randSeed * numMPI);
  std::normal_distribution<float> distribution(25, 7);
  std::default_random_engine generator;
  // numMPI -> CLP and ALP
  uint64_t numALP = (numMPI + 1) / 2;
  if (numALP > 32) {
    fprintf(stderr, "This version currently only runs with maximum 32 ALPs\n");
    exit(1);
  }
  uint64_t numCLP = numALP - 1;
  spdlog::debug("CLP: {}, ALP: {}", numCLP, numALP);
  sim.Construct(numCLP, numALP, 0, endTime);
  spdlog::info("MPI process up, rank {0}, size {1}", sim.rank(), sim.size());

  // attach alp to clp
  for (uint64_t i = numCLP; i < numMPI; ++i) {
    sim.attach_alp_to_clp(i, (i - 1) / 2);
    if (sim.rank() == 0) {
      spdlog::info("sim.attach_alp_to_clp({}, {})", i, (i - 1) / 2);
    }
  }
  int interval = worldSize / 6;
  int peak_xy_candidate[4] = {interval, interval * 2, interval * 3, interval * 4};
  vector<tuple<int, int>> points; // len:1024
  for (int i = 0; i < 32; ++i) { // 32 partitions because of maximum 32 ALP
    int peak_x_idx, peak_y_idx;
    peak_x_idx = (i >> 1) & 0b0011;
    peak_y_idx = i >> 3;
    for (int j = 0; j < numAgents / 32; ++j) { // agentNum/32 points per partition

      int peak_x = peak_xy_candidate[peak_x_idx];
      int peak_y = peak_xy_candidate[peak_y_idx];
      std::normal_distribution<float> _distribution(25, 7);
      float x = _distribution(generator) + peak_x;
      if (x < 0) x = 0;
      if (x > worldSize) x = worldSize - 1;
      float y = _distribution(generator) + peak_y;
      if (y < 0) y = 0;
      if (y > worldSize) y = worldSize - 1;
      points.emplace_back(tuple<int, int>(x, y));
    }
  }



// preload variables about agent locations

  for (uint64_t i = numCLP; i < numMPI; ++i) { //3,4,5,6
    for (uint64_t j = 0; j < numAgents / numALP; ++j) {  // max: 1024
      unsigned long agentId = AGENT_ID(i, j); // 1000000 + i * 10000 + 1 + j

      // e.g. 1000000 + 4*10000 + 1 + 1021 = 1041022 (1|04|1022)
      tuple<int, int> coord = points.back();
      points.pop_back();
      sim.preload_variable(agentId, Point(get<0>(coord), get<1>(coord)), 0);

    }
  }
  // preload variables about tile locations
  for (uint64_t i = 0; i < numTile; ++i) {
    int peak_x = peak_xy_candidate[rand() % 4];
    int peak_y = peak_xy_candidate[rand() % 4];
    std::normal_distribution<float> _distribution(25, 7);
    float x = _distribution(generator) + peak_x;
    if (x < 0) x = 0;
    if (x > worldSize) x = worldSize - 1;
    float y = _distribution(generator) + peak_y;
    if (y < 0) y = 0;
    if (y > worldSize) y = worldSize - 1;
    sim.preload_variable(3000000 + i, Point((int) x, (int) y), 0);
    if (sim.rank() == 0) {
      spdlog::info("sim.preload_variable({}, Point({}, {}), 0)", 3000000 + i, rand() % worldSize, rand() % worldSize);
    }
  }
  // hole locations
  for (uint64_t i = 0; i < numTile; ++i) {
    int peak_x = peak_xy_candidate[rand() % 4];
    int peak_y = peak_xy_candidate[rand() % 4];
    std::normal_distribution<float> _distribution(25, 7);
    float x = _distribution(generator) + peak_x;
    if (x < 0) x = 0;
    if (x > worldSize) x = worldSize - 1;
    float y = _distribution(generator) + peak_y;
    if (y < 0) y = 0;
    if (y > worldSize) y = worldSize - 1;
    sim.preload_variable(2000000 + i, Point((int) x, (int) y), 0);
    if (sim.rank() == 0) {
      spdlog::info("sim.preload_variable({}, Point({}, {}), 0)", 2000000 + i, rand() % worldSize, rand() % worldSize);
    }
  }


  sim.Initialise();

  spdlog::info("Initialized, rank {0}, is {1}", sim.rank(), sim.type());

  srand(randSeed * numMPI + sim.rank());
  if (sim.type() == "ALP") {
    for (uint64_t i = 0; i < numAgents / numALP; ++i) {
      unsigned long agentId = AGENT_ID(sim.rank(), i);
      TileWorldAgent *test = new TileWorldAgent(0, endTime, agentId, worldSize, worldSize, 3,
                                                randSeed);
      sim.add_agent(test);
    }
  }

  sim.Run();
  spdlog::info("LP exit, rank {0}", sim.rank());

  sim.Finalise();
}
