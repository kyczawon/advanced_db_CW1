#include "Joins.hpp"

void NestedLoopsJoin::open(){};
Tuple NestedLoopsJoin::next() { return {}; };
void NestedLoopsJoin::close(){};

void HashJoin::open(){};
Tuple HashJoin::next() { return {}; };
void HashJoin::close(){};

void SortMergeJoin::open(){};
Tuple SortMergeJoin::next() { return {}; };
void SortMergeJoin::close(){};
