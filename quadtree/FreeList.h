#pragma once
#include "pch.h"
#include <vector>

/// Provides an indexed free list with constant-time removals from anywhere
/// in the list without invalidating indices. T must be trivially constructible 
/// and destructible.
template <class T>
class FreeList
{
public:
	/// Creates a new free list.
	FreeList();

	/// Inserts an element to the free list and returns an index to it.
	int insert(const T& element);

	// Removes the nth element from the free list.
	void erase(int n);

	// Removes all elements from the free list.
	void clear();

	// Returns the size/range of valid indices.
	int size() const;

	// Returns the nth element.
	T& operator[](int n);

	// Returns the nth element.
	const T& operator[](int n) const;

private:
	union FreeElement
	{
		T element;
		int next;
	};
	std::vector<FreeElement> data;
	int first_free;
};
