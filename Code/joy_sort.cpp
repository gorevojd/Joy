#include "joy_sort.h"

template<typename t> static void HeapifyAndExtract(t* Array, int Count, bool Descending) {
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
    
	std::swap<t>(Array[0], Array[Count - 1]);
}

template<typename t> void HeapSort(t* Array, int Count, bool Descending) {
	while (Count > 0) {
		HeapifyAndExtract(Array, Count, Descending);
        
		--Count;
	}
}