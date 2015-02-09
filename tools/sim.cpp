// Author: Zhi-Ting Hu, Po-Yao Huang
// Date: 2014.10.26
#include "gflags/gflags.h"
#include "glog/logging.h"
#include "context.hpp"
#include "ee_engine.hpp"
#include "analyst.hpp"

#include <thread>
#include <vector>
#include <cstdint>
#include <iostream>

using namespace std;

// (temp) entity embedding parameters
DEFINE_int32(dim_embedding, 100, "");
DEFINE_string(distance_metric_mode, "DIAG", "");
DEFINE_int32(num_neg_sample, 50, "");

// Results
DEFINE_string(resume_path, "output/", "Results to be analyzed");
DEFINE_int32(resume_iter, 100, "Iteration of results");
// Data
DEFINE_string(dataset_path, "data/tech/", "data path");
DEFINE_string(output_file_prefix, "output/", "Results go here.");
DEFINE_string(category_filename, "categories.txt", "category filename");
DEFINE_string(entity_filename, "entity.txt", "entity filename");
DEFINE_string(entity_to_ancestor_filename, "entity2ancestor.bin", "entity-ancestor filename");
DEFINE_string(entity_to_category_filename, "entity2category.txt", "entity-category filename");
DEFINE_string(hierarchy_filename, "hierarchy.txt", "hierarchy filename");
DEFINE_string(hierarchy_id_filename, "hierarchy_id.txt", "hierarchy id filename");
DEFINE_string(pair_filename, "pair.txt", "pair id filename");
DEFINE_string(level_filename, "level.txt", "category level filename");

// WordSim data
DEFINE_string(wordsim_filename, "wordsim.txt", "word sim data");

int main(int argc, char *argv[]) {
  FLAGS_alsologtostderr = 1;
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  entity::Context::set_phase(entity::Context::ANALYZE);
  entity::Analyst analyst(FLAGS_resume_path, FLAGS_resume_iter);

  //vector<int> candidate_entities = {9336, 397504, 719040, 1480281, 131};

  FILE * fp = fopen(FLAGS_wordsim_filename.c_str(), "r");
  FILE * fp_out = fopen("output.txt", "w");
  int id1, id2;
  float score;
  std::vector<std::pair<int, int> > entity_pairs;
  std::vector<float> pair_scores, pair_scores_gnd;
  while (fscanf(fp, "%d%d%f", &id1, &id2, &score) == 3) {
      CHECK(id1 >=0 && id2 >= 0);
      entity_pairs.push_back(std::make_pair(id1, id2));
      pair_scores_gnd.push_back(score);
      
  }

  vector<int> occur_cnt;
  vector<float> weight;
  analyst.ComputeDistance(entity_pairs, pair_scores, occur_cnt, weight);

  for (int i = 0; i < entity_pairs.size(); ++i) {
      fprintf(fp_out, "%f\t%f\t%d\t%f\n", pair_scores_gnd[i], pair_scores[i], occur_cnt[i], weight[i]);
  }
  fclose(fp);
  fclose(fp_out);
  LOG(ERROR) << "Done."; 
 
  return 0;
}

