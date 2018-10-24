#include "Volcano.hpp"

Tuple Cross::next() {
	if(currentBufferedRightOffset == bufferedRightTuples.size()) {
		currentBufferedRightOffset = 0;
		currentLeftTuple = leftChild->next();
	}
	if(!bool(currentLeftTuple))
		return {};
	auto currentRightTuple = bufferedRightTuples[currentBufferedRightOffset++];
	return currentLeftTuple + currentRightTuple;
}

bool prefixesMatch(Tuple left, Tuple right) {
	for(size_t i = 0; i < min(left.size(), right.size()); i++)
		if(left[i] != right[i])
			return false;
	return true;
}

size_t nextSlot(size_t value, size_t hashtableSize);
size_t hashTuple(Tuple t, size_t hashtableSize);
void GroupBy::open() {
	child->open();
	auto inputTuple = child->next();
	while(inputTuple) {
		auto groupKeys = getGroupKeys(inputTuple);
		auto hashValue = hashTuple(groupKeys, hashTable.size());
		while(hashTable[hashValue].occupied && //
					!prefixesMatch(groupKeys, hashTable[hashValue].data))
			hashValue = nextSlot(hashValue, hashTable.size());

		hashTable[hashValue].occupied = true;
		if(hashTable[hashValue].data.size() != groupKeys.size() + aggregateFunctions.size())
			hashTable[hashValue].data.resize(groupKeys.size() + aggregateFunctions.size());
		size_t i = 0;
		for(; i < groupKeys.size(); i++)
			hashTable[hashValue].data[i] = groupKeys[i];
		for(size_t j = 0; j < aggregateFunctions.size(); j++)
			hashTable[hashValue].data[i + j] =
					aggregateFunctions[j](hashTable[hashValue].data[i + j], inputTuple);
		inputTuple = child->next();
	}
}

size_t nextSlot(size_t i, size_t hashTableSize) {
	return (i + 1) % hashTableSize;
};
size_t hashTuple(Tuple, size_t hashTableSize) { return 0; };

void simpleQuery() {
	Table input{{5l}, {7l}, {7l}, {9l}};
	auto plan =																		 //
			make_unique<GroupBy>(											 //
					make_unique<Select>(									 //
							make_unique<Scan>(input),					 //
							[](auto t) { return t[0] < 8l; }), //
					[](auto t) { return t; },							 //
					valarray<AggregationFunction>					 //
					{[](auto v, auto t) { return long(v) + 1; }});

	plan->open();
	for(auto t = plan->next(); t; t = plan->next())
		cout << t << endl;
}

void moreInterestingQuery() {
	Table input{{1l, "Holger", "180 Queens Gate"},
							{2l, "Sam", "32 Vassar Street"},
							{3l, "Peter", "180 Queens Gate"}};
	auto plan =																														 //
			make_unique<GroupBy>(																							 //
					make_unique<Select>(																					 //
							make_unique<Scan>(input),																	 //
							[](auto t) { return t[2] == string("180 Queens Gate"); }), //
					[](auto t) -> Tuple { return {t[1]}; },												 //
					valarray<AggregationFunction>																	 //
					{[](auto v, auto t) { return long(v) + 1; }});

	plan->open();
	for(auto t = plan->next(); t; t = plan->next())
		cout << t << endl;
}

Table readFromDisk(string const& _){
static map<string, Table> filesystem{};
if(filesystem.count(_) == 0)
filesystem[_] = {};
return filesystem[_];

}

size_t BufferedGroupBy::nextGroupOperatorID = 0;

size_t nextSlot(size_t value, size_t hashtableSize);
size_t hashTuple(Tuple t, size_t hashtableSize);

Tuple& BufferedGroupBy::getHashTableEntry(size_t id, Tuple const& groupKeys) {
	return bufferManager.getPageForRelationWithID(
			relationName,
			id /
					bufferManager.numberOfTuplesPerPage(hashTableEntrySize(
							groupKeys)))[id % bufferManager.numberOfTuplesPerPage(hashTableEntrySize(groupKeys))];
};

size_t BufferedGroupBy::hashTableEntrySize(Tuple const& groupKeys) const {
	return groupKeys.size() + aggregateFunctions.size() + 1;
}

void BufferedGroupBy::open() {
	auto inputTuple = child->next();
	bufferManager.createRelation("groupBuffer" + to_string(nextGroupOperatorID), hashTableSize,
															 hashTableEntrySize(getGroupKeys(inputTuple)));
	while(inputTuple) {
		auto groupKeys = getGroupKeys(inputTuple);
		auto hashValue = hashTuple(groupKeys, hashTableSize);
		for(auto& entry = getHashTableEntry(hashValue, groupKeys);
				long(entry[0]) > 0 && !prefixesMatch(groupKeys, entry[slice(1, entry.size() - 1, 1)]);
				entry = getHashTableEntry(hashValue, groupKeys)) {
		}

		// while(long(getHashTableEntry(hashValue, groupKeys)[0]) > 0 && //
		// 			!prefixesMatch(groupKeys, getHashTableEntry(
		// 																		hashValue, groupKeys)[slice(1, hashValue.size() - 1, 1)]))
		// 	hashValue = nextSlot(hashValue, hashTableSize);

		auto& hashTableEntry = getHashTableEntry(hashValue, groupKeys);
		if(hashTableEntry.size() != groupKeys.size() + aggregateFunctions.size() + 1)
			hashTableEntry.resize(groupKeys.size() + aggregateFunctions.size() + 1);
		hashTableEntry[0] = {1L};
		size_t i = 1;
		for(; i < groupKeys.size(); i++)
			hashTableEntry[i] = groupKeys[i];
		for(size_t j = 0; j < aggregateFunctions.size(); j++)
			hashTableEntry[i + j] = aggregateFunctions[j](hashTableEntry[i + j], inputTuple);
		inputTuple = child->next();
	}
}
