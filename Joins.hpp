#include "Volcano.hpp"
#include "cmath"

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

private:
	vector<Tuple> bufferedRightTuples;

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

private:
	const double OVER_ALLOCATION_FACTOR = 1.8;
	size_t hashTableSize;
    HashTableEntry *hashTable;

	long hash(long v) { return v % hashTableSize; }

   	// long probe(long v) { return (v + 1) % hashTableSize; };

	long probe(long v, long & distance) { 
		distance++;
		return long((v + pow(2, distance))) % hashTableSize;
	};

public:
	HashJoin(unique_ptr<Operator> left, size_t leftAttributeID, unique_ptr<Operator> right,
					 size_t rightAttributeID, size_t rightSideCardinalityEstimate)
			: Join(move(left), leftAttributeID, move(right), rightAttributeID), hashTableSize(floor(rightSideCardinalityEstimate*OVER_ALLOCATION_FACTOR)), hashTable(new HashTableEntry[hashTableSize]){}
	void open();
	Tuple next();
	void close();
};
