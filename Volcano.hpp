#pragma once
#include <valarray>
#include <variant>
#include <array>
#include <map>
#include <vector>
#include <variant>
#include <any>
#include <string>
#include <iostream>
#include <functional>
#include <memory>
#include "Storage.hpp"
using namespace std;

struct SupportedDatatype : public variant<long, double, string> { // <- supported datatypes
	// All of this stuff is convenience functions
	using variant<long, double, string>::variant;
	template <typename T, typename = std::enable_if_t<negation<is_same<T, SupportedDatatype>>::value>>
	bool operator==(T other) const {
		return get_if<T>(this) && *get_if<T>(this) == other;
	}
	template <typename T, typename = std::enable_if_t<negation<is_same<T, SupportedDatatype>>::value>>
	bool operator<(T other) const {
		return get_if<T>(this) && *get_if<T>(this) < other;
	}
	template <typename T, typename = std::enable_if_t<negation<is_same<T, SupportedDatatype>>::value>>
	bool operator>(T other) const {
		return get_if<T>(this) && *get_if<T>(this) > other;
	}
	template <typename T, typename = std::enable_if_t<negation<is_same<T, SupportedDatatype>>::value>>
	bool operator>=(T other) const {
		return get_if<T>(this) && *get_if<T>(this) >= other;
	}
	template <typename T, typename = std::enable_if_t<negation<is_same<T, SupportedDatatype>>::value>>
	bool operator<=(T other) const {
		return get_if<T>(this) && *get_if<T>(this) <= other;
	}
	template <typename T, typename = std::enable_if_t<negation<is_same<T, SupportedDatatype>>::value>>
	friend bool operator>=(T other, SupportedDatatype const& t) {
		return get_if<T>(&t) && other >= *get_if<T>(&t);
	}
	template <typename T, typename = std::enable_if_t<negation<is_same<T, SupportedDatatype>>::value>>
	friend bool operator<=(T other, SupportedDatatype const& t) {
		return get_if<T>(&t) && other <= *get_if<T>(&t);
	}
	bool operator<(SupportedDatatype const& t) const {
		if(index() < t.index())
			return true;
		else if(index() > t.index())
			return false;
		else {
			return map<size_t, function<bool()>>{{0, [&]() { return *this < *get_if<0>(&t); }},
																					 {1, [&]() { return *this < *get_if<1>(&t); }},
																					 {2, [&]() { return *this < *get_if<2>(&t); }}}
					.at(index())();
		}
	}
	bool operator>(SupportedDatatype const& t) const { return !(*this == t || *this < t); }

	template <typename T> explicit operator T() const {
		if(!get_if<T>(this))
			throw logic_error(string("attribute does not hold a value of the requested type: ") +
												typeid(T).name());
		return *get_if<T>(this);
	}

	friend ostream& operator<<(ostream& stream, SupportedDatatype const& v) {
		if(get_if<long>(&v))
			stream << *get_if<long>(&v);
		if(get_if<string>(&v))
			stream << *get_if<string>(&v);
		if(get_if<double>(&v))
			stream << *get_if<double>(&v);
		return stream;
	}
};

struct Tuple : valarray<SupportedDatatype> {
	// tuples are equal if all their values are equal
	bool operator==(Tuple const& other) const {
		return size() == other.size() && equal(begin(*this), end(*this), begin(other));
	}

	// an empty tuple is treated as false
	operator bool() const { return size() > 0; }

	// this is convenience stuff
	using valarray<SupportedDatatype>::valarray;
	Tuple& operator=(Tuple const& other) {
		resize(other.size());
		for(size_t i = 0; i < other.size(); i++)
			(*this)[i] = other[i];
		return *this;
	}
	Tuple& operator|=(Tuple const& other) {
		if(other)
			*this = other;
		return *this;
	}
	bool operator<(Tuple const& other) const {
		if(size() != other.size()) {
			return size() < other.size();
		}
		for(size_t i = 0; i < size(); i++) {
			if((*this)[i] < other[i]) {
				return true;
			} else if((*this)[i] > other[i]) {
				return false;
			}
		}
		return false;
	}
	bool operator>(Tuple const& other) const { return !(other == *this || other < *this); }
	Tuple operator+(Tuple const& other) const {
		Tuple result(size() + other.size());
		size_t i;
		for(i = 0; i < size(); i++)
			result[i] = (*this)[i];
		for(size_t j = 0; j < other.size(); j++)
			result[i + j] = other[j];
		return result;
	}

	friend ostream& operator<<(ostream& stream, Tuple const& t) {
		for(auto it = begin(t); it != prev(end(t)); ++it)
			cout << *it << ", ";
		return stream << *prev(end(t));
	}
};

using Table = valarray<Tuple>;

struct Operator {
	virtual void open() = 0;
	virtual Tuple next() = 0;
	virtual void close() = 0;
	virtual ~Operator(){};
};

struct Scan : Operator {
	Table input;
	size_t nextTupleIndex = 0;
	Scan(Table input) : input(input){};
	void open(){};
	Tuple next() {
		return nextTupleIndex < input.size() //
							 ? input[nextTupleIndex++]
							 : Tuple{};
	};
	void close(){};
};

using Projection = function<Tuple(Tuple)>;

struct Project : Operator {
	Projection projection;
	unique_ptr<Operator> child;
	void open() { child->open(); };
	Tuple next() { return projection(child->next()); };
	void close() { child->close(); };
};

using Predicate = function<bool(Tuple)>;

struct Select : Operator {
	unique_ptr<Operator> child;
	Predicate predicate;

	Select(unique_ptr<Operator> child, Predicate predicate)
			: child(move(child)), predicate(predicate){};

	void open() { child->open(); };
	Tuple next() {
		for(auto nextCandidate = child->next(); nextCandidate; //
				nextCandidate = child->next())
			if(predicate(nextCandidate))
				return nextCandidate;
		return {};
	};
	void close() { child->close(); };
};

struct Union : Operator {
	unique_ptr<Operator> leftChild;
	unique_ptr<Operator> rightChild;
	void open() {
		leftChild->open();
		rightChild->open();
	};
	Tuple next() {
		auto nextCandidate = leftChild->next();
		return nextCandidate ? nextCandidate : rightChild->next();
	};
	void close() {
		leftChild->close();
		rightChild->close();
	};
};

struct Difference : Operator {
	unique_ptr<Operator> leftChild;
	unique_ptr<Operator> rightChild;
	vector<Tuple> bufferedRightTuples;

	void open() {
		leftChild->open();
		rightChild->open();
		for(auto rightTuple = rightChild->next(); rightTuple; //
				rightTuple = rightChild->next()) {
			bufferedRightTuples.push_back(rightTuple);
		}
	};
	Tuple next() {
		for(auto nextCandidate = leftChild->next(); nextCandidate; //
				nextCandidate = leftChild->next())
			if(find(bufferedRightTuples.begin(), bufferedRightTuples.end(), //
							nextCandidate) != bufferedRightTuples.end())
				return nextCandidate;
		return {};
	};
	void close() {
		leftChild->close();
		rightChild->close();
	};
};

struct Cross : Operator {
	unique_ptr<Operator> leftChild;
	unique_ptr<Operator> rightChild;
	Tuple currentLeftTuple{};
	vector<Tuple> bufferedRightTuples;
	size_t currentBufferedRightOffset;
	void open() {
		leftChild->open();
		rightChild->open();
		for(auto rightTuple = rightChild->next(); rightTuple; //
				rightTuple = rightChild->next())
			bufferedRightTuples.push_back(rightTuple);
		currentBufferedRightOffset = bufferedRightTuples.size();
	};
	Tuple next();
	void close() {
		leftChild->close();
		rightChild->close();
	};
	Cross(unique_ptr<Operator>&& leftChild, unique_ptr<Operator>&& rightChild)
			: leftChild(move(leftChild)), rightChild(move(rightChild)) {}
};

using AggregationFunction = function<SupportedDatatype(SupportedDatatype, Tuple)>;
struct HashTableEntry {
	bool occupied = false;
	Tuple data;
};

bool prefixesMatch(Tuple left, Tuple right);

struct GroupBy : Operator {
	unique_ptr<Operator> child;
	array<HashTableEntry, 1024> hashTable; // 1024 is magically known
	Projection getGroupKeys;
	valarray<AggregationFunction> aggregateFunctions;

	GroupBy(unique_ptr<Operator> child, Projection getGroupKeys,
					valarray<AggregationFunction> aggregateFunctions)
			: child(move(child)), aggregateFunctions(aggregateFunctions), getGroupKeys(getGroupKeys) {}
	void open();

	int outputCursor = 0;
	Tuple next() {
		while(outputCursor < hashTable.size()) {
			auto slot = hashTable[outputCursor++];
			if(slot.occupied)
				return slot.data;
		}
		return {};
	};
	void close() { child->close(); }
};

template <typename key, typename value> class LRUCache : public map<key, value> {
	using map<key, value>::map;
};

Table readFromDisk(string const& pageID);

class VolcanoBufferManager : public BufferManager<Tuple> {
	// map<string, vector<Tuple>> openPages;
	// map<string, vector<string>> pagesOnDisk;
	// size_t numberOfTuplesPerPage;
	LRUCache<size_t, Table> cache;

public:
	size_t numberOfTuplesPerPage(size_t tupleSize) {
		return BufferManager<Tuple>::numberOfTuplesPerPage(tupleSize);
	};

	size_t getNumberOfPagesForRelation(string relationName) {
		return pagesOnDisk[relationName].size();
	}

	Table& getPageForRelationWithID(string relationName, size_t id) {
		if(cache.count(id) == 0)
			cache[id] = readFromDisk(pagesOnDisk[relationName][id]);
		return cache[id];
	};
	void deleteRelation(string _) { pagesOnDisk.erase(_); }
	void createRelation(string _, size_t size, size_t tupleSize) {
		pagesOnDisk[_] = vector<string>(ceilf(float(size) / numberOfTuplesPerPage(tupleSize)));
	}
};

struct BufferedScan : Operator {
	size_t nextTupleIndexOnPage = 0;
	size_t currentPageID = 0;
	VolcanoBufferManager& bufferManager;
	string relationName;
	BufferedScan(string relationName, VolcanoBufferManager& bufferManager)
			: relationName(relationName), bufferManager(bufferManager){};
	void open(){};
	Tuple next() {
		if(nextTupleIndexOnPage ==
			 bufferManager.getPageForRelationWithID(relationName, currentPageID).size()) {
			nextTupleIndexOnPage = 0;
			currentPageID++;
		}
		return currentPageID < bufferManager.getNumberOfPagesForRelation(relationName)
							 ? bufferManager.getPageForRelationWithID(relationName,
																												currentPageID)[nextTupleIndexOnPage]
							 : Tuple{};
	};
	void close(){};
};

struct BufferedGroupBy : BufferedScan {

	unique_ptr<Operator> child;
	// array<Tuple, 1024> hashTable; // 1024 is magically known
	Projection getGroupKeys;
	valarray<AggregationFunction> aggregateFunctions;
	size_t const hashTableSize = 1024;
	// VolcanoBufferManager& bufferManager;

	static size_t nextGroupOperatorID;
	size_t groupOperatorID;

	BufferedGroupBy(unique_ptr<Operator> child, Projection getGroupKeys,
									valarray<AggregationFunction> aggregateFunctions,
									VolcanoBufferManager& bufferManager)
			: child(move(child)), aggregateFunctions(aggregateFunctions), getGroupKeys(getGroupKeys),
				groupOperatorID(nextGroupOperatorID),
				BufferedScan("groupBuffer" + to_string(nextGroupOperatorID), bufferManager) {
		nextGroupOperatorID++;
	}
	Tuple& getHashTableEntry(size_t id, Tuple const& groupKeys);
	size_t hashTableEntrySize(Tuple const& groupKeys) const;

	void open();

	int outputCursor = 0;
	Tuple next() {
		for(auto nextEntry = BufferedScan::next(); nextEntry; nextEntry = BufferedScan::next())
			if(long(nextEntry[0]) > 0)
				return nextEntry[slice(1, nextEntry.size() - 1, 1)];
		return {};
	};
	void close() {
		bufferManager.deleteRelation("groupBuffer" + to_string(nextGroupOperatorID));

		child->close();
	}
};
