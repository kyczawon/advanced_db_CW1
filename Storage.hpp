#include<map>
using namespace std;

template <typename Tuple> class BufferManager {
protected:
	map<string, vector<Tuple>> openPages;
	map<string, vector<string>> pagesOnDisk;
	size_t numberOfTuplesPerPage(size_t tupleSize);

public:
	vector<Tuple>& getOpenPageForRelation(string relationName);
	void commitOpenPageForRelation(string relationName);
	vector<vector<Tuple>> getPagesForRelation(string relationName);
};
