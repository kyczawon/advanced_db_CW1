#include <iostream>
#include "Volcano.hpp"
#include "Joins.hpp"
#include <unordered_set>
#include <set>
#include <algorithm>
#include <cxxabi.h>
#include <random>
#include <iomanip>
#include <chrono>

using namespace std;
static Table customerReferenceData{
		{1L, "Holger", "180 Queens Gate"}, {2L, "Sam", "32 Vassar Street"},
		{4L, "Daniel", "180 Queens Gate"}, {5L, "Robert", "32 Vassar Street"},
		{7L, "Thomas", "180 Queens Gate"}, {8L, "Mike", "32 Vassar Street"},
		{3L, "Peter", "180 Queens Gate"},	{6L, "Peter", "180 Queens Gate"}};
static Table ordersReferenceData{{1L, 2L}, {2L, 3L}, {3L, 1L}, {4L, 2L},
																 {7L, 2L}, {5L, 3L}, {9L, 1L}, {8L, 2L}};
namespace utility {
bool compareResults(unique_ptr<Operator> plan, unique_ptr<Operator> reference, string caseName) {
	multiset<Tuple> referenceResult;
	multiset<Tuple> achievedResult, delta;
	reference->open();
	for(auto t = reference->next(); t; t = reference->next())
		referenceResult.insert(t);
	reference->close();

	plan->open();
	for(auto t = plan->next(); t; t = plan->next())
		achievedResult.insert(t);
	plan->close();

	set_symmetric_difference(referenceResult.begin(), referenceResult.end(), achievedResult.begin(),
													 achievedResult.end(), inserter(delta, end(delta)));
	cout << caseName << " correct: " << boolalpha << (delta.size() == 0) << endl;
	if(delta.size() > 20)
		cout << "way to many differences to display" << endl;
	else {
		for_each(begin(delta), end(delta), [&referenceResult](auto const& t) {
			cout << "(" << t << "): " << (referenceResult.count(t) ? "missing" : "superfluous")
					 << " in result" << endl;
		});
	}
	return delta.size() == 0;
}
}
namespace reference {
auto joinUsingCrossProduct(Table& customer, Table& orders, bool tablesSwapped = false) {
	return make_unique<Select>(
			make_unique<Cross>(make_unique<Scan>(customer), make_unique<Scan>(orders)),
			[tablesSwapped](auto t) { return t[0] == t[tablesSwapped ? 2 : 3]; });
}
unique_ptr<Operator> inequalityJoin(Table& customer, Table& orders) {
	return make_unique<Select>(
			make_unique<Cross>(make_unique<Scan>(customer), make_unique<Scan>(orders)),
			[](auto t) { return t[3] < long(t[0]) + 1 && t[3] > long(t[0]) - 1; });
}
}

namespace queries {
template <typename JoinImplementation>
unique_ptr<Operator> equalityJoin(Table& customer, Table& orders) {
	return make_unique<JoinImplementation>(make_unique<Scan>(customer), 0, make_unique<Scan>(orders),
																				 0, orders.size());
}

template <typename JoinImplementation>
unique_ptr<Operator> inequalityJoin(Table& customer, Table& orders) {
	return make_unique<JoinImplementation>(make_unique<Scan>(customer), 0, pair{-1, 1},
																				 make_unique<Scan>(orders), 0, orders.size());
}
}

namespace run {
template <typename T> void equalityQuery(bool needsSorting = false) {
	auto customer = customerReferenceData;
	auto orders = ordersReferenceData;
	if(needsSorting) {
		sort(begin(customer), end(customer),
				 [](auto const& t1, auto const& t2) { return t1[0] < t2[0]; });
		sort(begin(orders), end(orders), [](auto const& t1, auto const& t2) { return t1[0] < t2[0]; });
	}
	cout << "Testing Equality Join: " << abi::__cxa_demangle(typeid(T).name(), 0, 0, NULL) << "..."
			 << endl;
	utility::compareResults(queries::equalityJoin<T>(customer, orders),
													reference::joinUsingCrossProduct(customer, orders), "Customer x Orders");
	utility::compareResults(queries::equalityJoin<T>(orders, customer),
													reference::joinUsingCrossProduct(orders, customer, true),
													"Orders x Customer");
	cout << "--------------------" << endl;
}

template <typename T> void inequalityQuery() {
	auto customer = customerReferenceData;
	auto orders = ordersReferenceData;
	cout << "Testing Inequality Join: " << abi::__cxa_demangle(typeid(T).name(), 0, 0, NULL) << "..."
			 << endl;
	utility::compareResults(queries::inequalityJoin<T>(customer, orders),
													reference::inequalityJoin(customer, orders), "Customer <> Orders");
	cout << "--------------------" << endl;
}
}

namespace runAtScale {
pair<Table, Table> generateData(size_t scale, bool needsSorting) {
	Table customer(scale);
	for(size_t i = 0; i < customer.size(); i++)
		customer[i] = Tuple({long(i * 3), long((i * 312839) % 372189), long(17 * i)});
	Table orders(scale * 3);
	for(size_t i = 0, j = 0; i < orders.size(); i++)
		orders[i] = Tuple({long((j += 1 + (rand() % 3))), long(17 * i)});
	if(!needsSorting) {
		std::random_device rd;
		std::mt19937 g(rd());
		shuffle(begin(customer), end(customer), g);
		shuffle(begin(orders), end(orders), g);
	}
	return {customer, orders};
}

template <typename T>
void equalityQuery(size_t scale = 1000, bool needsSorting = false, bool checkCorrectness = true) {
	auto generatedData = generateData(scale, needsSorting);
	auto customer = generatedData.first;
	auto orders = generatedData.second;
	cout << "Testing Equality Join At Scale " << setw(9) << scale << " : "
			 << abi::__cxa_demangle(typeid(T).name(), 0, 0, NULL) << "...";
	auto case1Start = chrono::high_resolution_clock::now();

	auto case1 = queries::equalityJoin<T>(customer, orders);
	if(!checkCorrectness) {
		case1->open();
		for(auto t = case1->next(); t; t = case1->next()) {
		}
		case1->close();

	}

	auto between = chrono::high_resolution_clock::now();
	auto case2 = queries::equalityJoin<T>(orders, customer);
	if(!checkCorrectness) {
		case2->open();
		for(auto t = case2->next(); t; t = case2->next()) {
		}
		case2->close();
	}
	auto case2Finish = chrono::high_resolution_clock::now();
	if(checkCorrectness) {
		cout << endl;
		utility::compareResults(move(case1), reference::joinUsingCrossProduct(customer, orders),
														"Customer x Orders");
		utility::compareResults(move(case2), reference::joinUsingCrossProduct(orders, customer, true),
														"Orders x Customer");
		cout << "--------------------" << endl;
	} else {
		std::cout << "Customer Left: " << setw(9)
							<< (chrono::duration_cast<chrono::microseconds>(between - case1Start).count())
							<< "us";
		std::cout << ", Orders Left: " << setw(9)
							<< (chrono::duration_cast<chrono::microseconds>(case2Finish - between).count())
							<< "us" << endl;
	}
}
}

int main() {
	bool enableBonus = false;

	cout << endl;
	cout << endl << "testing correctness";
	cout << endl << "===================" << endl;
	run::equalityQuery<NestedLoopsJoin>();
	run::equalityQuery<HashJoin>();
	run::equalityQuery<SortMergeJoin>(true);
	cout << endl;
	cout << endl << "scaling experiments ";
	cout << endl << "===================" << endl;

	for(size_t i = 10; i < 1001; i *= 10) {
		runAtScale::equalityQuery<NestedLoopsJoin>(i);
		runAtScale::equalityQuery<HashJoin>(i);
		runAtScale::equalityQuery<SortMergeJoin>(i, true);
	}

	cout << endl;
	cout << endl << "performance testing";
	cout << endl << "===================" << endl;
	for(size_t i = 10; i < 100000; i *= 10)
		runAtScale::equalityQuery<NestedLoopsJoin>(i, false, false);
	for(size_t i = 10; i < 1000000; i *= 10)
		runAtScale::equalityQuery<HashJoin>(i, false, false);
	for(size_t i = 10; i < 1000000; i *= 10)
		runAtScale::equalityQuery<SortMergeJoin>(i, true, false);

	if(enableBonus)
		run::inequalityQuery<SortMergeJoin>();
	return 0;
}
