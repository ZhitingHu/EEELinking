#pragma once
#include <string>
#include <vector>
#include "util.hpp"
#include "string_util.hpp"
#include "context.hpp"
#include "analyst.hpp"

#define MAX_WORD_LENGTH 4096

namespace entity {

  using namespace util;
class ELEngine {
public:
  ELEngine();
  ~ELEngine();

  void ReadData();

  void Start();
  
  void Output(); 

private:
  void ReadMentionEntityDict(const map<string, int>& entity_id_map);

  void InitEntities(void);

  void SampleOneWord(int i, int j);
  void SampleOneWordFinalIter(int i, int j);
  void SampleOneDoc(int i);
  void SampleOneDocFinalIter(int i);

  double CalcLogProb(int entity_id, int i, int j);
  double CalcProb(int entity_id, int i, int j);

  void EvalDocs(int max_doc_idx);
  void EvalOneDoc(int i);

private:
  int doc_idx_start_, doc_idx_end_;
  int max_iter_;
  Analyst * analyst_;
  
  // doc => { mention => ground truth }
  vector<vector<pair<string, int> > > gt_vec_;
  // doc => { mention => idx in occur_times }
  vector<map<string, int> > mention_idx_maps_;
  // doc => { mention => occur_times}
  vector<vector<int> > occur_times_;
  // doc => { mention => { candidates } }
  vector<vector<pair<string, vector<int>* > > > can_vec_;
  vector<vector<pair<string, vector<float>* > > > can_prior_vec_;
  // doc => { predicts }
  vector<vector<int> > pred_vec_;

  vector<string> entity_strs_;

  // mention => { entity_id }
  map<string, vector<int> > mention_cand_entities_;
  // mention => { entity prior weight }
  map<string, vector<float> > mention_cand_entity_priors_;

  double lambda_;
  double log_lambda_;
};

}
