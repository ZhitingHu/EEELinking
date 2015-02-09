#include "el_engine.hpp"
#include "gflags/gflags.h"
#include "glog/logging.h"
#include "ee_engine.hpp"
#include "solver.hpp"
#include "workload_manager.hpp"
#include "util.hpp"
#include "string_util.hpp"
#include "context.hpp"
#include "analyst.hpp"

#include <string>
#include <fstream>
#include <iterator>
#include <vector>
#include <stdint.h>
#include <thread>
#include <time.h>
#include <omp.h>
#include <math.h>
#include <stdlib.h>
#include <algorithm>

namespace entity {

  using namespace util;

  // Constructor
ELEngine::ELEngine() {
}

ELEngine::~ELEngine() {
}

void ELEngine::ReadData() {
  Context& context = entity::Context::get_instance();
  el_can_filename_ = context.get_string("el_can_filename");
  el_gt_filename_ = context.get_string("el_gt_filename");
  el_out_filename_ = context.get_string("el_out_filename");
  gt_vec_.clear();
  can_vec_.clear();
  //FILE * fp_can = fopen(el_can_filename_.c_str(), "r");
  //FILE * fp_gt = fopen(el_gt_filename_.c_str(), "r");
  //CHECK((fp_can != NULL) && (fp_gt != NULL));
  std::string str_line;
  const char *line = NULL;
  char *endptr = NULL;
  char word[MAX_WORD_LENGTH];
  std::string word_str;
  size_t num_bytes;
  int base = 10;

  // Read ground truth
  std::ifstream gt_fp;
  gt_fp.open(el_gt_filename_.c_str(), std::ios::in);
  LOG(INFO) << "Open file " << el_gt_filename_;
  CHECK(gt_fp.good()) << "Failed to open file " << el_gt_filename_;
  while (getline(gt_fp, str_line))
  {
    str_line += "\n";
    line = str_line.c_str();
    const char * ptr = line;
    if (*ptr == '\n')
        continue;
    std::vector<std::pair<std::string, int> > gt_one_vec;
    while (*ptr != '\t') ++ptr; // ignore first field
    ++ptr;
    while (*ptr != '\n') {
      int i = 0;
      while (*ptr != ':') {
        word[i] = *ptr;
        ++ptr; // find the first :
        ++i;
      }
      word[i] = '\0';
      word_str = word;
      int gt_label = strtol(++ptr, &endptr, base);
      gt_one_vec.push_back(std::make_pair(word_str, gt_label));
      ptr = endptr;
      while (*ptr == '\t') ++ptr;
    }
    gt_vec_.push_back(gt_one_vec);
  }
  gt_fp.close();
  LOG(INFO) << "File " << el_gt_filename_ << " loaded";

  // Read candidates
  std::ifstream can_fp;
  can_fp.open(el_can_filename_.c_str(), std::ios::in);
  LOG(INFO) << "Open file";
  CHECK(can_fp.good()) << "Failed to open file " << el_can_filename_;
  while (getline(can_fp, str_line))
  {
    str_line += "\n";
    line = str_line.c_str();
    const char * ptr = line;
    if (*ptr == '\n')
        continue;
    std::vector<std::pair<std::string, std::vector<int> > > can_one_vec;
    while (*ptr != '\t') ++ptr; // ignore first field
    ++ptr;
    while (*ptr != '\n') {
      int i = 0;
      while (*ptr != ':') {
        word[i] = *ptr;
        ++ptr; // find the first :
        ++i;
      }
      word[i] = '\0';
      word_str = word;
      std::vector<int> can_ids;
      int gt_label = strtol(++ptr, &endptr, base);
      can_ids.push_back(gt_label);
      ptr = endptr;
      while (*ptr == ':') {
        int gt_label = strtol(++ptr, &endptr, base);
        can_ids.push_back(gt_label);
        ptr = endptr;
      }
      can_one_vec.push_back(std::make_pair(word_str, can_ids));
      ptr = endptr;
      while (*ptr == '\t') ++ptr;
    }
    can_vec_.push_back(can_one_vec);
  }
  can_fp.close();
  LOG(INFO) << "File " << el_can_filename_ << " loaded";

  // Read entity names
  entity_strs_.clear();
  string entity_filename = context.get_string("dataset_path") + "/entity.txt";
  std::ifstream entity_fp;
  entity_fp.open(entity_filename.c_str(), std::ios::in);
  LOG(INFO) << "Open file " << entity_filename;
  CHECK(entity_fp.good()) << "Failed to open file " << entity_filename;
  while (getline(entity_fp, str_line)) {
    entity_strs_.push_back(str_line);
  }
  entity_fp.close();
  LOG(INFO) << "File " << entity_filename << " loaded";
  LOG(INFO) << "#entities " << entity_strs_.size();
}

void ELEngine::Start() {
  LOG(INFO) << "Starting...";

  Context::set_phase(entity::Context::ANALYZE);
  Context& context = entity::Context::get_instance();
  string resume_path = context.get_string("resume_path");
  int resume_iter = context.get_int32("resume_iter");
  analyst_ = new Analyst(resume_path, resume_iter);
  srand (time(NULL));
  InitEntities();
  EvalEntities();
  max_iter_ = context.get_int32("max_iter"); 
  //for (int i = 0; i < can_vec_.size(); ++i) {
  int i = 0;
    for (int iter = 0; iter < max_iter_; ++iter) {
        LOG(INFO) << "i: " << i << ", iter: " << iter;
        SampleOneDoc(i);
        EvalEntities(i);
    }
  //}

  EvalEntities();
}

void ELEngine::SampleOneDoc(int i) {
    for (int j = 0; j < can_vec_[i].size(); ++j) {
        SampleOneWord(i, j);
    }
}

void ELEngine::SampleOneWord(int i, int j) {
  std::vector<int> entity_can = can_vec_[i][j].second;
  std::vector<float> prob_can(entity_can.size());
  int ground_truth_entity_id = gt_vec_[i][j].second;
  int ground_truth_k = -1;
  float prob_sum = 0.0;
  for (int k = 0; k < prob_can.size(); ++k) {
    int entity_candidate = entity_can[k];
    if (ground_truth_entity_id == entity_candidate) {
      ground_truth_k = k;
    }
    prob_can[k] = CalcProb(entity_candidate, i, j);
    prob_sum += prob_can[k];
  }
  //LOG(INFO) << prob_sum;
  float thre = float(rand()) / RAND_MAX;
  int accept_prop = -1;
  float prob = 0;
  for (int k = 0; k < prob_can.size(); ++k) {
    if (thre < prob_can[k] / prob_sum) {
        accept_prop = entity_can[k];
        prob = prob_can[k] / prob_sum;
        break;
    } else {
        thre = thre - prob_can[k] / prob_sum;
    }
  }
  pred_vec_[i][j] = accept_prop;

  // test
  CHECK_LT(accept_prop, entity_strs_.size());
  CHECK_GE(ground_truth_k, 0);
  LOG(INFO) << j << "\t" << gt_vec_[i][j].first << "\t" 
      << entity_strs_[accept_prop] << "\t"
      << entity_strs_[ground_truth_entity_id] << "\t" << prob << "\t" 
      << (prob_can[ground_truth_k] / prob_sum) << "\t" << prob_sum;
}

double ELEngine::CalcProb(int entity_id, int i, int j) {
    double prob = 0;
#ifdef OPENMP
  #pragma omp parallel for
#endif
  for (int k = 0; k < pred_vec_[i].size(); ++k) {
    if (k == j) {
      continue;
    }
    int id1 = entity_id;
    int id2 = pred_vec_[i][k];
    double cur_prob = 1.0 / (analyst_->ComputeDistance(id1, id2) + 0.00001);
#ifdef OPENMP
  #pragma omp atomic
#endif
    prob += cur_prob;
  }
  return prob;
}

void ELEngine::InitEntities() {
    CHECK(can_vec_.size() == gt_vec_.size());
    pred_vec_.resize(can_vec_.size());
    for (int i = 0; i < can_vec_.size(); ++i) {
        pred_vec_[i].resize(can_vec_[i].size());
        for (int j = 0; j < pred_vec_[i].size(); ++j) {
            pred_vec_[i][j] = can_vec_[i][j].second[rand() % can_vec_[i][j].second.size()];
        }
    }
}

void ELEngine::EvalEntities() {
    int total_words = 0;
    int total_correct_words = 0;
    int total_possible = 0;
    bool flag = true;
    for (int i = 0; i < gt_vec_.size(); ++i) {
        for (int j = 0; j < gt_vec_[i].size(); ++j) {
            ++total_words;
            if (gt_vec_[i][j].second == pred_vec_[i][j])
                ++total_correct_words;
            flag = false;
            for (int k = 0; k < can_vec_[i][j].second.size(); ++k) {
                if (gt_vec_[i][j].second == can_vec_[i][j].second[k])
                    flag = true;
            }
            if (flag)
                ++total_possible;
        }
    }
    LOG(ERROR)<<"Evaluation result: " << float(total_correct_words) / total_words;
    LOG(ERROR)<<"Maximum possible: " << float(total_possible) / total_words;
}

void ELEngine::EvalEntities(int i) {
    int total_words = 0;
    int total_correct_words = 0;
    int total_possible = 0;
    bool flag = true;
    for (int j = 0; j < gt_vec_[i].size(); ++j) {
        ++total_words;
        if (gt_vec_[i][j].second == pred_vec_[i][j])
            ++total_correct_words;
        flag = false;
        for (int k = 0; k < can_vec_[i][j].second.size(); ++k) {
            if (gt_vec_[i][j].second == can_vec_[i][j].second[k])
                flag = true;
        }
        if (flag)
            ++total_possible;
    }
    LOG(ERROR)<<"Evaluation result for document "<< i << ": " << float(total_correct_words) / total_words;
    LOG(ERROR)<<"Maximum possible: " << float(total_possible) / total_words;
}
void ELEngine::Output() {
}

} // namespace entity
