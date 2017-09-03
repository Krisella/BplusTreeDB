#ifndef OPERATIONS_H_
#define OPERATIONS_H_

#include "defn.h"
#include "AM.h"

/* Checks if given file index has any file with given name
 * fineName: Filename to search for
 * fileIfnoIndex: File index array in which to search
 * Returns 1 if the filename exists
 * Returns 0 if the filename does not exist
 */
int fileNameIsOpen(char* fileName, FileInfo* fileInfoIndex);


/* Checks if given file index has any file with given file ID
 * id: ID to search for
 * fileIfnoIndex: File index array in which to search
 * Returns 1 if the filename exists
 * Returns 0 if the filename does not exist
 */
int fileIDIsOpen(int id, FileInfo* fileInfoIndex);


/* Checks if given file index is full
 * fileIfnoIndex: File index array to check
 * Returns -1 if full
 * Returns first empty slot if not full
 */
int fileIndexIsFull(FileInfo* fileInfoIndex);


/* Checks if given scan index is full
 * scansInfoIndex: Scan index array to check
 * Returns -1 if full
 * Returns first empty slot if not full
 */
int scansIndexIsFull(ScansInfo* scansInfoIndex);


/* Checks if given position of scan index is populated
 * scansInfoIndex: Scan index array to check
 * position: The position to check
 * Returns 1 if populated
 * Returns 0 else
 */
int scanDescIsOpen(ScansInfo* scansInfoIndex, int position);


/* Checks if given fileID exists in given scan index
 * @fileID: The fileID to search for
 * @scansInfoIndex: Scan index array to check
 * Returns 1 if fileID has at least one open scan
 * Returns 0 else
 */
int fileIDHasOpenScan(int fileID, ScansInfo* scansInfoIndex);

///* Checks a file info for a file ID and returns its real file descriptor
// * @fileID: The fileID to search for
// * @fileInfoIndex: File index array to search
// * Returns file system's file descriptor if fileID exists in the given file index
// * Returns -1 else
// */
//int fileIDtoFSFileDescriptor(int fileID, FileInfo* fileInfoIndex);


///* Converts a user given string operator to its equivalent integer operator
// * charOp: The user given char* operator
// * Returns compOperator enumerated type operator
// */
//compOperator charToOp(char* charOp);

/*
 * Finds the leaf node for a specific key, returns the data block pointer it should rest in
 */
int findLeafNode(unsigned int curHeight, char keyType, void* key, FileInfo curFile, int curBlock, int* path_array);

/*
 * Creates a new data block
 */
int createNewDataBlock(FileInfo curFile);

/*
 * Inserts some already sorted records to data blocks
 */
int insertSortedRec(int fileDesc, FileInfo curInfo, int num_of_recs, void* value1, void* value2, void* block);

/*
 * Inserts an already sorted index to tree
 */
int insertSortedIndex(int fileDesc, FileInfo curInfo, void* key, int block_ptr, void* block);

/*
 * Returns how many entries of the block are occupied (counter alike)
 */
int filledEntriesInBlock(void* block, int keyAttrLen, unsigned int pairsPerBlock);

/*
 * Splits a leaf or internal node of the tree
 */
int splitIndexBlock(int block_to_be_split, void* key_pair, int block_pair, FileInfo curInfo, int* new_block);

/*
 * Splits a data block to two separate
 */
int splitDataBlock(int block_to_be_split, void* value1, void* value2, void* upper_level_key, int* new_block_ptr, FileInfo curInfo);

/*
 * Updates tree related info
 */
int updateIndexInfo(int fileDesc, int height, int root);

/*
 * Increases the height of the tree
 */
int increaseTreeHeight(int fileDesc,FileInfo* curInfo, int block_to_be_split_ptr, void* key_to_be_inserted, int key_length,int* new_block_ptr);

/*
 * Returns the path from root to data block pointer for a given key
 */
PathList* findDataBlockAndPath(FileInfo curInfo, void* key);

/*
 * Does a linear search in data block to find a key
 */
int findKeyInDataBlock(void* myblock, void* key, char type, int numOfPairsInBlock, int al1, int al2);

/*
 * Checks if the next pair of the current one is also equal (if they have the same key)
 */
int isNextPairEqual(void* myblock, void* key, char type, int numOfPairsInBlock, int al1, int al2, int position);

/*
 * Returns the first entry in the whole tree
 */
PathList* mostlyLeftPath(FileInfo file);

/*
 * Checks if key is equal to pointer on block (bvalue)
 */
int isEqual(void* bvalue, void* key, char type, int length);

/*
 * Finds the position in data block of the first lesser key
 */
int isLessThan(void* blockValuePtr, void* key, char type, int length);

/*
 * Finds the position in data block of the first lesser or equal key
 */
int isLessThanOrEqual(void* blockValuePtr, void* key, char type, int length);

/*
 * Finds the position in data block of the first greater key
 */
int findFirstGreater(void* myblock, void* key, char type, int numOfPairsInBlock, int al1, int al2);

/*
 * Finds the position in data block of the first greater or equal key
 */
int findFirstGreaterOrEqual(void* myblock, void* key, char type, int numOfPairsInBlock, int al1, int al2);

/*
 * Prints all the contents of all the data blocks
 */
void print_data_blocks(FileInfo curInfo);

/*
 * Prints from the root down
 */
void print_root(int fileDesc, FileInfo curInfo);

#endif /* OPERATIONS_H_ */
