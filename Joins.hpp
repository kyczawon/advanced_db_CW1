#include "Volcano.hpp"

class Join : public Operator {
protected:
	unique_ptr<Operator> left;
	size_t leftAttributeID;
	unique_ptr<Operator> right;
	size_t rightAttributeID;
	Join(unique_ptr<Operator> left, size_t leftAttributeID, unique_ptr<Operator> right,
			 size_t rightAttributeID)
			: left(move(left)), leftAttributeID(leftAttributeID), right(move(right)),
				rightAttributeID(rightAttributeID) {}
};

class NestedLoopsJoin : public Join {

public:
	NestedLoopsJoin(unique_ptr<Operator> left, size_t leftAttributeID, unique_ptr<Operator> right,
									size_t rightAttributeID, size_t rightSideCardinalityEstimate)
			: Join(move(left), leftAttributeID, move(right), rightAttributeID) {}
	void open();
	Tuple next();
	void close();
};

class SortMergeJoin : public Join {

public:
	SortMergeJoin(unique_ptr<Operator> left, size_t leftAttributeID, pair<long, long> windowOnTheLeft,
								unique_ptr<Operator> right, size_t rightAttributeID,
								size_t rightSideCardinalityEstimate)
			: Join(move(left), leftAttributeID, move(right), rightAttributeID) {}
	SortMergeJoin(unique_ptr<Operator> left, size_t leftAttributeID, unique_ptr<Operator> right,
								size_t rightAttributeID, size_t rightSideCardinalityEstimate)
			: SortMergeJoin(move(left), leftAttributeID, {0, 0}, move(right), rightAttributeID,
											rightSideCardinalityEstimate) {}

	void open();

	Tuple next();
	void close();
};

class HashJoin : public Join {

public:
	HashJoin(unique_ptr<Operator> left, size_t leftAttributeID, unique_ptr<Operator> right,
					 size_t rightAttributeID, size_t rightSideCardinalityEstimate)
			: Join(move(left), leftAttributeID, move(right), rightAttributeID) {}
	void open();
	Tuple next();
	void close();
};
