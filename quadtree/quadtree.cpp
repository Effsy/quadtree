#include "pch.h"
#include <iostream>
#include "FreeList.h"
#include <vector>

// Represents a node in the quadtree.
struct QuadNode
{
	// Points to the first child if this node is a branch or the first
	// element node if this node is a leaf.
	int firstChildIndex;

	// Stores the number of elements in the leaf or -1 if it this node is
	// not a leaf
	int count;

	QuadNode(int firstChildIndex, int count) : firstChildIndex(firstChildIndex), count(count) {
	}
};

// Represents an element node in the quadtree.
struct QuadElementNode
{
	// Points to the next element node in the leaf node. A value of -1
	// indicates the end of the list.
	int nextIndex;

	// Stores the element index.
	int elementIndex;

	QuadElementNode(int nextIndex, int elementIndex)
		: nextIndex(nextIndex), elementIndex(elementIndex) {
	}
};

// Represents an element in the quadtree.
struct QuadElement
{
	// Stores the ID for the element (can be used to
	// refer to external data).
	int id;

	// Stores the rectangle, AABB for the element.
	// x1, y1 = top left point
	// x2, y2 = bottom right point
	int x1, y1, x2, y2;

	QuadElement(int id, int x1, int y1, int x2, int y2)
		: id(id), x1(x1), y1(y1), x2(x2), y2(y2) {
	}
};

// Contains the AABB, depth and the index of the Node in the nodes FreeList. The AABB is calculated at runtime
struct QuadNodeData
{
	// The index of the node in the nodes FreeList this QuadNodeData object refers to
	int nodeIndex;

	// The depth of the node
	int depth;

	// Mid point
	int mx;
	int my;
	// Half-lengthsQuadElt
	int hx;
	int hy;

	QuadNodeData(int nodeIndex, int depth, int mx, int my, int hx, int hy)
		: nodeIndex(nodeIndex), depth(depth), mx(mx), my(my), hx(hx), hy(hy) {
	}

};
class Quadtree;

class IQuadtreeVisitor
{
	// Called when traversing a branch node.
	// (mx, my) indicate the center of the node's AABB.
	// (hx, hy) indicate the half-size of the node's AABB.
public:
	virtual void branch(const Quadtree& quadtree, const int node, const int depth, const int mx, const int my, const int hx, const int hy) const = 0;

	// Called when traversing a leaf node.
	// (mx, my) indicate the center of the node's AABB.
	// (hx, hy) indicate the half-size of the node's AABB.
	virtual void leaf(const Quadtree& quadtree, const int node, const int depth, const int mx, const int my, const int hx, const int hy) const = 0;
};

class Quadtree
{
private:
	// The maximum depth allowed for the quadtree.
	int maxDepth;
	// The maximum numbeer of elements allowed in a leaf before subdividing
	int maxElements;

	// Stores all the elements in the quadtree.
	FreeList<QuadElement> elements;

	// Stores all the element nodes in the quadtree.
	FreeList<QuadElementNode> elementNodes;

	// Stores all the nodes in the quadtree. The first node in this
	// sequence is always the root.
	FreeList<QuadNode> nodes;

	// Stores the first free node in the quadtree to be reclaimed as 4
	// contiguous nodes at once. A value of -1 indicates that the free
	// list is empty, at which point we simply insert 4 nodes to the
	// back of the nodes array.
	int freeNodeIndex;

	// Temp buffer variables for queries - used to check if element is already found (avoid returning repeated elements)
	std::vector<bool> tempBuffer;

	void nodeInsert(const int nodeIndex, const int depth, const int mx, const int my, const int hx, const int hy, const int elementIndex);
	void leafInsert(const int nodeIndex, const int depth, const int mx, const int my, const int hx, const int hy, const int elt);
	std::vector<QuadNodeData> findLeaves(const int nodeIndex, const int depth, const int mx, const int my, const int hx, const int hy, const int left, const int right, const int top, const int bottom);
	void traverse(const IQuadtreeVisitor& visitor);
	static bool intersect(const int l1, const int r1, const int t1, const int b1, const int l2, const int r2, const int t2, const int b2);

public:
	int insert(const int id, const int x1, const int y1, const int x2, const int y2);
	void remove(const int elementIndex);
	std::vector<QuadElement> query(const float x1, const float y1, const float x2, const float y2, const int omitElementIndex);
	std::vector<QuadElement> query(const float x1, const float y1, const float x2, const float y2);
	void cleanup();
	Quadtree(const int width, const int height, const int startMaxElements, const int maxDepth, const int tempBufferSize);

	// Stores the quadtree extents - boundaries
	const int rootMx, rootMy, rootHx, rootHy;
};

// Insert an element into the quadtree - make sure the element's fields are initialised
int Quadtree::insert(const int id, const int x1, const int y1, const int x2, const int y2)
{
	const int newElementIndex = elements.insert(QuadElement(id, x1, y1, x2, y2));
	nodeInsert(0, 0, rootMx, rootMy, rootHx, rootHy, newElementIndex);
	return newElementIndex;
}

// Remove an element from the quadtree - removes all element nodes and the element itself
void Quadtree::remove(const int elementIndex)
{
	const QuadElement & e = elements[elementIndex];
	std::vector<QuadNodeData> leaves = findLeaves(0, 0, rootMx, rootMy, rootHx, rootHy, e.x1, e.y1, e.x2, e.y2);

	// For each leaf node remove the element nodes
	for (int i = 0; i < leaves.size(); i++) {

		// The index of the Node object in nodes the QuadNodeData object refers to
		const int nodeIndex = leaves[i].nodeIndex;

		// Traverse the list until the element node is found
		int elementNodeIndex = nodes[nodeIndex].firstChildIndex;
		int prevElementNodeIndex = -1;
		while (elementNodeIndex != -1 && elementNodes[elementNodeIndex].elementIndex != elementIndex) {
			prevElementNodeIndex = elementNodeIndex;
			elementNodeIndex = elementNodes[elementNodeIndex].nextIndex;
		}
		// If nodeIndex == -1, the element could not be found in the leaf
		if (elementNodeIndex != -1) {
			// Remove the element node (LinkedList removal)
			const int nextIndex = elementNodes[elementNodeIndex].nextIndex;
			if (prevElementNodeIndex == -1) {
				nodes[nodeIndex].firstChildIndex = nextIndex;
			}
			else {
				elementNodes[prevElementNodeIndex].nextIndex = nextIndex;
			}

			elementNodes.erase(elementNodeIndex);
			nodes[nodeIndex].count--;
		}
	}
	// Remove the element itself
	elements.erase(elementIndex);
}
//class A : public IQuadtreeVisitor {
//	virtual void branch(const Quadtree& quadtree, const int node, const int depth, const int mx, const int my, const int hx, const int hy) override {
//
//	}
//
//	virtual void leaf(const Quadtree& quadtree, const int node, const int depth, const int mx, const int my, const int hx, const int hy) override {
//
//	}
//};

// Traverse all the nodes in the tree, calling 'branch' for branch nodes and 'leaf' for leaf nodes
void Quadtree::traverse(const IQuadtreeVisitor& visitor)
{
	std::vector<QuadNodeData> toProcess;
	toProcess.push_back(QuadNodeData(0, 0, rootMx, rootMy, rootHx, rootHy));

	while (toProcess.size() > 0) {
		const QuadNodeData nodeData = toProcess.back();
		toProcess.pop_back();
		const QuadNode & node = nodes[nodeData.nodeIndex];
		const int fc = node.firstChildIndex;

		// If the node is not a leaf
		if (node.count == -1) {

			// Push the child nodes onto the list (calculate the AABB)
			const int hx = nodeData.hx >> 1, hy = nodeData.hy >> 1;
			const int leftMx = nodeData.mx - hx, topMy = nodeData.my - hy, rightMx = nodeData.mx + hx, bottomMy = nodeData.my + hy;
			toProcess.push_back(QuadNodeData(fc + 0, nodeData.depth + 1, leftMx, topMy, hx, hy));
			toProcess.push_back(QuadNodeData(fc + 1, nodeData.depth + 1, rightMx, topMy, hx, hy));
			toProcess.push_back(QuadNodeData(fc + 2, nodeData.depth + 1, leftMx, bottomMy, hx, hy));
			toProcess.push_back(QuadNodeData(fc + 3, nodeData.depth + 1, rightMx, bottomMy, hx, hy));

			// Visit the branch
			visitor.branch(*this, nodeData.nodeIndex, nodeData.depth, nodeData.mx, nodeData.my, nodeData.hx, nodeData.hy);
		}
		else {
			visitor.leaf(*this, nodeData.nodeIndex, nodeData.depth, nodeData.mx, nodeData.my, nodeData.hx, nodeData.hy);
		}
	}

}


// Returns a list of elements found in the specified rectangle excluding the
// specified element to omit.
std::vector<QuadElement> Quadtree::query(const float x1, const float y1, const float x2, const float y2, const int omitElementIndex)
{
	// Contains the elements
	std::vector<QuadElement> out;
	// Keeps track of bool
	std::vector<int> clearList;

	// Cast coordinates to int
	const int qx1 = static_cast<int>(x1);
	const int qy1 = static_cast<int>(y1);
	const int qx2 = static_cast<int>(x2);
	const int qy2 = static_cast<int>(y2);

	std::vector<QuadNodeData> leaves = findLeaves(0, 0, rootMx, rootMy, rootHx, rootHy, qx1, qy1, qx2, qy2);

	// tempBuffer is used to track whether an element has already been added (elementNodes)
	// Increase temporary buffer size to acomodate number of elements
	// Check speed - may be faster by avoiding resizing multiple times, may be slower by unnecessary check
	if (tempBuffer.size() < elements.size()) {
		tempBuffer.assign(elements.size(), false);
	}

	// For each leaf, look for elements that intersect the AABB
	for (int i = 0; i < leaves.size(); i++) {
		const int nodeIndex = leaves[i].nodeIndex;

		// Walk the list and add elements that intersect
		int elementNodeIndex = nodes[nodeIndex].firstChildIndex;
		while (elementNodeIndex != -1) {
			// elementIndex checks
			const int elementIndex = elementNodes[elementNodeIndex].elementIndex;
			if (!tempBuffer[elementIndex] && elementIndex != omitElementIndex) {
				const QuadElement & e = elements[elementIndex];
				// Element checks
				if (intersect(qx1, qy1, qx2, qy2, e.x1, e.y1, e.x2, e.y2)) {
					// Element found - added to query result
					out.push_back(e);
					tempBuffer[elementIndex] = true;
					clearList.push_back(elementIndex);
				}
			}
			elementNodeIndex = elementNodes[elementNodeIndex].nextIndex;
		}
	}

	// Clear the element buffer - Unmark inserted elements
	for (int i = 0; i < clearList.size(); i++) {
		tempBuffer[clearList[i]] = false;
	}
	return out;
}

// Returns a list of elements found in the specified rectangle
std::vector<QuadElement> Quadtree::query(const float x1, const float y1, const float x2, const float y2)
{
	return query(x1, y1, x2, y2, -1);
}

// Clean up the tree, removing empty leaves
void Quadtree::cleanup()
{
	std::vector<QuadNode> toProcess;

	// Only process if the root is not a leaf
	if (nodes[0].count == -1) {
		toProcess.push_back(nodes[0]);
	}

	while (toProcess.size() > 0) {
		QuadNode node = toProcess.back();
		toProcess.pop_back();

		int numEmptyLeaves = 0;

		// Loop through the children
		for (int i = 0; i < 4; i++) {

			const QuadNode& child = nodes[node.firstChildIndex + i];

			// Increment empty leaf count if the child is an empty leaf
			// Otherwise if the child is a branch, add it to the stack to be processed in the next iteration
			// Don't do anything if the child is a leaf with children
			if (child.count == 0) {
				numEmptyLeaves++;
			}
			else if (child.count == -1) {
				toProcess.push_back(child);
			}
		}

		// If all the children were empty leaves, remove them and 
		// make this node the new empty leaf.
		if (numEmptyLeaves == 4)
		{
			// Push all 4 children to the free list. Note: See quadtree declaration for freeNode
			nodes[node.firstChildIndex].firstChildIndex = freeNodeIndex;
			freeNodeIndex = node.firstChildIndex;

			// Make this node the new empty leaf.
			node.firstChildIndex = -1;
			node.count = 0;
		}
	}
}

// Insert an element into a node
// Takes the fields of a QuadNodeData object and an index to the element being inserted
void Quadtree::nodeInsert(const int index, const int depth, const int mx, const int my, const int hx, const int hy, const int elementIndex)
{
	const int x1 = elements[elementIndex].x1;
	const int y1 = elements[elementIndex].y1;
	const int x2 = elements[elementIndex].x2;
	const int y2 = elements[elementIndex].y2;

	std::vector<QuadNodeData> leaves = findLeaves(index, depth, mx, my, hx, hy, x1, y1, x2, y2);

	for (int i = 0; i < leaves.size(); i++) {
		leafInsert(leaves[i].nodeIndex, leaves[i].depth, leaves[i].mx, leaves[i].my, leaves[i].hx, leaves[i].hy, elementIndex);
	}
}

// Insert a Quad Element into a particular leaf. Subdivide and reinsert if the leaf is full
// Takes the fields of a QuadNodeData object and an index to the element being inserted
void Quadtree::leafInsert(const int nodeIndex, const int depth, const int mx, const int my, const int hx, const int hy, const int elementIndex)
{
	// Insert the element into the beginning of the leaf's linked list of elements as the first child

	// Store the original first child (first child is an element index as the node is a leaf)
	const int prevFirstChildIndex = nodes[nodeIndex].firstChildIndex;
	QuadNode & node = nodes[nodeIndex];
	// Replace the first child with the new QuadElementNode
	node.firstChildIndex = elementNodes.insert(QuadElementNode(prevFirstChildIndex, elementIndex));

	// Subdivide if leaf is full
	if (node.count == maxElements && depth < maxDepth) {

		//Transfer elements from the leaf node to a list of elements
		std::vector<int> tempElements;

		// While the leaf still contains an element
		while (node.firstChildIndex != -1) {
			const int elementNodeIndex = node.firstChildIndex;

			const int nextElementNodeIndex = elementNodes[elementNodeIndex].nextIndex;
			const int elementIndex = elementNodes[elementNodeIndex].elementIndex;

			// Pop off the element node from the leaf and remove it from the quadtree
			node.firstChildIndex = nextElementNodeIndex;
			elementNodes.erase(elementNodeIndex);

			// Insert the element into the temporary list
			tempElements.push_back(elementIndex);
		}

		// Set the first child of the current node and allocate 4 empty child nodes
		node.firstChildIndex = nodes.insert(QuadNode(-1, 0));
		nodes.insert(QuadNode(-1, 0));
		nodes.insert(QuadNode(-1, 0));
		nodes.insert(QuadNode(-1, 0));

		// Transfer the elements in the former leaf node to its new children
		for (int i = 0; i < tempElements.size(); ++i) {
			nodeInsert(nodeIndex, depth, mx, my, hx, hy, tempElements[i]);
		}
	}
	else {
		node.count++;
	}
}

// Finds all leaves within a certain AABB starting from a particular node (QuadNodeData)
// x1, y1, x2, y2 = top-left and bottom-right of the AABB. QuadNodeData is the first node to check.
// After checking the first node, traverse through all child nodes and return all the relevant leaves
// Takes the fields of a QuadNodeData object and the fields of an AABB
std::vector<QuadNodeData> Quadtree::findLeaves(const int nodeIndex, const int depth, const int mx, const int my, const int hx, const int hy,
	const int x1, const int y1, const int x2, const int y2)
{
	std::vector<QuadNodeData> leaves, toProcess;
	toProcess.push_back(QuadNodeData(nodeIndex, depth, mx, my, hx, hy));

	while (toProcess.size() > 0) {
		const QuadNodeData nodeData = toProcess.back();
		toProcess.pop_back();
		// If this node is a leaf, insert it to the list.
		if (nodes[nodeData.nodeIndex].count != -1) {
			leaves.push_back(nodeData);
		}
		else {
			// Otherwise push the children that intersect the rectangle.

			//Calculate the bounding box of the current node
			const int mx = nodeData.mx, my = nodeData.my;
			const int hx = nodeData.hx >> 1, hy = nodeData.hy >> 1;
			const int fc = nodes[nodeData.nodeIndex].firstChildIndex;
			// Calculate the centers of the 4 children 
			const int leftMx = mx - hx, topMy = my - hx, rightMx = mx + hx, bottomMy = my + hy;

			// Compare the AABB with the four child nodes to check for intersections
			// Push any intersecting child nodes
			if (y1 <= my) {
				if (x1 <= mx)
					toProcess.push_back(QuadNodeData(fc + 0, nodeData.depth + 1, leftMx, topMy, hx, hy));
				if (x2 > mx)
					toProcess.push_back(QuadNodeData(fc + 1, nodeData.depth + 1, rightMx, topMy, hx, hy));
			}
			if (y2 > my) {
				if (x1 <= mx)
					toProcess.push_back(QuadNodeData(fc + 2, nodeData.depth + 1, leftMx, bottomMy, hx, hy));
				if (x2 > mx)
					toProcess.push_back(QuadNodeData(fc + 3, nodeData.depth + 1, rightMx, bottomMy, hx, hy));
			}
		}
	}
	return leaves;
}

// Standard AABB intersection check
bool Quadtree::intersect(const int x1A, const int y1A, const int x2A, const int y2A,
	const int x1B, const int y1B, const int x2B, const int y2B)
{
	return x1B <= x2A && x2B >= x1A && y1B <= y2A && y2B >= y1A;
}

//void Quadtree::write(ostream & o) const
//{
//
//}

Quadtree::Quadtree(const int width, const int height, const int maxElements, const int maxDepth, const int tempBufferSize)
	: maxElements(maxElements), maxDepth(maxDepth), rootMx(width / 2), rootMy(height / 2), rootHx(width / 2), rootHy(height / 2), tempBuffer(tempBufferSize, false)
{
	// Insert root
	nodes.insert(QuadNode(-1, -1));
}

class Entity {
public:
	int id;
	int x1, y1, x2, y2;
	Entity(int id, int x1, int y1, int x2, int y2) : id(id), x1(x1), y1(y1), x2(x2), y2(y2) {

	}
};

int main()
{
	std::cout << "asdsadasd";
	Quadtree quadtree(1000, 1000, 50, 10, 100);
	Entity entity(1, 10, 10, 20, 20);
	quadtree.insert(entity.x1, entity.x1, entity.y1, entity.x2, entity.y2);

	/*std::vector<Entity> entities;
	for (int i = 0; i < 1; ++i) {
		entities.push_back(Entity(i * ));
	}*/



}


/*
querys and collision detection

traversed = {}
gather quadtree leaves
for each leaf in leaves:
{
	 for each element in leaf:
	 {
		  if not traversed[element]:
		  {
			  use quad tree to check for collision against other elements
			  traversed[element] = true
		  }
	 }
}

grid_x1 = floor(element.x1 / cell_size);
grid_y1 = floor(element.y1 / cell_size);
grid_x2 = floor(element.x2 / cell_size);
grid_y2 = floor(element.y2 / cell_size);

for grid_y = grid_y1, grid_y2:
{
	 for grid_x = grid_x1, grid_x2:
	 {
		 for each other_element in cell(grid_x, grid_y):
		 {
			 if element != other_element and collide(element, other_element):
			 {
				 // The two elements intersect. Do something in response
				 // to the collision.
			 }
		 }
	 }
}


*/