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
  std::string el_can_filename_, el_gt_filename_, el_out_filename_;
  int max_iter_;
  Analyst * analyst_;
  // doc => { mention => ground truth }
  vector<vector<pair<string, int> > > gt_vec_;
  // doc => { }
  vector<vector<pair<string, vector<int> > > > can_vec_;
  // doc => { }
  vector<vector<int> > pred_vec_;

  vector<string> entity_strs_;

  void SampleOneWord(int i, int j);
  void SampleOneDoc(int i);
  void InitEntities(void);
  double CalcProb(int entity_id, int i, int j);
  void EvalEntities(void);
  void EvalEntities(int i);
};

}
