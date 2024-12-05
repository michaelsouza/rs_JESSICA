// src/CLI/BBStats.cpp
#include "BBStats.h"

#include <iomanip>  // For std::setw
#include <iostream> // For std::cout
#include <limits>
#include <mpi.h>
#include <sstream> // For std::ostringstream

BBStats::BBStats(int h_max)
{
  feasible_counter = std::vector<int>(h_max + 1, 0);
  split_counter = 0;
  prunings = std::vector<std::map<PruneReason, int>>(h_max + 1);
  // Initialize prunings
  std::vector<PruneReason> reasons = {PruneReason::ACTUATIONS, PruneReason::COST, PruneReason::PRESSURES, PruneReason::LEVELS,
                                      PruneReason::STABILITY};
  for (int h = 0; h <= h_max; ++h)
  {
    for (auto &reason : reasons)
    {
      prunings[h][reason] = 0;
    }
  }
}

void BBStats::add_pruning(PruneReason reason, int h)
{
  prunings[h][reason]++;
  // Increment split counter if the reason is SPLIT
  if (reason == PruneReason::SPLIT) split_counter++;
}

void BBStats::add_feasible(int h)
{
  feasible_counter[h]++;
}

void BBStats::to_json(const BBConfig &config, const BBConstraints &cnstr, double eta_secs, std::vector<int> &y_best, std::vector<int> &x_best)
{
  // Construct filename with date, time, and rank
  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  char filename[256];
  sprintf(filename, "BBStats_size_%d_rank_%d_acts_%d_hmax_%d_hthr_%d.json", size, rank, config.max_actuations, config.h_max, config.h_threshold);

  // Create a JSON object
  // Write JSON to file
  std::ofstream ofs(filename);
  if (!ofs.is_open())
  {
    Console::printf(Console::Color::RED, "Rank[%d]: Failed to open file %s for writing JSON stats.\n", rank, filename);
    return;
  }

  // Add solver info
  ofs << "{\n";
  ofs << "\t\"h_max\": " << config.h_max << ",\n";
  ofs << "\t\"max_actuations\": " << config.max_actuations << ",\n";
  ofs << "\t\"h_threshold\": " << config.h_threshold << ",\n";

  // Add eta_secs
  ofs << "\t\"eta_secs\": " << eta_secs << ",\n";

  // Add basic statistics
  ofs << "\t\"cost_best\": " << cnstr.cost_ub << ",\n";

  // Add y_best vector
  write_vector(ofs, y_best, "\t\"y_best\"");
  ofs << ",\n";

  // Add x_best vector
  write_vector(ofs, x_best, "\t\"x_best\"");
  ofs << ",\n";

  // Add feasible_counter
  write_vector(ofs, feasible_counter, "\t\"feasible_counter\"");
  ofs << ",\n";

  // Add split_counter
  ofs << "\t\"split_counter\": " << split_counter << ",\n";

  // Add prunings
  ofs << "\t\"prunings\": {\n";
  // Convert PruneReason enum to string for JSON
  for (size_t h = 0; h < prunings.size(); h++)
  {
    ofs << "\t\t\"h_" << h << "\": {\n";
    auto reasons = prunings[h];
    int reasons_size = reasons.size();
    int i = 0;
    for (const auto reason : reasons)
    {
      std::string reason_name = to_string(reason.first);
      ofs << "\t\t\t\"" << reason_name << "\": " << reason.second;
      if (i != reasons_size - 1)
      {
        ofs << ", ";
      }
      ofs << "\n";
      i++;
    }
    ofs << "\t\t}";
    if (h != prunings.size() - 1)
    {
      ofs << ",\n";
    }
  }
  ofs << "\n\t}\n}\n";
  ofs.flush();

  ofs.close();

  // Optional: Print confirmation
  Console::printf(Console::Color::GREEN, "Rank[%d]: Statistics written to %s\n", rank, filename);
}
