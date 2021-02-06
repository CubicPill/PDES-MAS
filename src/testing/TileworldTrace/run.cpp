#include "Simulation.h"
#include "TileWorldTraceAgent.h"
#include <iostream>
#include <random>
#include "spdlog/spdlog.h"
#include <fstream>
#include <sstream>
#include <string>

#define AGENT_ID(rank, i) (1000000 + (rank) * 10000 + 1 + (i))

using namespace std;
using namespace pdesmas;
int endTime = 100;

int main(int argc, char **argv) {
  spdlog::set_level(spdlog::level::info);
  spdlog::set_pattern("%f [%P] %+");
  Simulation sim = Simulation();

  endTime = 100;
  int numMPI;
  sim.InitMPI(&numMPI);

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

  // hardcoded
  unsigned long agent_list[1024];
  for (int i = 1; i <= 2; ++i) {
    for (int j = 1; j <= 512; ++j) {
      agent_list[(i - 1) * 512 + j] = 1000000 + i * 10000 + j;
    }
  }

  // preload from file

  ifstream ssv_file("trace/ssv.txt");
  // format: <clp>,<ssvid>,<px>,<py>
  string line;
  while (getline(ssv_file, line)) {
    istringstream iss(line);
    int clp_id, px, py;
    unsigned long ssv_id;
    iss >> clp_id;
    iss.ignore(1);
    iss >> ssv_id;
    iss.ignore(1);
    iss >> px;
    iss.ignore(1);
    iss >> py;
    sim.preload_variable(ssv_id, Point(px, py), clp_id);
    spdlog::info("Load SSV: {}, ({},{}), to clp {}", ssv_id, px, py, clp_id);
  }

  sim.Initialise();

  spdlog::info("Initialized, rank {0}, is {1}", sim.rank(), sim.type());

  // attach agent to ALP
  if (sim.type() == "ALP") {
    int index = sim.rank() - numCLP;
    int slice_len = 1024 / numALP;
    for (int i = slice_len * index; i < slice_len * (index + 1); ++i) {
      TileWorldTraceAgent *a = new TileWorldTraceAgent(0, 100, agent_list[i]);
      sim.add_agent(a);
    }
  }

  sim.Run();
  spdlog::info("LP exit, rank {0}", sim.rank());
  sim.Finalise();
}
