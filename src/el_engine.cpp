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
  lambda_ = 1.0;
  log_lambda_ = log(lambda_);
}

ELEngine::~ELEngine() {
}

void ELEngine::ReadData() {
  Context& context = entity::Context::get_instance();
  const string& el_gt_filename = context.get_string("el_gt_filename");
  //el_can_filename_ = context.get_string("el_can_filename");

  gt_vec_.clear();
  can_vec_.clear();
  std::string str_line;
  const char *line = NULL;
  char *endptr = NULL;
  char word[MAX_WORD_LENGTH];
  std::string word_str;
  size_t num_bytes;
  int base = 10;

  // Read entity id & names
  entity_strs_.clear();
  string entity_filename = context.get_string("dataset_path") + "/entity.txt";
  std::ifstream entity_fp;
  entity_fp.open(entity_filename.c_str(), std::ios::in);
  LOG(INFO) << "Open file " << entity_filename;
  CHECK(entity_fp.good()) << "Failed to open file " << entity_filename;
  map<string, int> entity_name_id_map;
  int entity_id = 0;
  while (getline(entity_fp, str_line)) {
    entity_strs_.push_back(str_line);
    entity_name_id_map[str_line] = entity_id++;
  }
  entity_fp.close();
  LOG(INFO) << "File " << entity_filename << " loaded";
  CHECK_EQ(entity_strs_.size(), entity_name_id_map.size());
  LOG(INFO) << "#entities " << entity_strs_.size();

  // Read dictionary
  ReadMentionEntityDict(entity_name_id_map);

  // Read ground truth
  std::ifstream gt_fp;
  gt_fp.open(el_gt_filename.c_str(), std::ios::in);
  LOG(INFO) << "Open file " << el_gt_filename;
  CHECK(gt_fp.good()) << "Failed to open file " << el_gt_filename;
  while (getline(gt_fp, str_line))
  {
    map<string, int> mention_idx_map;
    vector<int> occur_times_doc;
    str_line += "\n";
    line = str_line.c_str();
    const char * ptr = line;
    if (*ptr == '\n')
        continue;
    vector<pair<string, int> > gt_one_vec;
    vector<pair<string, vector<int>* > > can_one_vec;
    vector<pair<string, vector<float>* > > can_prior_one_vec;

    // Parse
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
      
      if (mention_idx_map.find(word_str) == mention_idx_map.end()) {
        int idx = mention_idx_map.size();
#ifdef DEBUG
        CHECK_EQ(idx, occur_times_doc.size());
        CHECK_EQ(idx, gt_one_vec.size());
        CHECK_EQ(idx, can_one_vec.size());
        CHECK_EQ(idx, can_prior_one_vec.size());
#endif
        mention_idx_map[word_str] = idx;
        occur_times_doc.push_back(1);
        gt_one_vec.push_back(std::make_pair(word_str, gt_label));
        // add candidate entities
        CHECK(mention_cand_entities_.find(word_str)
            != mention_cand_entities_.end()) << word_str << " " << str_line;
        CHECK(mention_cand_entity_priors_.find(word_str)
            != mention_cand_entity_priors_.end()) << word_str << " " << str_line;
        can_one_vec.push_back(
            make_pair(word_str, &mention_cand_entities_[word_str]));
        can_prior_one_vec.push_back(
            make_pair(word_str, &mention_cand_entity_priors_[word_str]));
      } else {
        occur_times_doc[mention_idx_map[word_str]]++;
      }
      ptr = endptr;
      while (*ptr == '\t') ++ptr;
    }
    gt_vec_.push_back(gt_one_vec);
    can_vec_.push_back(can_one_vec);
    can_prior_vec_.push_back(can_prior_one_vec);
    occur_times_.push_back(occur_times_doc);
    mention_idx_maps_.push_back(mention_idx_map);
  }
  gt_fp.close();
  LOG(INFO) << "File " << el_gt_filename << " loaded";

  doc_idx_start_ = 17;
  doc_idx_end_ = gt_vec_.size() - 1;

  // Read candidates
  //std::ifstream can_fp;
  //can_fp.open(el_can_filename_.c_str(), std::ios::in);
  //LOG(INFO) << "Open file";
  //CHECK(can_fp.good()) << "Failed to open file " << el_can_filename_;
  //int doc_id = 0;
  //while (getline(can_fp, str_line))
  //{
  //  const map<string, int>& mention_idx_map = mention_idx_maps_[doc_id];

  //  str_line += "\n";
  //  line = str_line.c_str();
  //  const char * ptr = line;
  //  if (*ptr == '\n')
  //      continue;
  //  vector<std::pair<std::string, vector<int> > > can_one_vec;
  //  can_one_vec.resize(mention_idx_map.size());
  //  
  //  while (*ptr != '\t') ++ptr; // ignore first field
  //  ++ptr;
  //  while (*ptr != '\n') {
  //    int i = 0;
  //    while (*ptr != ':') {
  //      word[i] = *ptr;
  //      ++ptr; // find the first :
  //      ++i;
  //    }
  //    word[i] = '\0';
  //    word_str = word;
  //    vector<int> can_ids;
  //    int gt_label = strtol(++ptr, &endptr, base);
  //    can_ids.push_back(gt_label);
  //    ptr = endptr;
  //    while (*ptr == ':') {
  //      int gt_label = strtol(++ptr, &endptr, base);
  //      can_ids.push_back(gt_label);
  //      ptr = endptr;
  //    }

  //    // add
  //    CHECK(mention_idx_map.find(word_str) != mention_idx_map.end());
  //    int mention_idx = mention_idx_map.find(word_str)->second;
  //    can_one_vec[mention_idx] = std::make_pair(word_str, can_ids);

  //    ptr = endptr;
  //    while (*ptr == '\t') ++ptr;
  //  }
  //  can_vec_.push_back(can_one_vec);

  //  doc_id++;
  //}
  //can_fp.close();
  //LOG(INFO) << "File " << el_can_filename_ << " loaded";
}

// Read dictionary
// Sort & Truncate
// Normalize
void ELEngine::ReadMentionEntityDict(const map<string, int>& entity_id_map) {
  mention_cand_entities_.clear();
  mention_cand_entity_priors_.clear();

  Context& context = entity::Context::get_instance();
  string dict_filename = context.get_string("dict_filename");
  std::ifstream dict_fp;
  dict_fp.open(dict_filename.c_str(), std::ios::in);
  LOG(INFO) << "Open file " << dict_filename;

  const int truncate_level = context.get_int32("truncate_level");

  string line;
  while (getline(dict_fp, line)) {
    vector<string> parts;
    entity::Tokenize(line, parts, "\t");
    const string& mention = parts[0];
    const int cand_cnt = atoi(parts[1].c_str());

    CHECK_GT(cand_cnt, 0) << line;
    //map<string, float>& cand_entity_weight_map
    //    = mention_cand_entities_weights_[mention];
    vector<int>& cand_entities
        = mention_cand_entities_[mention];
    vector<float>& cand_entity_priors
        = mention_cand_entity_priors_[mention];
    float weight_sum = 0;
    if (cand_cnt > truncate_level) {
      // sort & truncate
      vector<pair<string, float> > cand_entities_weights;
      for (int cand_idx = 0; cand_idx < cand_cnt; ++cand_idx) {
        const string& cand_entity = parts[2 + cand_idx * 2];
        const float cand_weight = atof(parts[2 + cand_idx * 2 + 1].c_str());
        cand_entities_weights.push_back(make_pair(cand_entity, cand_weight));
      }
      std::partial_sort(cand_entities_weights.begin(),
          cand_entities_weights.begin() + truncate_level,
          cand_entities_weights.end(), DesSortBySecondOfStrFloatPair());
      for (int cand_idx = 0; cand_idx < truncate_level; ++cand_idx) {
        //cand_entity_weight_map[cand_entities_weights[cand_idx].first]
        //   = cand_entities_weights[cand_idx].second;
        const string& entity_name = cand_entities_weights[cand_idx].first;
        const float entity_prior = cand_entities_weights[cand_idx].second;
        cand_entities.push_back(entity_id_map.find(entity_name)->second);
        cand_entity_priors.push_back(entity_prior);
        weight_sum += entity_prior;
      }
    } else {
      for (int cand_idx = 0; cand_idx < cand_cnt; ++cand_idx) {
        const string& cand_entity = parts[2 + cand_idx * 2];
        const float cand_weight = atof(parts[2 + cand_idx * 2 + 1].c_str());
        //cand_entity_weight_map[cand_entity] = cand_weight;
        cand_entities.push_back(entity_id_map.find(cand_entity)->second);
        cand_entity_priors.push_back(cand_weight);
        weight_sum += cand_weight;
      }
    }

    // Normalize TODO: smooth
    if (weight_sum <= 0) {
      for (auto& weight : cand_entity_priors) {
        weight = 1 / cand_entity_priors.size();
      }
    } else {
      for (auto& weight : cand_entity_priors) {
        weight /= weight_sum;
      }
    }
  }
  dict_fp.close();

  LOG(INFO) << "dict size (#mentions) " << mention_cand_entities_.size();
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
  EvalDocs(gt_vec_.size() - 1);
  max_iter_ = context.get_int32("max_iter"); 
  // TODO
  for (int i = doc_idx_start_; i <= doc_idx_end_; ++i) {
  //int i = 3;
    for (int iter = 0; iter < max_iter_; ++iter) {
        LOG(INFO) << "i: " << i << ", iter: " << iter;
        SampleOneDoc(i);
        EvalOneDoc(i);
    }
    SampleOneDocFinalIter(i);
    EvalOneDoc(i);
    EvalDocs(i);
  }

  EvalDocs(gt_vec_.size() - 1);
}

void ELEngine::SampleOneDoc(int i) {
  for (int j = 0; j < can_vec_[i].size(); ++j) {
    SampleOneWord(i, j);
  }
}

void ELEngine::SampleOneDocFinalIter(int i) {
  for (int j = 0; j < can_vec_[i].size(); ++j) {
    SampleOneWordFinalIter(i, j);
  }
}

//void ELEngine::SampleOneWord(int i, int j) {
//  vector<int> entity_can = can_vec_[i][j].second;
//  vector<float> log_prob_can(entity_can.size());
//  float max_log_prob;
//  float log_prob_sum = 0.0;
//  int ground_truth_entity_id = gt_vec_[i][j].second;
//  int ground_truth_k = -1;
//  for (int k = 0; k < log_prob_can.size(); ++k) {
//    int entity_candidate = entity_can[k];
//    if (ground_truth_entity_id == entity_candidate) {
//      ground_truth_k = k;
//    }
//    log_prob_can[k] = CalcLogProb(entity_candidate, i, j);
//    if (k == 0) {
//      max_log_prob = log_prob_can[k];
//    } else {
//      max_log_prob = max(max_log_prob, log_prob_can[k]);
//    }
//  }
//  for (int k = 0; k < log_prob_can.size(); ++k) {
//    log_prob_sum += exp(log_prob_can[k] - max_log_prob);
//  }
//  log_prob_sum = log(log_prob_sum) + max_log_prob;
//
//  //LOG(INFO) << prob_sum;
//  float thre = float(rand()) / RAND_MAX;
//  int accept_prop = -1;
//  float sampled_prob = 0;
//  for (int k = 0; k < log_prob_can.size(); ++k) {
//    float cur_prob = exp(log_prob_can[k] - log_prob_sum);
//    if (thre < cur_prob) {
//      accept_prop = entity_can[k];
//      sampled_prob = cur_prob;
//      break;
//    } else {
//      thre = thre - cur_prob;
//    }
//  }
//  pred_vec_[i][j] = accept_prop;
//
//  // test
//  CHECK_LT(accept_prop, entity_strs_.size());
//  CHECK_GE(ground_truth_k, 0);
//  LOG(INFO) << j << "\t" << gt_vec_[i][j].first << "\t" 
//    << entity_strs_[accept_prop] << "\t"
//    << entity_strs_[ground_truth_entity_id] << "\t" << sampled_prob << "\t" 
//    << exp(log_prob_can[ground_truth_k] - log_prob_sum);
//}

void ELEngine::SampleOneWord(int i, int j) {
  const vector<int>& entity_can = *(can_vec_[i][j].second);
  const vector<float>& prior_can = *(can_prior_vec_[i][j].second);
  vector<float> prob_can(entity_can.size());
  int ground_truth_entity_id = gt_vec_[i][j].second;
  int ground_truth_k = -1;
  float prob_sum = 0.0;
  for (int k = 0; k < prob_can.size(); ++k) {
    int entity_candidate = entity_can[k];
    if (ground_truth_entity_id == entity_candidate) {
      ground_truth_k = k;
    }
    prob_can[k] = CalcProb(entity_candidate, i, j) * prior_can[k]; // add prior
    prob_sum += prob_can[k];
  }

  // Sample
  float thre = float(rand()) / RAND_MAX;
  float thre_bac = thre;
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
  if (accept_prop == -1) { //TODO
    ostringstream oss;
    oss << thre_bac << " " << thre << " " << prob_sum << ": ";
    for (int k = 0; k < prob_can.size(); ++k) {
      oss << (prob_can[k] / prob_sum) << " ";
    }
    oss << "\n";
    LOG(INFO) << oss.str();
    accept_prop = entity_can[prob_can.size() - 1];
  }
  //CHECK_LT(accept_prop, entity_strs_.size());
  //CHECK_GE(ground_truth_k, 0);
  if (ground_truth_k >= 0) {
    LOG(INFO) << j << "\t" << gt_vec_[i][j].first << "\t" 
        << entity_strs_[accept_prop] << "\t"
        << entity_strs_[ground_truth_entity_id] << "\t" << prob << "\t" 
        << (prob_can[ground_truth_k] / prob_sum) << "\t" << prob_sum;
  } else {
    LOG(INFO) << j << "\t" << gt_vec_[i][j].first << "\t" 
        << entity_strs_[accept_prop] << "\t" << prob << "\t" << prob_sum;
  }
}

void ELEngine::SampleOneWordFinalIter(int i, int j) {
  const vector<int>& entity_can = *(can_vec_[i][j].second);
  const vector<float>& prior_can = *(can_prior_vec_[i][j].second);
  vector<float> prob_can(entity_can.size());
  int ground_truth_entity_id = gt_vec_[i][j].second;
  int ground_truth_k = -1;
  float prob_sum = 0.0;
  for (int k = 0; k < prob_can.size(); ++k) {
    int entity_candidate = entity_can[k];
    if (ground_truth_entity_id == entity_candidate) {
      ground_truth_k = k;
    }
    prob_can[k] = CalcProb(entity_candidate, i, j) * prior_can[k]; // add prior
    prob_sum += prob_can[k];
  }

  // Use the entity with highest probability
  int accept_prop = -1;
  float prob = 0;
  for (int k = 0; k < prob_can.size(); ++k) {
    if (prob < prob_can[k] / prob_sum) {
        accept_prop = entity_can[k];
        prob = prob_can[k] / prob_sum;
    }
  }
  pred_vec_[i][j] = accept_prop;

  /// test
  if (accept_prop == -1) { //TODO
    ostringstream oss;
    oss << prob_sum << ": ";
    for (int k = 0; k < prob_can.size(); ++k) {
      oss << (prob_can[k] / prob_sum) << " ";
    }
    oss << "\n";
    LOG(INFO) << oss.str();
    accept_prop = entity_can[prob_can.size() - 1];
  }
  //CHECK_LT(accept_prop, entity_strs_.size());\

  //CHECK_GE(ground_truth_k, 0);
  if (ground_truth_k >= 0) {
    LOG(INFO) << j << "\t" << gt_vec_[i][j].first << "\t" 
        << entity_strs_[accept_prop] << "\t"
        << entity_strs_[ground_truth_entity_id] << "\t" << prob << "\t" 
        << (prob_can[ground_truth_k] / prob_sum) << "\t" << prob_sum;
  } else {
    LOG(INFO) << j << "\t" << gt_vec_[i][j].first << "\t" 
        << entity_strs_[accept_prop] << "\t" << prob << "\t" << prob_sum;
  }
}
double ELEngine::CalcLogProb(int entity_id, int i, int j) {
  double log_prob = 0;
#ifdef OPENMP
  #pragma omp parallel for
#endif
  for (int k = 0; k < pred_vec_[i].size(); ++k) {
    if (k == j) {
      continue;
    }
    int id1 = entity_id;
    int id2 = pred_vec_[i][k];
    //double cur_log_prob = occur_times_[i][k]
    //    * (-1.0 * lambda_ * (analyst_->ComputeDistance(id1, id2) + 0.00001) 
    //       + log_lambda_);
    double cur_log_prob = occur_times_[i][k]
        * log(1.0 / (analyst_->ComputeDistance(id1, id2) + 0.00001));
#ifdef OPENMP
    #pragma omp atomic
#endif
    log_prob += cur_log_prob;
  }
  return log_prob;
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
    prob += cur_prob * occur_times_[i][k]; //TODO: 'x' or '+' ??
  }
  return prob;
}

void ELEngine::InitEntities() {
  CHECK(can_vec_.size() == gt_vec_.size());
  pred_vec_.resize(can_vec_.size());
  for (int i = 0; i < can_vec_.size(); ++i) {
    pred_vec_[i].resize(can_vec_[i].size());
    for (int j = 0; j < pred_vec_[i].size(); ++j) {
      int rand_idx = rand() % can_vec_[i][j].second->size();
      pred_vec_[i][j] = can_vec_[i][j].second->at(rand_idx);
    }
  }
}

void ELEngine::EvalDocs(int max_doc_idx) {
  CHECK_LE(max_doc_idx, doc_idx_end_);
  CHECK_GE(max_doc_idx, doc_idx_start_);

  int total_words = 0;
  int total_correct_words = 0;
  int total_possible = 0;
  bool flag = true;
  for (int i = doc_idx_start_; i <= max_doc_idx; ++i) {
    for (int j = 0; j < gt_vec_[i].size(); ++j) {
      // Compute accuracy
      total_words += occur_times_[i][j];
      if (gt_vec_[i][j].second == pred_vec_[i][j]) {
        total_correct_words += occur_times_[i][j];
      }

      // Compute max possible accuracy
      flag = false;
      for (int k = 0; k < can_vec_[i][j].second->size(); ++k) {
        if (gt_vec_[i][j].second == can_vec_[i][j].second->at(k)) {
          flag = true;
        }
      }
      if (flag) {
        total_possible += occur_times_[i][j];
      }
    }
  }
  LOG(ERROR)<<"Evaluation result: " << float(total_correct_words) / total_words;
  LOG(ERROR)<<"Maximum possible: " << float(total_possible) / total_words;
}

void ELEngine::EvalOneDoc(int i) {
  int total_words = 0;
  int total_correct_words = 0;
  int total_possible = 0;
  bool flag = true;
  for (int j = 0; j < gt_vec_[i].size(); ++j) {
    // Compute accuracy
    total_words += occur_times_[i][j];
    if (gt_vec_[i][j].second == pred_vec_[i][j]) {
      total_correct_words += occur_times_[i][j];
    }
    
    // Compute max possible accuracy
    flag = false;
    for (int k = 0; k < can_vec_[i][j].second->size(); ++k) {
      if (gt_vec_[i][j].second == can_vec_[i][j].second->at(k)) {
        flag = true;
      }
    }
    if (flag) {
      total_possible += occur_times_[i][j];
    }
  }
  LOG(ERROR)<<"Evaluation result for document "<< i << ": " << float(total_correct_words) / total_words;
  LOG(ERROR)<<"Maximum possible: " << float(total_possible) / total_words;
}

void ELEngine::Output() {

}

} // namespace entity
