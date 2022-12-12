#ifndef PARALLEL_MERGE_SORT_H_
#define PARALLEL_MERGE_SORT_H_
#include <memory>
#include <vector>

struct ProcessData {
    int comm_size;
    int rank;
    std::unique_ptr<int> random_array;
    size_t sub_array_size;
    int remainder;
    std::unique_ptr<int> sub_array;
    std::unique_ptr<int> result;
};

ProcessData getProccessData();
void scatterData(ProcessData& sort);
void gatherData(ProcessData& sort);
void makeSubArray(ProcessData& sort);

#endif  // PARALLEL_MERGE_SORT_H_