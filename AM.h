#ifndef AM_H_
#define AM_H_

/* Error codes */

extern int AM_errno;

#define AME_OK 0
#define AME_EOF -1
#define AME_WRONG_ATTRIBUTES_TYPE 10001
#define AME_WRONG_ATTRIBUTES_LENGTH 10002
#define AME_FILE_ALREADY_EXIST -2
#define AME_FILE_NOT_CREATED -3
#define AME_COULD_NOT_ALLOCATE_BLOCK -4
#define AME_COULD_NOT_GET_COUNTER -5
#define AME_OPEN_FILE_DESTROY -6
#define AME_COULD_NOT_DELETE_FILE -7
#define AME_FILE_NOT_OPEN -8
#define AME_WRONG_OPERATOR -9
#define AME_SCAN_INDEX_FULL -10
#define AME_SCAN_ALREADY_CLOSED -11
#define AME_CLOSE_WITH_OPEN_SCAN -12
#define AME_OUT_OF_BOUNDS_FD -13
#define AME_CLOSE_FAILED -14
#define AME_FILE_NOT_OPENED -15
#define AME_OUT_OF_BOUNDS_SD -16
#define AME_FILE_INDEX_FULL -17


typedef enum {
	OPERATOR_ERROR = 0,
	EQUAL = 1,
	NOT_EQUAL = 2,
	LESS_THAN = 3,
	GREATER_THAN = 4,
	LESS_THAN_OR_EQUAL = 5,
	GREATER_THAN_OR_EQUAL = 6
} compOperator;

void AM_Init( void );


int AM_CreateIndex(
  char *fileName, /* όνομα αρχείου */
  char attrType1, /* τύπος πρώτου πεδίου: 'c' (συμβολοσειρά), 'i' (ακέραιος), 'f' (πραγματικός) */
  int attrLength1, /* μήκος πρώτου πεδίου: 4 γιά 'i' ή 'f', 1-255 γιά 'c' */
  char attrType2, /* τύπος πρώτου πεδίου: 'c' (συμβολοσειρά), 'i' (ακέραιος), 'f' (πραγματικός) */
  int attrLength2 /* μήκος δεύτερου πεδίου: 4 γιά 'i' ή 'f', 1-255 γιά 'c' */
);


int AM_DestroyIndex(
  char *fileName /* όνομα αρχείου */
);


int AM_OpenIndex (
  char *fileName /* όνομα αρχείου */
);


int AM_CloseIndex (
  int fileDesc /* αριθμός που αντιστοιχεί στο ανοιχτό αρχείο */
);


int AM_InsertEntry(
  int fileDesc, /* αριθμός που αντιστοιχεί στο ανοιχτό αρχείο */
  void *value1, /* τιμή του πεδίου-κλειδιού προς εισαγωγή */
  void *value2 /* τιμή του δεύτερου πεδίου της εγγραφής προς εισαγωγή */
);


int AM_OpenIndexScan(
  int fileDesc, /* αριθμός που αντιστοιχεί στο ανοιχτό αρχείο */
  int op, /* τελεστής σύγκρισης */
  void *value /* τιμή του πεδίου-κλειδιού προς σύγκριση */
);


void *AM_FindNextEntry(
  int scanDesc /* αριθμός που αντιστοιχεί στην ανοιχτή σάρωση */
);


int AM_CloseIndexScan(
  int scanDesc /* αριθμός που αντιστοιχεί στην ανοιχτή σάρωση */
);


void AM_PrintError(
  char *errString /* κείμενο για εκτύπωση */
);


#endif /* AM_H_ */
