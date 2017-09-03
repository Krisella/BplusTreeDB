#ifndef DEFN_H_
#define DEFN_H_

#include "PathList.h"

#define INTEGER 'i'
#define FLOAT 'f'
#define STRING 'c'
#define INTSIZE sizeof(int)
#define PTRSIZE sizeof(int)
#define MAXOPENFILES 20
#define MAXSCANS 20

typedef struct type{
	char attrType1;
	char attrType2;
	int attrLength1;
	int attrLength2;
} RecordType;

// Open file index structure
typedef struct {
	// Real file descriptor
	int fd;
	// Real file name
	char* fn;
	// internal nodes per block
	unsigned int inpb;
	// leaf nodes per block
	unsigned int lnpb;
	// data nodes per block
	unsigned int dnpb;
	// Tree height
	unsigned int height;
	//current root
	unsigned int root;
	// Information for the type of records
	RecordType rec;
} FileInfo;

// First block info block structure
typedef struct {
	// BTree flag
	char mybtreestr[22];
	// Information for the type of records
	RecordType rec;
	// internal nodes per block
	unsigned int inpb;
	// leaf nodes per block
	unsigned int lnpb;
	// data nodes per block
	unsigned int dnpb;
	// Tree height
	unsigned int height;
	//current root
	unsigned int root;
} BTreeIndexInfo;

// Open scans index structure
typedef struct {
	// File descriptor - id
	int fileID;
	// Search operator
	int op;
	// Path to last entry to match
	PathList* lastPath;
	// Position within the data block (numbered)
	int dbposition;
	// The actual value the search will utilize
	// Can be casted to type later
	void* value;
	// Boolean, initialSearch
	int initial;
	// Boolean, IFF true shows that search has reached EOF
	int reof;
} ScansInfo;

#endif /* DEFN_H_ */
