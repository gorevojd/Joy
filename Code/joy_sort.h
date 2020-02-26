#ifndef JOY_SORT_H
#define JOY_SORT_H

#include "joy_defines.h"

/*
NOTE(dima): Theese algorithms assume that t-type has an overloaded
operator >. If not, the compiler error will be generated.
*/

template<typename t> void Heapify(t* Array, int Count, bool Descending = 0);
template<typename t> void HeapSort(t* Array, int Count, bool Descending = 0);

template<typename t> void InsertSort(t* Array, int Count, bool Descending);
template<typename t> void SelectionSort(t* Array, int Count, bool Descending);
template<typename t> void BubbleSort(t* Array, int Count, bool Descending);
#endif