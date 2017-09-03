#ifndef PATHLIST_H_
#define PATHLIST_H_

typedef struct PathNode PathNode;

struct PathNode{
	PathNode* parent;
	int blockNo;
};

typedef struct{
	PathNode* lastNode;
} PathList;

/*
 * Creates and initializes a list with path from root to a data block node
 */
PathList* initPathList();
/*
 * Adds a children to the last node of this list
 */
void addChildren(PathList* list, int childNodeID);
/*
 * Frees the memory of the list and the nodes respectively
 */
void destroyPathList(PathList* list);
/*
 * Removes the last part of the path so a new one can be added
 */
void removeLast(PathList* list);

#endif /* PATHLIST_H_ */
