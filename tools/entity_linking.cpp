// Author: Zhi-Ting Hu, Po-Yao Huang
// Date: 2014.10.26
#include "gflags/gflags.h"
#include "glog/logging.h"
#include "context.hpp"
#include "ee_engine.hpp"
#include "analyst.hpp"
#include "el_engine.hpp"

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
DEFINE_string(dataset_path, "", "data path");
DEFINE_string(output_file_prefix, "output/", "Results go here.");
DEFINE_string(category_filename, "categories.txt", "category filename");
DEFINE_string(entity_filename, "entity.txt", "entity filename");
DEFINE_string(entity_to_ancestor_filename, "entity2ancestor.bin", "entity-ancestor filename");
DEFINE_string(entity_to_category_filename, "entity2category.txt", "entity-category filename");
DEFINE_string(hierarchy_filename, "hierarchy.txt", "hierarchy filename");
DEFINE_string(hierarchy_id_filename, "hierarchy_id.txt", "hierarchy id filename");
DEFINE_string(pair_filename, "pair.txt", "pair id filename");
DEFINE_string(level_filename, "level.txt", "category level filename");

// Entity linking data
DEFINE_int32(truncate_level, 30, "Max number of candidate entities per mention");
DEFINE_int32(max_iter, 100, "Iteration of sampling");
DEFINE_string(el_gt_filename, "", "entity ground truth filename");
//DEFINE_string(el_can_filename, "", "entity candidates filename");
DEFINE_string(el_out_filename, "", "entity output filename");
DEFINE_string(dict_filename, "", "mention entity dictionary filename");

int main(int argc, char *argv[]) {
  FLAGS_alsologtostderr = 1;
  // Do not buffer
  FLAGS_logbuflevel = -1;
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  entity::ELEngine elengine;
  elengine.ReadData();
  elengine.Start();
  elengine.Output();

  LOG(ERROR) << "Done."; 
 
  return 0;
}

