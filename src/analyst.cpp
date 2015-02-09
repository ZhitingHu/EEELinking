// Date: 2014.10.26
#include "gflags/gflags.h"
#include "glog/logging.h"
#include "util.hpp"
#include "string_util.hpp"
#include "analyst.hpp"
#include "dataset.hpp"

#include <string>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
#include <stdint.h>
#include <algorithm>
#include <omp.h>

namespace entity {

  using namespace util;

struct sort_comparator {
  bool operator()(const std::pair<int,float> &lhs, const std::pair<int,float> &rhs) {
      return lhs.second < rhs.second;
  }
};

Analyst::Analyst(const string& resume_path, const int iter) {
  ee_engine_ = new EEEngine();
  ee_engine_->ReadData();

  solver_ = new Solver();
  solver_->Restore(resume_path, iter);
}

void Analyst::ComputeNearestNeibors(const int top_k, 
    const vector<int>& candidate_entities, const string& output_path) {
  LOG(ERROR) << "Computing nearest neibors";
  ofstream output;
  output.open((output_path + "/nearest_neibors").c_str());
  CHECK(output.is_open());

  Dataset* train_data = ee_engine_->train_data();
  int counter = 0;
//#ifdef OPENMP
//  #pragma omp parallel for
//#endif
  LOG(INFO) << "Number of candidate entities: " << candidate_entities.size();
  for (int idx = 0; idx < candidate_entities.size(); ++idx) {
    const int e_id = candidate_entities[idx];
    vector<pair<int, float> > nearest_entities;
    ComputeEntityNearestNeibors(e_id, top_k, nearest_entities);
//#ifdef OPENMP
//  #pragma omp critical
//#endif
  {
    output << e_id << " ";
    for (int rank = 0; rank < top_k; ++rank) {
      // TODO
      int occur_cnt = 0;
      if (train_data->positive_entities(e_id).find(nearest_entities[rank].first)
          != train_data->positive_entities(e_id).end()) {
        occur_cnt++;
      }
      if (train_data->positive_entities(nearest_entities[rank].first).find(e_id)
          != train_data->positive_entities(nearest_entities[rank].first).end()) {
        occur_cnt++;
      }
      
      Path* path = ee_engine_->entity_category_hierarchy()->
          FindPathBetweenEntities(e_id, nearest_entities[rank].first);

      output << nearest_entities[rank].first << ":" 
          << nearest_entities[rank].second << ":" << occur_cnt << ":" 
          << path->scale_2() << " ";
      
      delete path;
    }
    output << endl;
    counter++;
//#ifdef OPENMP
//    const int tid = omp_get_thread_num();
//    if (tid == 0)
//#endif
    {
      if (counter % (candidate_entities.size() / 10 + 1) == 0) {
        std::cout << "." << std::flush;
      }
    }
  }
  }

  std::cout << std::endl;
  output.close();
}

void Analyst::ComputeEntityNearestNeibors(const int entity, const int top_k, 
    vector<pair<int, float> >& nearest_entities) {
#ifdef OPENMP
  #pragma omp parallel for
#endif
  for (int e_id = 0; e_id < solver_->num_entity(); ++e_id) {
    if (e_id == entity) { continue; }
    Path* path = ee_engine_->entity_category_hierarchy()->
        FindPathBetweenEntities(entity, e_id);
    path->RefreshAggrDistMetric(solver_->categories());
    float dist = solver_->ComputeDist(entity, e_id, path);

#ifdef OPENMP
  #pragma omp critical
#endif
  {  
    nearest_entities.push_back(pair<int, float>(e_id, dist));
  }

    delete path;
  }

  // sort
  std::partial_sort(nearest_entities.begin(), nearest_entities.begin() + top_k, 
      nearest_entities.end(), sort_comparator());
}

void Analyst::ComputeDistance(const std::vector<std::pair<int, int> > & entity_pairs, 
    std::vector<float> & pair_scores, vector<int>& occur_cnts, vector<float>& weight) {
    Dataset* train_data = ee_engine_->train_data();
    pair_scores.clear();
    for (std::vector<std::pair<int, int> >::const_iterator it = entity_pairs.begin();
            it != entity_pairs.end(); ++it) {
        int id1 = (*it).first;
        int id2 = (*it).second;
        int occur_cnt = 0;
        if (train_data->positive_entities(id1).find(id2)
            != train_data->positive_entities(id1).end()) {
          occur_cnt++;
        }
        if (train_data->positive_entities(id2).find(id1)
            != train_data->positive_entities(id2).end()) {
          occur_cnt++;
        }
        Path* path = ee_engine_->entity_category_hierarchy()->
            FindPathBetweenEntities(id1, id2);
        path->RefreshAggrDistMetric(solver_->categories());
        float dist = solver_->ComputeDist(id1, id2, path);
        pair_scores.push_back(dist);
        occur_cnts.push_back(occur_cnt);
        weight.push_back(path->scale_2());
        delete path;
    }
}
float Analyst::ComputeDistance(int id1, int id2) {
    Path* path = ee_engine_->entity_category_hierarchy()->
      FindPathBetweenEntities(id1, id2);
    path->RefreshAggrDistMetric(solver_->categories());
    float dist = solver_->ComputeDist(id1, id2, path);
    delete path;
    return dist;
}

} // namespace entity
