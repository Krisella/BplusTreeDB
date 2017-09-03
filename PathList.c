#include "PathList.h"
#include <stdio.h>
#include <stdlib.h>

void addChildren(PathList* list, int childNodeID) {
	PathNode* temp = malloc(sizeof(PathNode));
	temp->blockNo = childNodeID;
	temp->parent = list->lastNode;
	list->lastNode = temp;
}

void destroyPathList(PathList* list) {

	while(list->lastNode != NULL) {
		PathNode* temp = list->lastNode;
		list->lastNode = temp->parent;
		free(temp);
	}
	free(list);
	list = NULL;
}

void removeLast(PathList* list) {
	PathNode* temp = list->lastNode;
	list->lastNode = temp->parent;
	free(temp);
}

PathList* initPathList() {
	PathList* list = malloc(sizeof(PathList));
	list->lastNode = NULL;
	return list;
}
