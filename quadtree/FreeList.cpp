#include "pch.h"
#include "FreeList.h"

template <class T>
FreeList<T>::FreeList() : first_free(-1)
{
}

// Insert an element into the FreeList - either an empty node or a new node
// Return the index to where the element was inserted
template <class T>
int FreeList<T>::insert(const T& element)
{
	if (first_free != -1)
	{
		const int index = first_free;
		first_free = data[first_free].next;
		data[index].element = element;
		return index;
	}
	else
	{
		FreeElement fe;
		fe.element = element;
		data.push_back(fe);
		return static_cast<int>(data.size() - 1);
	}
}

// Erase the element at index n
// The empty node becomes the first in the free linked list
template <class T>
void FreeList<T>::erase(int n)
{
	data[n].next = first_free;
	first_free = n;
}

template <class T>
void FreeList<T>::clear()
{
	data.clear();
	first_free = -1;
}

template <class T>
int FreeList<T>::size() const
{
	return static_cast<int>(data.size());
}

template <class T>
T& FreeList<T>::operator[](int n)
{
	return data[n].element;
}

template <class T>
const T& FreeList<T>::operator[](int n) const
{
	return data[n].element;
}