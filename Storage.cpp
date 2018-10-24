#include "Storage.hpp"

class Tuple;
template<>
	size_t BufferManager<Tuple>::numberOfTuplesPerPage(unsigned long tupleSize) { //
		return 4096 / tupleSize;
	}
