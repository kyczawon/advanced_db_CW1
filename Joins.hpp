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
	//buffer is a vector due to varying relation sizes
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
	const double OVER_ALLOCATION_FACTOR = 2; //over allocation factor determines
	size_t hashTableSize; //used to allocate the size of the hashtable based on the OVER_ALLOCATION_FACTOR
    HashTableEntry *hashTable;

	// hash and probe functions are used by both open and close functions so it's better for it to be a private function
	long hash(long v) { return v % hashTableSize; }

	// linear probing function
   	// long probe(long v) { return (v + 1) % hashTableSize; };

	// quadratic probing as seen in class
	long probe(long v, long & distance) { 
		return long((v + pow(distance++, 2))) % hashTableSize;
	};

	// quadratic probing as defined in some other sources
	// long probe(long v, long & distance) { 
	// 	return long((v + pow(2, distance++))) % hashTableSize;
	// };

public:
	//hash table pointer is initialized in the constructor with the hash table size being the rightSideCardinalityEstimate * OVER_ALLOCATION_FACTOR
	HashJoin(unique_ptr<Operator> left, size_t leftAttributeID, unique_ptr<Operator> right,
					 size_t rightAttributeID, size_t rightSideCardinalityEstimate)
			: Join(move(left), leftAttributeID, move(right), rightAttributeID), hashTableSize(floor(rightSideCardinalityEstimate*OVER_ALLOCATION_FACTOR)), hashTable(new HashTableEntry[hashTableSize]){}
	void open();
	Tuple next();
	void close();
};
