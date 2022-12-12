#include "merge_sort.h"
 
void merge(int* a, int* b, int start, int middle, int end) {
    int na1, na2, nb, i;
    na1 = start;
    nb = start;
    na2 = middle + 1;
 
    while ((na1 <= middle) && (na2 <= end)) {
        if (a[na1] <= a[na2]) {
            b[nb++] = a[na1++];
        } else {
            b[nb++] = a[na2++];
        }
    }
    if (na1 <= middle) {
        for (i = na1; i <= middle; i++) {
            b[nb++] = a[i];
        }
    }
    if (na2 <= end) {
        for (i = na2; i <= end; i++) {
            b[nb++] = a[i];
        }
    }
    for (i = start; i <= end; i++) {
        a[i] = b[i];
    }
}
 
void mergeSort(int* a, int* b, int start, int end) {
    int middle;
    if (start < end) {
        middle = (start + end) / 2;
        mergeSort(a, b, start, middle);
        mergeSort(a, b, middle + 1, end);
        merge(a, b, start, middle, end);
    }
}