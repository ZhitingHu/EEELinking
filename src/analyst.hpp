#ifndef ENTITY_ANALYST_HPP_
#define ENTITY_ANALYST_HPP_

#include "solver.hpp"
#include "ee_engine.hpp"
#include <vector>
#include <set>

namespace entity {

class Analyst {
public:
  Analyst(const string& snapshot_path, const int iter);
  ~Analyst() { }
 
  void ComputeNearestNeibors(const int top_k, 
      const vector<int>& candidate_entities, const string& output_filename);

  void ComputeEntityNearestNeibors(const int entity, const int top_k, 
      vector<pair<int, float> >& nearest_entities);

  void ComputeDistance(const std::vector<std::pair<int, int> >& entity_pairs, 
      std::vector<float>& pair_scores, vector<int>& occur_cnts, vector<float>& weight);
 
  float ComputeDistance(int id1, int id2);
private:
  EEEngine* ee_engine_;
  Solver* solver_;
};

}  // namespace entity

#endif
