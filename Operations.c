#include "Operations.h"
#include "BF.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int fileNameIsOpen(char* fileName, FileInfo* fileInfoIndex) {
	int counter;
	for (counter = 0; counter < MAXOPENFILES; ++counter) {
		if (fileInfoIndex[counter].fd!=-1 && !strcmp(fileInfoIndex[counter].fn, fileName))
			return 1;
	}
	return 0;
}

int fileIDIsOpen(int id, FileInfo* fileInfoIndex) {
	if (id >= MAXOPENFILES)
		return 0;
	int counter;
	for (counter = 0; counter < MAXOPENFILES; ++counter) {
		if (fileInfoIndex[counter].fd == id)
			return 1;
	}
	return 0;
}

int fileIndexIsFull(FileInfo* fileInfoIndex) {
	int counter;
	for (counter = 0; counter < MAXOPENFILES; ++counter) {
		if (fileInfoIndex[counter].fd == -1) {
			return counter;
		}
	}
	return -1;
}

int scansIndexIsFull(ScansInfo* scansInfoIndex) {
	int counter;
	for (counter = 0; counter < MAXSCANS; ++counter) {
		if (scansInfoIndex[counter].fileID == -1) {
			return counter;
		}
	}
	return -1;
}

int scanDescIsOpen(ScansInfo* scansInfoIndex, int position) {
	if (scansInfoIndex[position].fileID != -1)
		return 1;
	return 0;
}

int fileIDHasOpenScan(int fileID, ScansInfo* scansInfoIndex) {
	int counter;
	for (counter = 0; counter < MAXSCANS; ++counter) {
		if (scansInfoIndex[counter].fileID == fileID) {
			return 1;
		}
	}
	return 0;
}

int findLeafNode(unsigned int curHeight, char keyType, void* key, FileInfo curFile, int curBlock, int* path_array) {
    if (curBlock < 1 || curHeight < 0 || (keyType != INTEGER && keyType != FLOAT && keyType != STRING))
        return -1;
    void* myblock = NULL;
    unsigned int pairSize = curFile.rec.attrLength1 + PTRSIZE;
    int counter;

    path_array[curHeight-1]=curBlock;

    // While in internal nodes
    if (curHeight < curFile.height) {
        if (!BF_ReadBlock(curFile.fd, curBlock, &myblock)) {
            if (keyType == INTEGER) {
                for (counter = 0; counter < curFile.inpb; ++counter) {
                    // If pointer before the entry we will check is empty, we return the last non empty = the one before this.
                    if (*((int*)(myblock + (counter*pairSize))) == -1) {
                        return findLeafNode(curHeight + 1, keyType, key, curFile, *((int*)(myblock + ((counter-1)*pairSize))),path_array);
                    }
                    // Else we check if lesser and return the optimal node
                    if (*(int*)key <= *((int*)(myblock + ((counter*pairSize))+PTRSIZE))) {
                        if (*(int*)key == *((int*)(myblock + ((counter*pairSize))+PTRSIZE))) {
                            return findLeafNode(curHeight + 1, keyType, key, curFile, *((int*)(myblock + ((counter+1)*pairSize))),path_array);
                        } else {
                            return findLeafNode(curHeight + 1, keyType, key, curFile, *((int*)(myblock + (counter*pairSize))),path_array);
                        }
                    }
                }
                // If nothing was found to match V<+Ki then we return the very last pointer in the block
                printf("mparmpoutsala\n");
                return findLeafNode(curHeight + 1, keyType, key, curFile, *((int*)(myblock + (curFile.inpb*pairSize))),path_array);
            } else if(keyType == FLOAT) {
                for (counter = 0; counter < curFile.inpb; ++counter) {
                    // If pointer before the entry we will check is empty, we return the last non empty = the one before this.
                    if (*((int*)(myblock + (counter*pairSize))) == -1) {
                        return findLeafNode(curHeight + 1, keyType, key, curFile, *((int*)(myblock + ((counter-1)*pairSize))),path_array);
                    }
                    if (*(float*)key <= *((float*)(myblock + ((counter*pairSize))+PTRSIZE))) {
                        if (*(float*)key == *((float*)(myblock + ((counter*pairSize))+PTRSIZE))) {
                            return findLeafNode(curHeight + 1, keyType, key, curFile, *((int*)(myblock + ((counter+1)*pairSize))),path_array);
                        } else {
                            return findLeafNode(curHeight + 1, keyType, key, curFile, *((int*)(myblock + (counter*pairSize))),path_array);
                        }
                    }
                }
                // If nothing was found to match V<+Ki then we return the very last pointer in the block
                printf("mparmpoutsala\n");
                return findLeafNode(curHeight + 1, keyType, key, curFile, *((int*)(myblock + (curFile.inpb*pairSize))),path_array);
            } else {
                for (counter = 0; counter < curFile.inpb; ++counter) {
                    // If pointer before the entry we will check is empty, we return the last non empty = the one before this.
                    if (*((int*)(myblock + (counter*pairSize))) == -1) {
                        return findLeafNode(curHeight + 1, keyType, key, curFile, *((int*)(myblock + ((counter-1)*pairSize))),path_array);
                    }
                    if (strcmp((char*)key , ((char*)(myblock + ((counter*pairSize))+PTRSIZE))) <= 0) {
                    	if (strcmp((char*)key , ((char*)(myblock + ((counter*pairSize))+PTRSIZE))) == 0) {
                            return findLeafNode(curHeight + 1, keyType, key, curFile, *((int*)(myblock + ((counter+1)*pairSize))),path_array);
                        } else {
                            return findLeafNode(curHeight + 1, keyType, key, curFile, *((int*)(myblock + (counter*pairSize))),path_array);
                        }
                    }
                }
                // If nothing was found to match V<+Ki then we return the very last pointer in the block
                printf("mparmpoutsala\n");
                return findLeafNode(curHeight + 1, keyType, key, curFile, *((int*)(myblock + (curFile.inpb*pairSize))),path_array);
            }
        } else {
            AM_PrintError("In findLeafNode (BF_ReadBlock / Internal nodes)");
        }
    } else {
    	// If at a leaf node
        if (!BF_ReadBlock(curFile.fd, curBlock, &myblock)) {
        	// Return the data block pointer exactly after the best matching key
            int filledEntries = filledEntriesInBlock(myblock, curFile.rec.attrLength1, curFile.lnpb);
            int* dataBlockPointer=NULL;
            if (keyType == INTEGER) {
                for (counter = 0; counter < curFile.lnpb && counter < filledEntries; ++counter) {
                    if (*((int*)((myblock + PTRSIZE) + (counter*pairSize))) > *(int*)key) {
                        dataBlockPointer = (int*)(myblock + (counter*pairSize));
                        break;
                    }
                }
            } else if(keyType == FLOAT) {
                for (counter = 0; counter < curFile.lnpb && counter < filledEntries; ++counter) {
                    if (*((float*)((myblock + PTRSIZE) + (counter*pairSize))) > *(float*)key) {
                        dataBlockPointer = (int*)(myblock + (counter*pairSize));
                        break;
                    }
                }
            } else {
                for (counter = 0; counter < curFile.lnpb && counter < filledEntries; ++counter) {
                    if (strcmp(((char*)((myblock + PTRSIZE) + (counter*pairSize))), ((char*)key)) > 0) {
                        dataBlockPointer = (int*)(myblock + (counter*pairSize));
                        break;
                    }
                }
            }
            if(counter==filledEntries)
                dataBlockPointer=(int*)(myblock+filledEntries*pairSize);

            return *dataBlockPointer;
        } else {
            AM_PrintError("In findLeafNode (BF_ReadBlock / Leaf nodes)");
        }
    }
    return -1;
}

int createNewDataBlock(FileInfo curFile) {

    if (BF_AllocateBlock(curFile.fd)<0) {
        AM_errno = AME_COULD_NOT_ALLOCATE_BLOCK;
        AM_PrintError("In createNewDataBlock (BF_AllocateBlock)");
        if (BF_CloseFile(curFile.fd)) {
            AM_errno = 0;
            AM_PrintError("In createNewDataBlock (BF_CloseFile)");
        }
        return AME_COULD_NOT_ALLOCATE_BLOCK;
    }
    int newBlock = -1;
    if ((newBlock = BF_GetBlockCounter(curFile.fd)) < 0) {
        AM_errno = AME_COULD_NOT_GET_COUNTER;
        AM_PrintError("In createNewDataBlock (BF_GetBlockCounter)");
        if (BF_CloseFile(curFile.fd)) {
            AM_PrintError("In createNewDataBlock (BF_CloseFile/BF_GetBlockCounter)");
            AM_errno = 0;
        }
        return AME_COULD_NOT_GET_COUNTER;
    }

    return newBlock;
}

int insertSortedRec(int fileDesc, FileInfo curInfo, int num_of_recs, void* value1, void* value2, void* block){
    int i,flag=0;
    int pair_size=curInfo.rec.attrLength1 + curInfo.rec.attrLength2;

    if(num_of_recs==0){
        memcpy(block+sizeof(int),value1,curInfo.rec.attrLength1);
        memcpy(block+curInfo.rec.attrLength1+sizeof(int),value2,curInfo.rec.attrLength2);
        num_of_recs++;
        memcpy(block,&num_of_recs,sizeof(int));
        return 0;
    }

    if(curInfo.rec.attrType1=='i'){
        for(i=0;i<num_of_recs;i++){
            int cur_rec;
            memcpy(&cur_rec,block+sizeof(int)+i*pair_size,sizeof(int));
            if(cur_rec>*(int*)value1){
                memmove(block+sizeof(int)+(i+1)*pair_size,block+sizeof(int)+i*pair_size,pair_size*(num_of_recs-i));
                memcpy(block+sizeof(int)+i*pair_size, value1, curInfo.rec.attrLength1);
                memcpy(block+sizeof(int)+i*pair_size+curInfo.rec.attrLength1, value2, curInfo.rec.attrLength2);
                flag=1;
                break;
            }
        }
    }else if(curInfo.rec.attrType1=='f'){
        for(i=0;i<num_of_recs;i++){
            float cur_rec;
            memcpy(&cur_rec,block+sizeof(int)+i*pair_size,sizeof(float));
            if(cur_rec>*(float*)value1){
                memmove(block+sizeof(int)+(i+1)*pair_size,block+sizeof(int)+i*pair_size,pair_size*(num_of_recs-i));
                memcpy(block+sizeof(int)+i*pair_size, value1, curInfo.rec.attrLength1);
                memcpy(block+sizeof(int)+i*pair_size+curInfo.rec.attrLength1, value2, curInfo.rec.attrLength2);
                flag=1;
                break;
            }
        }
    }else if(curInfo.rec.attrType1=='c'){
        for(i=0;i<num_of_recs;i++){
            char cur_rec[curInfo.rec.attrLength1];
            memcpy(cur_rec,block+sizeof(int)+i*pair_size,sizeof(char)*curInfo.rec.attrLength1);
            if(strcmp(cur_rec,(char*)value1)>0){
                memmove(block+sizeof(int)+(i+1)*pair_size,block+sizeof(int)+i*pair_size,pair_size*(num_of_recs-i));
                memcpy(block+sizeof(int)+i*pair_size, value1, curInfo.rec.attrLength1);
                memcpy(block+sizeof(int)+i*pair_size+curInfo.rec.attrLength1, value2, curInfo.rec.attrLength2);
                flag=1;
                break;
            }
        }
    }
    if(flag==0){
        memcpy(block+sizeof(int)+num_of_recs*pair_size,value1,curInfo.rec.attrLength1);
        memcpy(block+sizeof(int)+num_of_recs*pair_size+curInfo.rec.attrLength1,value2,curInfo.rec.attrLength2);
    }
    num_of_recs++;
    memcpy(block,&num_of_recs,sizeof(int));

    return 0;
}

int insertSortedIndex(int fileDesc, FileInfo curInfo, void* key, int block_ptr, void* block){

    int i, end_of_block=-1, flag=0;
    unsigned int pair_size=curInfo.rec.attrLength1 + PTRSIZE;
    int key_length=curInfo.rec.attrLength1;
    void* cur_key;

    int pairs_in_block=filledEntriesInBlock(block,key_length,curInfo.inpb);
    cur_key=malloc(key_length);

    for(i=0;i<pairs_in_block;i++){

        memcpy(cur_key,block+i*pair_size+PTRSIZE,key_length);

    /*  if(*(int*)(block+(i+1)*pair_size)==-1){
            memcpy(block+i*pair_size+PTRSIZE,key,key_length);
            memcpy(block+(i+1)*pair_size,&block_ptr,PTRSIZE);

            break;
        }*/

        if((curInfo.rec.attrType1==INTEGER && *(int*)cur_key>*(int*)key)
            || (curInfo.rec.attrType1==FLOAT && *(float*)cur_key>*(float*)key)
            || (curInfo.rec.attrType1==STRING && strcmp((char*)cur_key,(char*)key)>0)){

            memmove(block+(i+1)*pair_size+PTRSIZE,block+i*pair_size+PTRSIZE,(pairs_in_block-i)*pair_size);
            memcpy(block+i*pair_size+PTRSIZE,key,key_length);
            memcpy(block+(i+1)*pair_size,&block_ptr,PTRSIZE);
            flag=1;
            break;
        }
    }
    if(flag==0){
        memcpy(block+pairs_in_block*pair_size+PTRSIZE,key,key_length);
        memcpy(block+pairs_in_block*pair_size+PTRSIZE+key_length,&block_ptr,PTRSIZE);
    }
    if(pairs_in_block+1 != curInfo.inpb)
        memcpy(block+(pairs_in_block+2)*pair_size,&end_of_block,sizeof(int));

//    if(curInfo.rec.attrType1==STRING){
//        printf("%d, ",*(int*)(block));
//        for(i=0;i<pairs_in_block+1;i++){
//            printf(" %s, ",(char*)(block+i*pair_size+PTRSIZE));
//            printf(" %d, ",*(int*)(block+(i+1)*pair_size));
//        }
//        printf(" %d \n",*(int*)(block+(pairs_in_block+2)*pair_size));
//    }

    free(cur_key);
    return 0;
}

int filledEntriesInBlock(void* block, int keyAttrLen, unsigned int pairsPerBlock) {
    int counter;
    for (counter = 1; counter <= pairsPerBlock; ++counter) {
        if (*((int*)(block + (counter * (keyAttrLen + PTRSIZE)))) == -1) {
            return counter-1;
        }
    }
    return pairsPerBlock;
}

int splitIndexBlock(int block_to_be_split, void* key_pair, int block_pair, FileInfo curInfo, int* new_block){

	void* block;
	int block_ptr_arr[curInfo.inpb+2], i,j=0,flag=0,new_block_ptr, end_of_block=-1;
	int pair_size=PTRSIZE + curInfo.rec.attrLength1;
	int key_length=curInfo.rec.attrLength1;
	int fileDesc=curInfo.fd;
	void* key_arr, *cur_key;

	int num_of_pointers_to_first_block=(int)((curInfo.inpb+2)/2.0 + 0.5);
	int num_of_pointers_to_second_block=(curInfo.inpb+2)-num_of_pointers_to_first_block;

	if(BF_AllocateBlock(fileDesc)<0){
		AM_errno=0;
		AM_PrintError("In splitIndexBlock (AllocateBlock)");
		return AME_COULD_NOT_ALLOCATE_BLOCK;
	}
	if((new_block_ptr=((BF_GetBlockCounter(fileDesc))-1))<0){
		AM_errno=0;
		AM_PrintError("In splitIndexBlock (GetBlockCounter)");
		return AME_COULD_NOT_GET_COUNTER;
	}
	if(BF_ReadBlock(fileDesc, block_to_be_split, &block)<0){
		AM_errno=0;
		AM_PrintError("In splitIndexBlock (ReadBlock)");
		return -1;
	}

	//int filledEntries=filledEntriesInBlock(block,key_length,curInfo.inpb);

	//storing sorted keys and pointers to arrays

	block_ptr_arr[0]=*(int*)(block);
	key_arr=malloc((curInfo.inpb+1)*key_length);
	cur_key=malloc(key_length);

	for(i=0; i<curInfo.inpb; i++){

		memcpy(cur_key,block+i*pair_size+PTRSIZE,key_length);
		if((curInfo.rec.attrType1==INTEGER && flag==0 && (*(int*)cur_key) > (*(int*)key_pair))
			||(curInfo.rec.attrType1==FLOAT && flag==0 && (*(float*)cur_key) > (*(float*)key_pair))
			||(curInfo.rec.attrType1==STRING && flag==0 && strcmp((char*)cur_key,(char*)key_pair)>0)){

			memcpy(key_arr+j*key_length,key_pair,key_length);
			block_ptr_arr[j+1]=block_pair;
			j++;
			flag=1;
		}

		memcpy(key_arr+j*key_length,block+i*pair_size+PTRSIZE,key_length);
		block_ptr_arr[j+1]=(*(int*)(block+i*pair_size+PTRSIZE+key_length));
		j++;
	}

	if(flag==0){
		memcpy(key_arr+j*key_length,key_pair,key_length);
		block_ptr_arr[j+1]=block_pair;
		j++;
	}
	//storing the upper ceiling half number of pointers and half number of keys from the sorted arrays
	//to the old and new index blocks respectively
	memcpy(block,&block_ptr_arr[0],PTRSIZE);
	for(i=0;i<num_of_pointers_to_first_block-1;i++){
		memcpy(block+i*pair_size+PTRSIZE,key_arr+i*key_length,key_length);
		memcpy(block+i*pair_size+PTRSIZE+key_length,&(block_ptr_arr[i+1]),sizeof(int));
//		printf("this is crucial: %d \n",*(int*)(block+i*pair_size+PTRSIZE+key_length));
	}

	memcpy(block+(i+1)*pair_size,&end_of_block,sizeof(int));


//	if(curInfo.rec.attrType1==STRING){
//		printf("\n printing split index \n");
//		printf("%d, ",*(int*)(block));
//		for(k=0;k<num_of_pointers_to_first_block-1;k++){
//			printf(" %s, ",(char*)(block+k*pair_size+PTRSIZE));
//			printf(" %d, ",(*(int*)(block+(k+1)*pair_size)));
//		}
//		printf(" %d \n",*(int*)(block+(k+1)*pair_size));
//		printf("\n");
//	}

	if(BF_WriteBlock(fileDesc, block_to_be_split)<0){
		AM_errno=0;
		AM_PrintError("In splitIndexBlock (WriteBlock)");
		return -1;
	}

	//storing pair to be added on the upper level
	memcpy(key_pair,key_arr+i*key_length,key_length);
	i++;
	*new_block=new_block_ptr;

	if(BF_ReadBlock(fileDesc,new_block_ptr,&block)<0){
		AM_errno=0;
		AM_PrintError("In splitIndexBlock (ReadBlock)");
		return -1;
	}

	memcpy(block,&(block_ptr_arr[i]),PTRSIZE);
	j=i;
	for(i=0;i<num_of_pointers_to_second_block-1;i++){
		memcpy(block+i*pair_size+PTRSIZE,key_arr+j*key_length,key_length);
		memcpy(block+i*pair_size+PTRSIZE+key_length,&(block_ptr_arr[j+1]),sizeof(int));
		j++;
	}

	memcpy(block+(i+1)*pair_size,&end_of_block,sizeof(int));

	if(BF_WriteBlock(fileDesc, new_block_ptr)<0){
		AM_errno=0;
		AM_PrintError("In splitIndexBlock (WriteBlock)");
		return -1;
	}

	free(key_arr);
	free(cur_key);
	return 0;
}

int splitDataBlock(int block_to_be_split, void* value1, void* value2, void* upper_level_key, int* new_block_ptr, FileInfo curInfo){

	void* block1, *block2;
	int fileDesc=curInfo.fd, num_of_recs, num_of_recs2, i, flag=0, j=0;
	int key_length=curInfo.rec.attrLength1;
	int attrLength2=curInfo.rec.attrLength2;
	int pair_size=curInfo.rec.attrLength1+curInfo.rec.attrLength2;
	int next_data_block_ptr;
	void* arr,*cur_rec;

	(*new_block_ptr)=createNewDataBlock(curInfo)-1;

//	if(*(new_block_ptr)==20)
//		printf("bla \n");

	if(BF_ReadBlock(fileDesc, block_to_be_split, &block1)<0){
		AM_errno=0;
		AM_PrintError("In splitDataBlock (ReadBlock)");
		return -1;
	}

	memcpy(&num_of_recs,block1,sizeof(int));
	arr=malloc(pair_size*(num_of_recs+1));
	cur_rec=malloc(key_length);

	for(i=0;i<num_of_recs;i++){

		memcpy(cur_rec,block1+i*pair_size+sizeof(int),key_length);

		if((curInfo.rec.attrType1==INTEGER && flag==0 && (*(int*)cur_rec) > (*(int*)value1))
			||  (curInfo.rec.attrType1==FLOAT && flag==0 && (*(float*)cur_rec) > (*(float*)value1) )
			|| (curInfo.rec.attrType1==STRING && flag==0 && strcmp(cur_rec,(char*)value1)>0)){

			memcpy(arr+j*pair_size,value1,key_length);
			memcpy(arr+j*pair_size+key_length,value2,attrLength2);
			j++;
			flag=1;
		}
		memcpy(arr+j*pair_size,block1+i*pair_size+sizeof(int),key_length);
		memcpy(arr+j*pair_size+key_length,block1+i*pair_size+key_length+sizeof(int),attrLength2);
		j++;
	}

	if(flag==0){
		memcpy(arr+j*pair_size,value1,key_length);
		memcpy(arr+j*pair_size+key_length,value2,attrLength2);
		j++;
	}
	num_of_recs2=(num_of_recs+1)/2;
	num_of_recs=(int)((num_of_recs+1)/2.0 + 0.5);

	memcpy(&next_data_block_ptr,block1+sizeof(int)+curInfo.dnpb*(pair_size),sizeof(int));
	memcpy(block1+sizeof(int)+curInfo.dnpb*(pair_size),new_block_ptr,sizeof(int));
	memcpy(block1,&num_of_recs,sizeof(int));
	memcpy(block1+sizeof(int),arr,num_of_recs*pair_size);

	if(BF_WriteBlock(fileDesc,block_to_be_split)<0){
		AM_errno=0;
		AM_PrintError("In splitDataBlock (BF_WriteBlock)");
		return -1;
	}

	if(BF_ReadBlock(fileDesc,*new_block_ptr,&block2)){
		AM_errno=0;
		AM_PrintError("In splitDataBlock (ReadBlock)");
		return -1;
	}

	memcpy(block2,&num_of_recs2,sizeof(int));
	memcpy(block2+curInfo.dnpb*pair_size+sizeof(int),&next_data_block_ptr,sizeof(int));
	memcpy(block2+sizeof(int),arr+num_of_recs*pair_size,pair_size*num_of_recs2);
	memcpy(upper_level_key,block2+sizeof(int),key_length);

	if(!BF_WriteBlock(fileDesc,*new_block_ptr)<0){
		AM_errno=0;
		AM_PrintError("In splitDataBlock (WriteBlock)");
		return -1;
	}

	free(arr);
	free(cur_rec);
	return 0;
}

int updateIndexInfo(int fileDesc, int height, int root){
    BTreeIndexInfo info;
    void* block;

    if(BF_ReadBlock(fileDesc,0,&block)<0){
        AM_errno=0;
        AM_PrintError("In updateIndexInfo (ReadBlock)");
        return -1;
    }

    memcpy(&info,block,sizeof(BTreeIndexInfo));
    info.height=height;
    info.root=root;
    memcpy(block,&info,sizeof(BTreeIndexInfo));

    if(BF_WriteBlock(fileDesc,0)<0){
        AM_errno=0;
        AM_PrintError("In updateIndexInfo (WriteBlock)");
        return -1;
    }

    return 0;
}

int increaseTreeHeight(int fileDesc,FileInfo* curInfo, int block_to_be_split_ptr, void* key_to_be_inserted, int key_length,int* new_block_ptr){
    void* block;
    int index_block_ptr, end_of_index_block=-1;

    unsigned int cur_height=curInfo->height;

    if(BF_AllocateBlock(fileDesc)<0){
        AM_errno=0;
        AM_PrintError("In AM_InsertEntry (AllocateBlock)");
        return AME_COULD_NOT_ALLOCATE_BLOCK;
    }

    if((index_block_ptr=(BF_GetBlockCounter(fileDesc)-1))<0){
        AM_errno=0;
        AM_PrintError("In AM_InsertEntry (GetBlockCounter)");
        return AME_COULD_NOT_GET_COUNTER;
    }

    if(BF_ReadBlock(fileDesc,index_block_ptr,&block)<0){
        AM_errno=0;
        AM_PrintError("In AM_InsertEntry (ReadBlock)");
        return -1;
    }
    memcpy(block,&block_to_be_split_ptr,PTRSIZE);
    memcpy(block+PTRSIZE,key_to_be_inserted,key_length);
    memcpy(block+PTRSIZE+key_length,new_block_ptr,PTRSIZE);
    if(curInfo->inpb>1)
        memcpy(block+2*(key_length+PTRSIZE),&end_of_index_block,sizeof(int));

    if(curInfo->rec.attrType1==STRING){
//        printf("Printing Root: \n");
//        printf("%d \n",*(int*)(block));
//        printf("%s \n",(char*)(block+PTRSIZE));
//        printf("%d \n",*(int*)(block+key_length+PTRSIZE));
//        printf("%d \n",*(int*)(block+2*(key_length+PTRSIZE)));
    }
    if(BF_WriteBlock(fileDesc, index_block_ptr)<0){
        AM_errno=0;
        AM_PrintError("In AM_InsertEntry (WriteBlock)");
        return -1;
    }
    cur_height+=1;
    curInfo->height=cur_height;
    curInfo->root=index_block_ptr;

    updateIndexInfo(fileDesc,cur_height,index_block_ptr);
    return 0;
}

void print_root(int fileDesc, FileInfo curInfo){
    void* block;
    int i;
    if(curInfo.root!=0){
        BF_ReadBlock(fileDesc,curInfo.root,&block);
        int pairs=filledEntriesInBlock(block,curInfo.rec.attrLength1,curInfo.inpb);
        if(curInfo.rec.attrType1==INTEGER){
            printf("%d \n",*(int*)(block));
            for(i=0;i<pairs;i++){
                printf("%d \n",*(int*)(block+sizeof(int)+(i+1)*2*sizeof(int)));
                printf("%d \n",*(int*)(block+i*2*sizeof(int)));
            }
        }
    }
}

PathList* findDataBlockAndPath(FileInfo curInfo, void* key) {
	int* path_array = malloc(curInfo.height * sizeof(int));
	PathList* list = initPathList();
//	printf("Height: %d\n", curInfo.height);
	if (curInfo.height == 0) {
		addChildren(list, 1);
	} else {
//		printf("Root is: %d\n", curInfo.root);
		int data_block_ptr = findLeafNode(1, curInfo.rec.attrType1,
				key, curInfo, curInfo.root, path_array);
		void* myblock;
		int counter;
		for (counter = 0; counter < curInfo.height; counter++) {
			if (!BF_ReadBlock(curInfo.fd, path_array[counter], &myblock)) {
//				int counter;
//				printf("Current block: %d\n", curInfo.root);
//				for (counter = 0; counter < curInfo.lnpb && counter < filledEntriesInBlock(myblock, PTRSIZE, curInfo.lnpb); counter++) {
//					printf(" Ptr: %d \n Key: %s -", *(int*)(myblock+counter*(PTRSIZE + curInfo.rec.attrLength1)),
//							(char*)(myblock+counter * (PTRSIZE + curInfo.rec.attrLength1) + PTRSIZE));
//				}
//				printf("Ptr: %d\n", *(int*)(myblock+(counter+1)*(PTRSIZE + curInfo.rec.attrLength1)));
			}
			addChildren(list, path_array[counter]);
		}
		addChildren(list, data_block_ptr);
	}
	free(path_array);
	return list;
}

int findKeyInDataBlock(void* myblock, void* key, char type, int numOfPairsInBlock, int al1, int al2) {
	int counter;
	void* off = myblock + sizeof(int);
	int pairSize = al1 + al2;
	if (type == INTEGER) {
		for (counter = 0; counter < numOfPairsInBlock && counter < *(int*)myblock; ++counter) {
			if ( *(int*)off == *(int*)key ) {
				return counter;
			}
			off += pairSize;
		}
	} else if (type == FLOAT) {
		for (counter = 0; counter < numOfPairsInBlock && counter < *(int*)myblock; ++counter) {
			if ( *(float*)off == *(float*)key ) {
				return counter;
			}
			off += pairSize;
		}
	} else if (type == STRING) {
		for (counter = 0; counter < numOfPairsInBlock && counter < *(int*)myblock; ++counter) {
			if ( !strncmp(off, key, al1) ) {
				return counter;
			}
			off += pairSize;
		}
	}
	return -1;
}

int isNextPairEqual(void* myblock, void* key, char type, int numOfPairsInBlock, int al1, int al2, int position) {
	if (position + 1 >= *(int*)myblock)
		return 0;
	void* off = myblock + sizeof(int) + (position + 1) * (al1 + al2);
	return isEqual(off, key, type, al1);
}

PathList* mostlyLeftPath(FileInfo file) {
	// For every block before data blocks return the most left pointer
	PathList* list = initPathList();
	if (file.height == 0) {
		addChildren(list, 1);
	} else {
		int counter;
		void* myblock = NULL;
		int tempb = file.root;
		for (counter = 0; counter <= file.height; ++counter) {
			if (!BF_ReadBlock(file.fd, tempb, &myblock)) {
				addChildren(list, tempb);
				tempb = *(int*)(myblock);
			} else {
				AM_PrintError("In mostlyLeftPath (BF_ReadBlock)");
				return list;
			}
		}
	}
	return list;
}

int isEqual(void* blockValuePtr, void* key, char type, int length) {
	if (type == INTEGER) {
		if ( *(int*)blockValuePtr == *(int*)key ) {
			return 1;
		}
	} else if (type == FLOAT) {
		if ( *(float*)blockValuePtr == *(float*)key ) {
			return 1;
		}
	} else if (type == STRING) {
		if ( !strncmp(blockValuePtr, key, length) ) {
			return 1;
		}
	}
	return 0;
}

int isLessThan(void* blockValuePtr, void* key, char type, int length) {
	if (type == INTEGER) {
		if ( *(int*)blockValuePtr < *(int*)key ) {
			return 1;
		}
	} else if (type == FLOAT) {
		if ( *(float*)blockValuePtr < *(float*)key ) {
			return 1;
		}
	} else if (type == STRING) {
		if ( strncmp(blockValuePtr, key, length) < 0 ) {
			return 1;
		}
	}
	return 0;
}

int isLessThanOrEqual(void* blockValuePtr, void* key, char type, int length) {
	if (type == INTEGER) {
		if ( *(int*)blockValuePtr <= *(int*)key ) {
			return 1;
		}
	} else if (type == FLOAT) {
		if ( *(float*)blockValuePtr <= *(float*)key ) {
			return 1;
		}
	} else if (type == STRING) {
		if ( strncmp(blockValuePtr, key, length) <= 0 ) {
			return 1;
		}
	}
	return 0;
}

int findFirstGreater(void* myblock, void* key, char type, int numOfPairsInBlock, int al1, int al2) {
	int counter;
	void* off = myblock + sizeof(int);
	int pairSize = al1 + al2;
	if (type == INTEGER) {
		for (counter = 0; counter < numOfPairsInBlock && counter < *(int*)myblock; ++counter) {
			if ( *(int*)off > *(int*)key ) {
				return counter;
			}
			off += pairSize;
		}
	} else if (type == FLOAT) {
		for (counter = 0; counter < numOfPairsInBlock && counter < *(int*)myblock; ++counter) {
			if ( *(float*)off > *(float*)key ) {
				return counter;
			}
			off += pairSize;
		}
	} else if (type == STRING) {
		for (counter = 0; counter < numOfPairsInBlock && counter < *(int*)myblock; ++counter) {
			if ( strncmp(off, key, al1) > 0 ) {
				return counter;
			}
			off += pairSize;
		}
	}
	return -1;
}

int findFirstGreaterOrEqual(void* myblock, void* key, char type, int numOfPairsInBlock, int al1, int al2) {
	int counter;
	void* off = myblock + sizeof(int);
	int pairSize = al1 + al2;
	if (type == INTEGER) {
		for (counter = 0; counter < numOfPairsInBlock && counter < *(int*)myblock; ++counter) {
			if ( *(int*)off >= *(int*)key ) {
				return counter;
			}
			off += pairSize;
		}
	} else if (type == FLOAT) {
		for (counter = 0; counter < numOfPairsInBlock && counter < *(int*)myblock; ++counter) {
			if ( *((float*)off) >= *(float*)key ) {
				return counter;
			}
			off += pairSize;
		}
	} else if (type == STRING) {
		for (counter = 0; counter < numOfPairsInBlock && counter < *(int*)myblock; ++counter) {
			if ( strncmp(off, key, al1) >= 0 ) {
				return counter;
			}
			off += pairSize;
		}
	}
	return -1;
}


void print_data_blocks(FileInfo curInfo){
	int i,block_ptr=curInfo.root;
	int key_length=curInfo.rec.attrLength1;
	int value_length=curInfo.rec.attrLength2;
	int pair_size=key_length+value_length;
	void* block;
	for(i=0;i<curInfo.height;i++){
		if(BF_ReadBlock(curInfo.fd,block_ptr,&block)<0)
			printf("bla1 \n");
		block_ptr=*(int*)(block);
	}
	int data_block=*(int*)(block);
	do{
		if(BF_ReadBlock(curInfo.fd,data_block,&block)<0)
			printf("bla2 \n");
		int num_of_recs=*(int*)(block);
		printf("Num of recs: %d \n",num_of_recs);
		for(i=0;i<num_of_recs;i++){
			printf("Block: %d, ",data_block);
			printf("Key: %s, ", (char*)(block+sizeof(int)+i*pair_size));
			printf("Value: %d \n", *(int*)(block+sizeof(int)+i*pair_size+key_length));
		}
		data_block=*(int*)(block+sizeof(int)+curInfo.dnpb*pair_size);
	}while(data_block!=-1);
}
