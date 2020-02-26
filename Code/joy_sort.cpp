#include "joy_sort.h"
#include <algorithm>

template<typename t> void Heapify(t* Array, int Count, bool Descending){
    for (int i = Count / 2 - 1; i >= 0; i--) {
		int Left = i * 2 + 1;
		int Right = i * 2 + 2;
		int ToSwapIndex = i;
        
		if (Right < Count && ((Array[Right] > Array[ToSwapIndex]) ^ Descending)) {
			ToSwapIndex = Right;
		}
        
		if (Left < Count && ((Array[Left] > Array[ToSwapIndex]) ^ Descending)) {
			ToSwapIndex = Left;
		}
        
		if (ToSwapIndex != i) {
			std::swap<t>(Array[ToSwapIndex], Array[i]);
		}
	}
}

template<typename t> INTERNAL_FUNCTION 
void HeapifyAndExtract(t* Array, int Count, bool Descending) {
	Heapify(Array, Count, Descending);
    
	std::swap<t>(Array[0], Array[Count - 1]);
}

template<typename t> void HeapSort(t* Array, int Count, bool Descending) {
	while (Count > 0) {
		HeapifyAndExtract<t>(Array, Count, Descending);
        
		--Count;
	}
}

template<typename t> 
void InsertSort(t* Array, int Count, bool Descending){
    
}

template<typename t> 
void SelectionSort(t* Array, int Count, bool Descending){
	for (int i = 0; i < Count - 1; i++) {
		int MinIndex = i;
		for (int j = i + 1; j < Count; j++) {
			if (Array[MinIndex] > Array[j]) {
				MinIndex = j;
			}
		}

		if (MinIndex != i) {
			std::swap<t>(Array[i], Array[MinIndex]);
		}
		Array[i] = 0;
	}
}


template<typename t> 
void BubbleSort(t* Array, int Count, bool Descending){
    for(int i = 0; i < Count; i++){
        for(int j = Count - 1; j >= i; j--){
            int FirstIndex = i;
            int SecondIndex = j;
            
			if (Descending) {
				FirstIndex = j;
				SecondIndex = i;
			}

            if(Array[FirstIndex] > Array[SecondIndex]){
                std::swap<t>(Array[SecondIndex], Array[FirstIndex]);
            }
        }
    }
}
