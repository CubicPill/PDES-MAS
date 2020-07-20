#include "Simulation.h"
#include "TileWorldAgent.h"
#include <iostream>
#include "spdlog/spdlog.h"

#define AGENT_ID(rank, i) (1000000 + (rank) * 10000 + 1 + (i))

using namespace std;
using namespace pdesmas;
int endTime = 1000;

int main(int argc, char **argv) {
  spdlog::set_level(spdlog::level::debug);
  spdlog::set_pattern("%f [%P] %+");
  Simulation sim = Simulation();

  uint64_t numAgents = std::atoll(argv[1]);
  uint64_t numMPI = std::atoll(argv[2]);
  int randSeed = std::atoi(argv[3]);
  endTime = std::atoi(argv[4]);

  const int worldSize = sqrt(numAgents) * 18;
  const uint64_t numTile = numAgents / 4;
  srand(randSeed * numMPI);

  // numMPI -> CLP and ALP
  uint64_t numALP = (numMPI + 1) / 2;
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

  // preload variables about agent locations
  for (uint64_t i = numCLP; i < numMPI; ++i) { //3,4,5,6
    for (uint64_t j = 0; j < numAgents / numALP; ++j) {  // max: 1024
      unsigned long agentId = AGENT_ID(i, j); // 1000000 + i * 10000 + 1 + j
      // e.g. 1000000 + 4*10000 + 1 + 1021 = 1041022 (1|04|1022)
      sim.preload_variable(agentId, Point(rand() % worldSize, rand() % worldSize), 0);
      if (sim.rank() == 0) {
        spdlog::info("sim.preload_variable({}, Point({}, {}), 0);", agentId, rand() % worldSize, rand() % worldSize);
      }
    }
  }
  // preload variables about tile locations
  for (uint64_t i = 0; i < numTile; ++i) {
    sim.preload_variable(3000000 + i, Point(rand() % worldSize, rand() % worldSize), 0);
    if (sim.rank() == 0) {
      spdlog::info("sim.preload_variable({}, Point({}, {}), 0)", 3000000 + i, rand() % worldSize, rand() % worldSize);
    }
  }

  sim.Initialise();

  spdlog::info("Initialized, rank {0}, is {1}", sim.rank(), sim.type());

  srand(randSeed * numMPI + sim.rank());
  if (sim.type() == "ALP") {
    for (uint64_t i = 0; i < numAgents / numALP; ++i) {
      TileWorldAgent *test = new TileWorldAgent(0, endTime, AGENT_ID(sim.rank(), i), worldSize, worldSize, 10,
                                                randSeed);
      sim.add_agent(test);
    }

  }

  sim.Run();
  spdlog::info("LP exit, rank {0}", sim.rank());

  sim.Finalise();
}