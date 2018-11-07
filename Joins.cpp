#include "Joins.hpp"

void NestedLoopsJoin::open(){
    left->open();
    right->open();
    for(Tuple rightTuple = right->next(); rightTuple; //
        rightTuple = right->next())
      bufferedRightTuples.push_back(rightTuple);
};
Tuple NestedLoopsJoin::next() {

    Tuple leftInput = left->next();

    while(leftInput)
    {
        for (size_t j = 0; j < bufferedRightTuples.size(); j++) {

            Tuple rightInput = bufferedRightTuples.at(j);
            if (leftInput[leftAttributeID] == rightInput[rightAttributeID])
            {
                return{leftInput + rightInput};
            }
        }
        leftInput = left->next();
    }

    return {};
};
void NestedLoopsJoin::close(){
    left->close();
    right->close();
};

void HashJoin::open(){
    left->open();
    right->open();

    Tuple rightInput = right->next();

    while(rightInput)
    {
        long hashValue = hash(long(rightInput[rightAttributeID]));// hash-function
        long distance = 0; //distance is used only for quadratic probing
        while(hashTable[hashValue].occupied) {
            hashValue = probe(hashValue, distance);  // probe function; 
        }
        hashTable[hashValue].occupied = true;
        hashTable[hashValue].data = rightInput;    
        
        rightInput = right->next();           
    }
};

Tuple HashJoin::next() {

    Tuple leftInput = left->next();
    while(leftInput)
    {
        long hashValue = hash(long(leftInput[leftAttributeID]));
        long distance = 0; //distance is used only for quadratic probing
        while (hashTable[hashValue].occupied && hashTable[hashValue].data[leftAttributeID] != leftInput[leftAttributeID]) {
                hashValue = probe(hashValue, distance);
            }
        if (hashTable[hashValue].occupied && hashTable[hashValue].data[leftAttributeID] == leftInput[leftAttributeID]) {
            return {leftInput + hashTable[hashValue].data};
        }
        leftInput = left->next();
    }

    return {}; 
};

void HashJoin::close(){
    left->close();
    right->close();
};

void SortMergeJoin::open(){
    left->open();
    right->open();
};
Tuple SortMergeJoin::next(
) {
    Tuple leftInput = left->next();
    Tuple rightInput = right->next();
    while (leftInput && rightInput)
    {
        if (leftInput[leftAttributeID] < rightInput[rightAttributeID])
            leftInput = left->next();
        else if (rightInput[rightAttributeID] < leftInput[leftAttributeID])
            rightInput = right->next();
        else
        {
            return {leftInput + rightInput};
            leftInput = left->next();
            rightInput = right->next();
        }
    }
    return {};
};
void SortMergeJoin::close(){
    left->close();
    right->close();
};
