#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#include "BF.h"
#include "Operations.h"

int AM_errno;

FileInfo openFiles[MAXOPENFILES];
ScansInfo openScans[MAXSCANS];

void AM_Init(void) {
	BF_Init();
	int counter;
	for (counter = 0; counter < MAXOPENFILES; ++counter) {
		openFiles[counter].fd = -1;
		openFiles[counter].fn = NULL;
		openFiles[counter].lnpb = 0;
		openFiles[counter].inpb = 0;
		openFiles[counter].dnpb = 0;
		openFiles[counter].height = 0;
		openFiles[counter].root=0;
		openFiles[counter].rec.attrLength1 = 0;
		openFiles[counter].rec.attrLength2 = 0;
		openFiles[counter].rec.attrType1 = 0;
		openFiles[counter].rec.attrType2 = 0;
	}
	for (counter = 0; counter < MAXSCANS; ++counter) {
		openScans[counter].fileID = -1;
		openScans[counter].op = 0;
		openScans[counter].reof = 0;
		openScans[counter].initial = 1;
		openScans[counter].lastPath = NULL;
		openScans[counter].dbposition = -1;
		openScans[counter].value = NULL;
	}

	return;
}

int AM_CreateIndex(char *fileName, char attrType1, int attrLength1,
		char attrType2, int attrLength2) {
	if ((attrType1 == INTEGER || attrType1 == FLOAT || attrType1 == STRING)
			&& (attrType2 == INTEGER || attrType2 == FLOAT
					|| attrType2 == STRING)) {

		// Check if the given values are correct
		if ((attrType1 == INTEGER || attrType1 == FLOAT)
				&& attrLength1 != INTSIZE) {
			return AME_WRONG_ATTRIBUTES_LENGTH;
		} else if (attrType1 == STRING
				&& (attrLength1 < 1 || attrLength1 > 255)) {
			return AME_WRONG_ATTRIBUTES_LENGTH;
		}
		if ((attrType2 == INTEGER || attrType2 == FLOAT)
				&& attrLength2 != INTSIZE) {
			return AME_WRONG_ATTRIBUTES_LENGTH;
		} else if (attrType2 == STRING
				&& (attrLength2 < 1 || attrLength2 > 255)) {
			return AME_WRONG_ATTRIBUTES_LENGTH;
		}

		// Check if the file already exists
		struct stat tempBuf;
		if (stat(fileName, &tempBuf) == 0) {
			return AME_FILE_ALREADY_EXIST;
		}

		// Tell BF level to create the file and check its return value
		int fileDesc = -1;
		if (BF_CreateFile(fileName) < 0) {
			AM_errno = 0;
			AM_PrintError("In AM_CreateIndex (BF_CreateFile)");
			return AME_FILE_NOT_CREATED;
		}

		if ((fileDesc = BF_OpenFile(fileName)) < 0) {
			AM_errno = AME_FILE_NOT_OPENED;
			AM_PrintError("In AM_CreateIndex (BF_OpenFile)");
			return AME_FILE_NOT_OPENED;
		}

		unsigned int myinpb = (BLOCK_SIZE - PTRSIZE) / (attrLength1 + PTRSIZE);
		unsigned int mylnpb = (BLOCK_SIZE - PTRSIZE) / (attrLength1 + PTRSIZE);
		unsigned int mydnpb = (BLOCK_SIZE - 2*sizeof(int)) / (attrLength1 + attrLength2);

		if (BF_AllocateBlock(fileDesc)) {
			AM_errno = 0;
			AM_PrintError("In AM_CreateIndex (BF_AllocateBlock)");
			if (BF_CloseFile(fileDesc)) {
				AM_errno = 0;
				AM_PrintError("In AM_CreateIndex (BF_CloseFile)");
			}
			return AME_COULD_NOT_ALLOCATE_BLOCK;
		}
		if (BF_GetBlockCounter(fileDesc) != 1) {
			AM_errno = 0;
			printf("Wrong initial allocated blocks!");
			if (BF_CloseFile(fileDesc)) {
				AM_PrintError(
						"In AM_CreateIndex (BF_CloseFile/BF_GetBlockCounter)");
				AM_errno = 0;
			}
			return AME_COULD_NOT_GET_COUNTER;
		}
		void* myblock = NULL;
		if (!BF_ReadBlock(fileDesc, 0, &myblock)) {
			BTreeIndexInfo curInfo;
			strcpy(curInfo.mybtreestr, "This is a BTree file!");
			curInfo.rec.attrLength1 = attrLength1;
			curInfo.rec.attrLength2 = attrLength2;
			curInfo.rec.attrType1 = attrType1;
			curInfo.rec.attrType2 = attrType2;
			curInfo.inpb = myinpb;
			curInfo.lnpb = mylnpb;
			curInfo.dnpb = mydnpb;
			curInfo.height = 0;
			curInfo.root=0;
			memcpy(myblock, &curInfo, sizeof(curInfo));
			if (BF_WriteBlock(fileDesc, 0)) {
				AM_errno = 0;
				printf("Error while writing the initial block on file!\n");
				AM_PrintError("In AM_CreateIndex (BF_WriteBlock)");
				return -1;
			}
		} else {
			AM_PrintError("In AM_CreateIndex (BF_ReadBlock)");
		}
		if (BF_CloseFile(fileDesc))
			AM_PrintError("In AM_CreateIndex (BF_CloseFile)");
	} else {
		return AME_WRONG_ATTRIBUTES_TYPE;
	}
	return AME_OK;
}

int AM_DestroyIndex(char *fileName) {
	if (!fileNameIsOpen(fileName, openFiles)) {
		if (remove(fileName)) {
			AM_errno = errno;
			AM_PrintError("In AM_DestroyIndex (Could not delete the file)");
			return AME_COULD_NOT_DELETE_FILE;
		}
		return AME_OK;
	} else {
		AM_PrintError("In AM_DestroyIndex (file is open)");
		return AME_OPEN_FILE_DESTROY;
	}
}

int AM_OpenIndex(char *fileName) {
	int i;
	int fd = -1;
	BTreeIndexInfo index_info;
	void* block;

	if ((i = fileIndexIsFull(openFiles)) == -1) {
		AM_errno = AME_FILE_INDEX_FULL;
		AM_PrintError("Max Files Open");
		return -1;
	}

	if ((fd = BF_OpenFile(fileName)) < 0) {
		AM_errno = 0;
		AM_PrintError("Opening File");
		return -1;
	}

	if (BF_ReadBlock(fd, 0, &block) < 0) {
		AM_errno = 0;
		AM_PrintError("Reading File");
		return -1;
	}
	memcpy(&index_info, block, sizeof(BTreeIndexInfo));
	openFiles[i].fd = fd;

	openFiles[i].inpb = index_info.inpb;
	openFiles[i].lnpb = index_info.lnpb;
	openFiles[i].dnpb = index_info.dnpb;
	openFiles[i].fn = fileName;
	openFiles[i].height = index_info.height;
	openFiles[i].root=index_info.root;
	openFiles[i].rec.attrLength1 = index_info.rec.attrLength1;
	openFiles[i].rec.attrLength2 = index_info.rec.attrLength2;
	openFiles[i].rec.attrType1 = index_info.rec.attrType1;
	openFiles[i].rec.attrType2 = index_info.rec.attrType2;
	return i;
}

int AM_CloseIndex(int fileDesc) {
	int k;

	for(k=0;k<MAXOPENFILES;k++)
		if(openFiles[k].fd==fileDesc)
			break;

	if (fileDesc < 0 || fileDesc > MAXOPENFILES) {
		AM_errno = AME_OUT_OF_BOUNDS_FD;
		AM_PrintError("Given file ID is not within the preset bounds.");
		return AME_OUT_OF_BOUNDS_FD;
	}
	if (!fileIDIsOpen(fileDesc, openFiles)) {
		AM_errno = AME_FILE_NOT_OPEN;
		AM_PrintError("File to be closed is not open.");
		return AME_FILE_NOT_OPEN;
	}
	if (fileIDHasOpenScan(fileDesc, openScans)) {
		AM_errno = AME_CLOSE_WITH_OPEN_SCAN;
		AM_PrintError("Cannot close a file while there is a pending scan on it.");
		return AME_CLOSE_WITH_OPEN_SCAN;
	}
	if (BF_CloseFile(openFiles[fileDesc].fd) < 0 ) {
		AM_errno = AME_CLOSE_FAILED;
		AM_PrintError("Trying to close file failed.");
		BF_PrintError("Close fail error:\n");
		return AME_CLOSE_FAILED;
	}
	openFiles[fileDesc].fd = -1;
	openFiles[fileDesc].fn = NULL;
	openFiles[fileDesc].lnpb = 0;
	openFiles[fileDesc].inpb = 0;
	openFiles[fileDesc].dnpb = 0;
	openFiles[fileDesc].height = 0;
	openFiles[fileDesc].rec.attrLength1 = 0;
	openFiles[fileDesc].rec.attrLength2 = 0;
	openFiles[fileDesc].rec.attrType1 = 0;
	openFiles[fileDesc].rec.attrType2 = 0;
	return AME_OK;
}

int AM_InsertEntry(int fileDesc, void *value1, void *value2) {
    // TODO Write AM_InsertEntry.

    int num_of_blocks, num_of_recs=0, data_block_ptr, i;
    int fd=openFiles[fileDesc].fd;
    void* key_to_be_inserted;
    int new_block_ptr, block_to_be_split_ptr=1;
    int *path_array;
    int next_data_block_ptr=-1;
    void* block;

    int key_length=openFiles[fileDesc].rec.attrLength1;
    int cur_height=openFiles[fileDesc].height;

    key_to_be_inserted=malloc(key_length);

    //print_root(fd,openFiles[fileDesc]);

    if((num_of_blocks=BF_GetBlockCounter(fd))<0){
        AM_errno=0;
        AM_PrintError("In AM_InsertEntry (GetBlockCounter)");
        return AME_COULD_NOT_GET_COUNTER;
    }

    //it's the first entry to be inserted
    if(num_of_blocks==1){
        createNewDataBlock(openFiles[fileDesc]);

        if(BF_ReadBlock(fd, 1, &block)<0){
            AM_errno=0;
            AM_PrintError("In AM_InsertEntry (ReadBlock)");
            return -1;
        }

        memcpy(block+sizeof(int)+openFiles[fileDesc].dnpb*(openFiles[fileDesc].rec.attrLength1+openFiles[fileDesc].rec.attrLength2),&next_data_block_ptr,sizeof(int));
        insertSortedRec(fd, openFiles[fileDesc], num_of_recs, value1, value2, block);

        if(BF_WriteBlock(fd,1)<0){
            AM_errno=0;
            AM_PrintError("In AM_InsertEntry (WriteBlock)");
            return -1;
        }
        return AME_OK;
    }

    //if there is no index then there is only one data block
    if(openFiles[fileDesc].height==0){

        if(BF_ReadBlock(fd,1,&block)<0){
            AM_errno=0;
            AM_PrintError("In AM_InsertEntry (ReadBlock)");
            return -1;
        }

        if(*(int*)(block) < openFiles[fileDesc].dnpb){
            insertSortedRec(fd, openFiles[fileDesc], *((int*)(block)), value1, value2, block);

            if(BF_WriteBlock(fd, 1)<0){
                AM_errno=0;
                AM_PrintError("In AM_InsertEntry (WriteBlock)");
                return -1;
            }
            return AME_OK;
        }else{
            // split data block and create the first index block

            splitDataBlock(block_to_be_split_ptr, value1, value2, key_to_be_inserted, &new_block_ptr, openFiles[fileDesc]);
            increaseTreeHeight(fd,&(openFiles[fileDesc]),block_to_be_split_ptr,key_to_be_inserted,key_length,&new_block_ptr);

            return AME_OK;
        }
    }

    path_array=malloc(cur_height*sizeof(int));
    data_block_ptr=findLeafNode(1,openFiles[fileDesc].rec.attrType1,value1,openFiles[fileDesc],openFiles[fileDesc].root,path_array);

    if(BF_ReadBlock(fd, data_block_ptr, &block)<0){
        AM_errno=0;
        AM_PrintError("In AM_InsertEntry (ReadBlock)");
        return -1;
    }

    if(*((int*)(block)) < openFiles[fileDesc].dnpb){
        insertSortedRec(fd, openFiles[fileDesc], *((int*)(block)), value1, value2, block);

        if(BF_WriteBlock(fd, data_block_ptr)<0){
            AM_errno=0;
            AM_PrintError("In AM_InsertEntry (WriteBlock)");
            return -1;
        }
        return AME_OK;
    }

    splitDataBlock(data_block_ptr, value1, value2, key_to_be_inserted, &new_block_ptr, openFiles[fileDesc]);
    int block_ptr;
    for(i=cur_height-1;i>=0;i--){

        block_ptr=new_block_ptr;
        if(BF_ReadBlock(fd, path_array[i], &block)<0){
            AM_errno=0;
            AM_PrintError("In AM_InsertEntry (ReadBlock)");
            return -1;
        }

        if(filledEntriesInBlock(block,key_length,openFiles[fileDesc].inpb) < openFiles[fileDesc].inpb){
            insertSortedIndex(fd, openFiles[fileDesc], key_to_be_inserted,block_ptr,block);

            if(BF_WriteBlock(fd,path_array[i])<0){
                AM_errno=0;
                AM_PrintError("In AM_InsertEntry (WriteBlock)");
                return -1;
            }
            return AME_OK;
        }else
            splitIndexBlock(path_array[i],key_to_be_inserted,block_ptr,openFiles[fileDesc],&new_block_ptr);
    }

    //create new root and increase tree height
    if(i==-1)
        increaseTreeHeight(fd,&(openFiles[fileDesc]),path_array[0],key_to_be_inserted,key_length,&new_block_ptr);

    return 0;
}


int AM_OpenIndexScan(int fileDesc, int op, void *value) {
	if (fileDesc < 0 || fileDesc > MAXOPENFILES) {
		AM_errno = AME_OUT_OF_BOUNDS_FD;
		AM_PrintError("Given file ID is not within the preset bounds.");
		return AME_OUT_OF_BOUNDS_FD;
	}
	if (!fileIDIsOpen(fileDesc, openFiles)) {
		AM_errno = AME_FILE_NOT_OPEN;
		AM_PrintError("File to be searched is not open.");
		return AME_FILE_NOT_OPEN;
	}
	if (op < 1 || op > 6) {
		AM_errno = AME_WRONG_OPERATOR;
		AM_PrintError("Such operator does not exist.");
		return AME_WRONG_OPERATOR;
	}
	int myScanIndex = -1;
	if ((myScanIndex = scansIndexIsFull(openScans)) == -1) {
		AM_errno = AME_SCAN_INDEX_FULL;
		AM_PrintError("Scan index if full.");
		return AME_SCAN_INDEX_FULL;
	}
	openScans[myScanIndex].fileID = fileDesc;
	openScans[myScanIndex].op = op;
	openScans[myScanIndex].value = value;
	openScans[myScanIndex].initial = 1;
	openScans[myScanIndex].reof = 0;
	openScans[myScanIndex].dbposition = -1;
	openScans[myScanIndex].lastPath = NULL;

//	print_data_blocks(openFiles[openScans[myScanIndex].fileID]);

	return myScanIndex;
}

void *AM_FindNextEntry(int scanDesc) {
	if (scanDesc < 0 || scanDesc > MAXSCANS) {
		AM_errno = AME_OUT_OF_BOUNDS_SD;
		AM_PrintError("Given scan ID is not within the preset bounds.");
		return NULL;
	}

	int fd = openFiles[openScans[scanDesc].fileID].fd;
	int fid = openScans[scanDesc].fileID;
	void* myblock = NULL;

	// Check if search has already reached EOF
	if (openScans[scanDesc].reof) {
		AM_errno = AME_EOF;
		return NULL;
	}



	if (openScans[scanDesc].op == EQUAL) {
		// For the first time of the search find the path to the possible data block of the key
		if (openScans[scanDesc].initial) {
			openScans[scanDesc].lastPath = findDataBlockAndPath(openFiles[fid], openScans[scanDesc].value);
			openScans[scanDesc].initial = 0;
			if (!BF_ReadBlock(fd, openScans[scanDesc].lastPath->lastNode->blockNo, &myblock)) {
				openScans[scanDesc].dbposition = findKeyInDataBlock(myblock, openScans[scanDesc].value, openFiles[fid].rec.attrType1,
						openFiles[fid].dnpb, openFiles[fid].rec.attrLength1,
						openFiles[fid].rec.attrLength2);
				if (openScans[scanDesc].dbposition == -1) {
					openScans[scanDesc].reof = 1;
					AM_errno = AME_EOF;
					return NULL;
				} else {
					return myblock + sizeof(int) + openScans[scanDesc].dbposition * (openFiles[fid].rec.attrLength1 +
							openFiles[fid].rec.attrLength2) + openFiles[fid].rec.attrLength1;
				}

			} else {
				AM_PrintError("In AM_FindNextEntry (BF_ReadBlock)");
				return NULL;
			}
		} else {
			// From then on just check the next pair and see if the key is the same with the searched one
			if (!BF_ReadBlock(fd, openScans[scanDesc].lastPath->lastNode->blockNo, &myblock)) {
				if (isNextPairEqual(myblock, openScans[scanDesc].value, openFiles[fid].rec.attrType1,
						openFiles[fid].dnpb, openFiles[fid].rec.attrLength1,
						openFiles[fid].rec.attrLength2, openScans[scanDesc].dbposition)) {
					++openScans[scanDesc].dbposition;
					return myblock + sizeof(int) + openScans[scanDesc].dbposition * (openFiles[fid].rec.attrLength1 +
							openFiles[fid].rec.attrLength2) + openFiles[fid].rec.attrLength1;
				} else {
					openScans[scanDesc].reof = 1;
					openScans[scanDesc].dbposition = -1;
					AM_errno = AME_EOF;
					return NULL;
				}
			} else {
				AM_PrintError("In AM_FindNextEntry (BF_ReadBlock)");
				return NULL;
			}
		}




	} else if (openScans[scanDesc].op == NOT_EQUAL) {
		if (openScans[scanDesc].initial) {
			openScans[scanDesc].initial = 0;
			openScans[scanDesc].lastPath = mostlyLeftPath(openFiles[fid]);
			openScans[scanDesc].dbposition = -1;
		}
		int foundNotEqual = 0;
		// Search from the very beginning of the data blocks till you find something not equal
		// Return it, next time resume from where you stopped
		while (foundNotEqual == 0) {
			if (!BF_ReadBlock(fd, openScans[scanDesc].lastPath->lastNode->blockNo, &myblock)) {
				(openScans[scanDesc].dbposition)++;
				while(openScans[scanDesc].dbposition < openFiles[fid].dnpb && foundNotEqual == 0 && openScans[scanDesc].dbposition < *(int*)myblock) {
					if (!isEqual((myblock + INTSIZE + openScans[scanDesc].dbposition * (openFiles[fid].rec.attrLength1 + openFiles[fid].rec.attrLength2)),
							openScans[scanDesc].value, openFiles[fid].rec.attrType1, openFiles[fid].rec.attrLength1)) {
						foundNotEqual = 1;
						(openScans[scanDesc].dbposition)--;
					}
					(openScans[scanDesc].dbposition)++;
				}
				if (foundNotEqual == 0) {
					int tempPointer = *(int*)(myblock + INTSIZE + openFiles[fid].dnpb * (openFiles[fid].rec.attrLength1 + openFiles[fid].rec.attrLength2));
					if (tempPointer == -1) {
						openScans[scanDesc].reof = 1;
						openScans[scanDesc].dbposition = -1;
						AM_errno = AME_EOF;
						return NULL;
					}
					removeLast(openScans[scanDesc].lastPath);
					addChildren(openScans[scanDesc].lastPath, tempPointer);
					openScans[scanDesc].dbposition = -1;
				}
			} else {
				AM_PrintError("In AM_FindNextEntry (BF_ReadBlock)");
				return NULL;
			}
		}
		if (foundNotEqual)
			return myblock + sizeof(int) + openScans[scanDesc].dbposition * (openFiles[fid].rec.attrLength1 +
									openFiles[fid].rec.attrLength2) + openFiles[fid].rec.attrLength1;
		else
			return NULL;



	} else if (openScans[scanDesc].op == LESS_THAN) {
		// First time see if the very first entry is lesser than the one searching for, if it is return EOF.
		if (openScans[scanDesc].initial) {
			openScans[scanDesc].initial = 0;
			openScans[scanDesc].lastPath = mostlyLeftPath(openFiles[fid]);
			openScans[scanDesc].dbposition = 0;
			if (!BF_ReadBlock(fd, openScans[scanDesc].lastPath->lastNode->blockNo, &myblock)) {
				if (!isLessThan(myblock + INTSIZE, openScans[scanDesc].value, openFiles[fid].rec.attrType1, openFiles[fid].rec.attrLength1)) {
					openScans[scanDesc].reof = 1;
					openScans[scanDesc].dbposition = -1;
					printf("EOF\n");
					AM_errno = AME_EOF;
					return NULL;
				}
			} else {
				AM_PrintError("In AM_FindNextEntry (BF_ReadBlock)");
				return NULL;
			}
			openScans[scanDesc].dbposition = -1;
		}

		// Return all the entries till you find one that is not lesser.
		if (!BF_ReadBlock(fd, openScans[scanDesc].lastPath->lastNode->blockNo, &myblock)) {
			++(openScans[scanDesc].dbposition);
			if (openScans[scanDesc].dbposition < *(int*)myblock && openScans[scanDesc].dbposition < openFiles[fid].dnpb) {
				if(!isLessThan(myblock + INTSIZE + openScans[scanDesc].dbposition * (openFiles[fid].rec.attrLength1 + openFiles[fid].rec.attrLength2),
						openScans[scanDesc].value, openFiles[fid].rec.attrType1, openFiles[fid].rec.attrLength1)) {
					openScans[scanDesc].reof = 1;
					openScans[scanDesc].dbposition = -1;
					AM_errno = AME_EOF;
					return NULL;
				}
			} else {
				// In case the current data block is consumed resume to the next one
				int tempPointer = *(int*)(myblock + INTSIZE + openFiles[fid].dnpb * (openFiles[fid].rec.attrLength1 + openFiles[fid].rec.attrLength2));
				if (tempPointer == -1) {
					openScans[scanDesc].reof = 1;
					openScans[scanDesc].dbposition = -1;
					AM_errno = AME_EOF;
					return NULL;
				}
				removeLast(openScans[scanDesc].lastPath);
				addChildren(openScans[scanDesc].lastPath, tempPointer);
				openScans[scanDesc].dbposition = 0;
				if (!BF_ReadBlock(fd, openScans[scanDesc].lastPath->lastNode->blockNo, &myblock)) {
					if(!isLessThan(myblock + INTSIZE + openScans[scanDesc].dbposition * (openFiles[fid].rec.attrLength1 + openFiles[fid].rec.attrLength2),
							openScans[scanDesc].value, openFiles[fid].rec.attrType1, openFiles[fid].rec.attrLength1)) {
						openScans[scanDesc].reof = 1;
						openScans[scanDesc].dbposition = -1;
						AM_errno = AME_EOF;
						return NULL;
					}
				} else {
					AM_PrintError("In AM_FindNextEntry (BF_ReadBlock)");
					return NULL;
				}
			}
		} else {
			AM_PrintError("In AM_FindNextEntry (BF_ReadBlock)");
			return NULL;
		}
	return myblock + sizeof(int) + openScans[scanDesc].dbposition * (openFiles[fid].rec.attrLength1 +
								openFiles[fid].rec.attrLength2) + openFiles[fid].rec.attrLength1;




	} else if (openScans[scanDesc].op == GREATER_THAN) {
		// Go down the tree to find the equal entries, go ahead them
		if (openScans[scanDesc].initial) {
			openScans[scanDesc].lastPath = findDataBlockAndPath(openFiles[fid],
					openScans[scanDesc].value);
			openScans[scanDesc].initial = 0;
			if (!BF_ReadBlock(fd, openScans[scanDesc].lastPath->lastNode->blockNo, &myblock)) {
				openScans[scanDesc].dbposition = findFirstGreater(myblock,
						openScans[scanDesc].value, openFiles[fid].rec.attrType1,
						openFiles[fid].dnpb, openFiles[fid].rec.attrLength1,
						openFiles[fid].rec.attrLength2);
				if (openScans[scanDesc].dbposition == -1) {
					int tempPointer = *(int*)(myblock + INTSIZE + openFiles[fid].dnpb * (openFiles[fid].rec.attrLength1 + openFiles[fid].rec.attrLength2));
					if (tempPointer == -1) {
						openScans[scanDesc].reof = 1;
						openScans[scanDesc].dbposition = -1;
						AM_errno = AME_EOF;
						return NULL;
					}
					removeLast(openScans[scanDesc].lastPath);
					addChildren(openScans[scanDesc].lastPath, tempPointer);
					openScans[scanDesc].dbposition = 0;
					if (!BF_ReadBlock(fd, openScans[scanDesc].lastPath->lastNode->blockNo, &myblock)) {
						openScans[scanDesc].dbposition = findFirstGreater(myblock,
												openScans[scanDesc].value, openFiles[fid].rec.attrType1,
												openFiles[fid].dnpb, openFiles[fid].rec.attrLength1,
												openFiles[fid].rec.attrLength2);
						if (openScans[scanDesc].dbposition == -1) {
							openScans[scanDesc].reof = 1;
							openScans[scanDesc].dbposition = -1;
							AM_errno = AME_EOF;
							return NULL;
						}
					} else {
						AM_PrintError("In AM_FindNextEntry (BF_ReadBlock)");
						return NULL;
					}
				}
			} else {
				AM_PrintError("In AM_FindNextEntry (BF_ReadBlock)");
				return NULL;
			}
			(openScans[scanDesc].dbposition)--;
		}

		// Take every entry till the end of data blocks.
		if (!BF_ReadBlock(fd, openScans[scanDesc].lastPath->lastNode->blockNo, &myblock)) {
			++openScans[scanDesc].dbposition;
			if (openScans[scanDesc].dbposition < openFiles[fid].dnpb && openScans[scanDesc].dbposition < *(int*)myblock) {
				return myblock + sizeof(int) + openScans[scanDesc].dbposition * (openFiles[fid].rec.attrLength1 + openFiles[fid].rec.attrLength2)
						+ openFiles[fid].rec.attrLength1;
			} else {
				int tempPointer = *(int*)(myblock + INTSIZE + openFiles[fid].dnpb * (openFiles[fid].rec.attrLength1 + openFiles[fid].rec.attrLength2));
				if (tempPointer == -1) {
					openScans[scanDesc].reof = 1;
					openScans[scanDesc].dbposition = -1;
					AM_errno = AME_EOF;
					return NULL;
				}
				removeLast(openScans[scanDesc].lastPath);
				addChildren(openScans[scanDesc].lastPath, tempPointer);
				openScans[scanDesc].dbposition = 0;
				if (!BF_ReadBlock(fd, openScans[scanDesc].lastPath->lastNode->blockNo, &myblock)) {
					return myblock + sizeof(int) + openScans[scanDesc].dbposition * (openFiles[fid].rec.attrLength1 + openFiles[fid].rec.attrLength2)
										+ openFiles[fid].rec.attrLength1;
				} else {
					AM_PrintError("In AM_FindNextEntry (BF_ReadBlock)");
					return NULL;
				}
			}
		} else {
			AM_PrintError("In AM_FindNextEntry (BF_ReadBlock)");
			return NULL;
		}




	} else if (openScans[scanDesc].op == LESS_THAN_OR_EQUAL) {
		// Just like less than but also return equals
		if (openScans[scanDesc].initial) {
			openScans[scanDesc].initial = 0;
			openScans[scanDesc].lastPath = mostlyLeftPath(openFiles[fid]);
			openScans[scanDesc].dbposition = 0;
			if (!BF_ReadBlock(fd, openScans[scanDesc].lastPath->lastNode->blockNo, &myblock)) {
				if (!isLessThanOrEqual(myblock + INTSIZE, openScans[scanDesc].value, openFiles[fid].rec.attrType1, openFiles[fid].rec.attrLength1)) {
					openScans[scanDesc].reof = 1;
					openScans[scanDesc].dbposition = -1;
					AM_errno = AME_EOF;
					return NULL;
				}
			} else {
				AM_PrintError("In AM_FindNextEntry (BF_ReadBlock)");
				return NULL;
			}
			openScans[scanDesc].dbposition = -1;
		}

		if (!BF_ReadBlock(fd, openScans[scanDesc].lastPath->lastNode->blockNo, &myblock)) {
			++(openScans[scanDesc].dbposition);
			if (openScans[scanDesc].dbposition < *(int*)myblock && openScans[scanDesc].dbposition < openFiles[fid].dnpb) {
				if(!isLessThanOrEqual(myblock + INTSIZE + openScans[scanDesc].dbposition * (openFiles[fid].rec.attrLength1 + openFiles[fid].rec.attrLength2),
						openScans[scanDesc].value, openFiles[fid].rec.attrType1, openFiles[fid].rec.attrLength1)) {
					openScans[scanDesc].reof = 1;
					openScans[scanDesc].dbposition = -1;
					AM_errno = AME_EOF;
					return NULL;
				}
			} else {
				int tempPointer = *(int*)(myblock + INTSIZE + openFiles[fid].dnpb * (openFiles[fid].rec.attrLength1 + openFiles[fid].rec.attrLength2));
				if (tempPointer == -1) {
					openScans[scanDesc].reof = 1;
					openScans[scanDesc].dbposition = -1;
					AM_errno = AME_EOF;
					return NULL;
				}
				removeLast(openScans[scanDesc].lastPath);
				addChildren(openScans[scanDesc].lastPath, tempPointer);
				openScans[scanDesc].dbposition = 0;
				if (!BF_ReadBlock(fd, openScans[scanDesc].lastPath->lastNode->blockNo, &myblock)) {
					if(!isLessThanOrEqual(myblock + INTSIZE + openScans[scanDesc].dbposition * (openFiles[fid].rec.attrLength1 + openFiles[fid].rec.attrLength2),
							openScans[scanDesc].value, openFiles[fid].rec.attrType1, openFiles[fid].rec.attrLength1)) {
						openScans[scanDesc].reof = 1;
						openScans[scanDesc].dbposition = -1;
						AM_errno = AME_EOF;
						return NULL;
					}
				} else {
					AM_PrintError("In AM_FindNextEntry (BF_ReadBlock)");
					return NULL;
				}
			}
		} else {
			AM_PrintError("In AM_FindNextEntry (BF_ReadBlock)");
			return NULL;
		}
	return myblock + sizeof(int) + openScans[scanDesc].dbposition * (openFiles[fid].rec.attrLength1 +
								openFiles[fid].rec.attrLength2) + openFiles[fid].rec.attrLength1;





	} else if (openScans[scanDesc].op == GREATER_THAN_OR_EQUAL) {
		//Just like greater than but also returns equals
		if (openScans[scanDesc].initial) {
			openScans[scanDesc].lastPath = findDataBlockAndPath(openFiles[fid],
					openScans[scanDesc].value);
			openScans[scanDesc].initial = 0;
			if (!BF_ReadBlock(fd, openScans[scanDesc].lastPath->lastNode->blockNo, &myblock)) {
				openScans[scanDesc].dbposition = findFirstGreaterOrEqual(myblock,
						openScans[scanDesc].value, openFiles[fid].rec.attrType1,
						openFiles[fid].dnpb, openFiles[fid].rec.attrLength1,
						openFiles[fid].rec.attrLength2);
				if (openScans[scanDesc].dbposition == -1) {
					int tempPointer = *(int*)(myblock + INTSIZE + openFiles[fid].dnpb * (openFiles[fid].rec.attrLength1 + openFiles[fid].rec.attrLength2));
					if (tempPointer == -1) {
						openScans[scanDesc].reof = 1;
						openScans[scanDesc].dbposition = -1;
						AM_errno = AME_EOF;
						return NULL;
					}
					removeLast(openScans[scanDesc].lastPath);
					addChildren(openScans[scanDesc].lastPath, tempPointer);
					openScans[scanDesc].dbposition = 0;
					if (!BF_ReadBlock(fd, openScans[scanDesc].lastPath->lastNode->blockNo, &myblock)) {
						openScans[scanDesc].dbposition = findFirstGreaterOrEqual(myblock,
												openScans[scanDesc].value, openFiles[fid].rec.attrType1,
												openFiles[fid].dnpb, openFiles[fid].rec.attrLength1,
												openFiles[fid].rec.attrLength2);
						if (openScans[scanDesc].dbposition == -1) {
							openScans[scanDesc].reof = 1;
							openScans[scanDesc].dbposition = -1;
							AM_errno = AME_EOF;
							return NULL;
						}
					} else {
						AM_PrintError("In AM_FindNextEntry (BF_ReadBlock)");
						return NULL;
					}
				}
			} else {
				AM_PrintError("In AM_FindNextEntry (BF_ReadBlock)");
				return NULL;
			}
			(openScans[scanDesc].dbposition)--;
		}

		if (!BF_ReadBlock(fd, openScans[scanDesc].lastPath->lastNode->blockNo, &myblock)) {
			++openScans[scanDesc].dbposition;
			if (openScans[scanDesc].dbposition < openFiles[fid].dnpb && openScans[scanDesc].dbposition < *(int*)myblock) {
				return myblock + sizeof(int) + openScans[scanDesc].dbposition * (openFiles[fid].rec.attrLength1 + openFiles[fid].rec.attrLength2)
						+ openFiles[fid].rec.attrLength1;
			} else {
				int tempPointer = *(int*)(myblock + INTSIZE + openFiles[fid].dnpb * (openFiles[fid].rec.attrLength1 + openFiles[fid].rec.attrLength2));
				if (tempPointer == -1) {
					openScans[scanDesc].reof = 1;
					openScans[scanDesc].dbposition = -1;
					AM_errno = AME_EOF;
					return NULL;
				}
				removeLast(openScans[scanDesc].lastPath);
				addChildren(openScans[scanDesc].lastPath, tempPointer);
				openScans[scanDesc].dbposition = 0;
				if (!BF_ReadBlock(fd, openScans[scanDesc].lastPath->lastNode->blockNo, &myblock)) {
					return myblock + sizeof(int) + openScans[scanDesc].dbposition * (openFiles[fid].rec.attrLength1 + openFiles[fid].rec.attrLength2)
										+ openFiles[fid].rec.attrLength1;
				} else {
					AM_PrintError("In AM_FindNextEntry (BF_ReadBlock)");
					return NULL;
				}
			}
		} else {
			AM_PrintError("In AM_FindNextEntry (BF_ReadBlock)");
			return NULL;
		}
	}


	return NULL;
}

int AM_CloseIndexScan(int scanDesc) {
	if (scanDescIsOpen(openScans, scanDesc)) {
		openScans[scanDesc].fileID = -1;
		openScans[scanDesc].op = 0;
		openScans[scanDesc].reof = 0;
		openScans[scanDesc].initial = 1;
		if (openScans[scanDesc].lastPath != NULL)
			destroyPathList(openScans[scanDesc].lastPath);
		openScans[scanDesc].dbposition = -1;
		openScans[scanDesc].value = NULL;
		return AME_OK;
	}
	else {
		AM_errno = AME_SCAN_ALREADY_CLOSED;
		AM_PrintError("Tried to close already closed scan.");
		return AME_SCAN_ALREADY_CLOSED;
	}

}

void AM_PrintError(char *errString) {
	printf("Error: %d\n%s\n", AM_errno, errString);
	if(AM_errno==0)
		BF_PrintError(errString);
	return;
}
