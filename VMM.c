#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>

#define FILENAME 32
#define PAGE_TABLE_ENTRIES 256
#define PAGE_SIZE 256
#define TABLE_ENTRIES 16
#define FRAME_SIZE 256
#define FRAMES 128
#define MEMORY_SIZE (FRAME_SIZE*FRAMES)
 
/***************************************
****************************************/
struct fentry
{
int fnum;
struct fentry *next;
};

typedef struct fentry fflentry;
char *memory;
fflentry *frameList;

/****************************************
*****************************************/
struct pageTblSNode

{
int pnum;
struct pageTblSNode *prev;
struct pageTblSNode *next;
};

typedef struct pageTblSNode pStackNode;
pStackNode *ptop;
pStackNode *pbottom;

typedef struct
{
int pnum;
int fnum;
} TabEntry;

typedef struct
{
int fnum;
bool isValid;
pStackNode *node;
}pageTableEntry;

pageTableEntry * pageTable;
TabEntry *Table;

int fTabIndex;
int *tCounter;
int nRefer;
int nPageFaults;
int nTableHits;
int nCorrectReads;

/**************************************************
***************************************************/
void getAddress(int add, int *pageNum, int *offset)
{
int m = 0xff;

*offset = add & m;
*pageNum = (add >> 8) & m;
}

int getFreeFrame()

{
fflentry *temp;

int frameNum;

pStackNode *pPtr;

if(frameList == NULL)
{
	pPtr = pbottom->prev;
	if(pPtr == ptop)
	{
		printf("ERROR \n");
		return 0;
	}
	pageTable[pPtr -> pnum].isValid = false;
	pbottom -> prev = pPtr -> prev;
	pPtr -> prev -> next = pbottom;

	frameList = (fflentry *)malloc(sizeof(fflentry));
	frameList -> fnum = pageTable[pPtr -> pnum].fnum;
	frameList -> next = NULL;
}
temp = frameList;
frameList = temp -> next;
frameNum = temp -> fnum;
free(temp);

return frameNum;
}

/*********************************************************
**********************************************************/
void InitializePageTable()
{
int i;

pageTable = (pageTableEntry *)malloc(sizeof(pageTableEntry)*PAGE_TABLE_ENTRIES);
Table = (TabEntry *)malloc(sizeof(pageTableEntry)*TABLE_ENTRIES);
fTabIndex = 0;
tCounter = (int *)malloc(sizeof(int)*TABLE_ENTRIES);

ptop = (pStackNode *)malloc(sizeof(pStackNode));
pbottom = (pStackNode *)malloc(sizeof(pStackNode));
ptop -> prev = NULL;
pbottom -> next = NULL;
ptop -> next = pbottom;
pbottom -> prev = ptop;

for(i = 0; i < PAGE_TABLE_ENTRIES; ++i)
{
	pageTable[i].isValid = false;
	pageTable[i].node = (pStackNode *)malloc(sizeof(pStackNode));
	pageTable[i].node -> pnum = i;
}
};

/********************************************************
*********************************************************/
void InitializeMemory()
{
int i; 
int nFrame;

fflentry *temp;

memory = (char *)malloc(MEMORY_SIZE * sizeof(char));
frameList = NULL;
nFrame = MEMORY_SIZE/FRAME_SIZE;

for(i = 0; i < nFrame; ++i)
{
	temp = frameList;
	frameList = (fflentry *)malloc(sizeof(fflentry));
	frameList -> fnum = nFrame -i - 1;
	frameList -> next = temp;
}
}

/*******************************************************
********************************************************/
int translate(int lAdd, FILE *fBackingStore, int tCount)
{
int pageNum; 
int offset; 
int frameNum; 
int pAdd; 

int i; 
int TabI;

int temp1; 
int temp2;

pStackNode *pPtr;

++nRefer;
getAddress(lAdd, &pageNum, &offset);

for(i = 0; i < fTabIndex; ++i)
{
	if(pageNum == Table[i].pnum)
	{
	++nTableHits;
	TabI = i;
	break;
	}
}

if(i == fTabIndex)
{
	if(!pageTable[pageNum].isValid)
	{
	++nPageFaults;
	frameNum = getFreeFrame();
	fseek(fBackingStore, pageNum * PAGE_SIZE, SEEK_SET);
	fread(memory + frameNum * FRAME_SIZE, sizeof(char), FRAME_SIZE, fBackingStore);

	pageTable[pageNum].isValid = true;
	pageTable[pageNum].fnum = frameNum;

	pageTable[pageNum].node -> next = ptop -> next;
	ptop -> next -> prev = pageTable[pageNum].node;
	ptop -> next = pageTable[pageNum].node;
	pageTable[pageNum].node -> prev = ptop;
	}
	frameNum = pageTable[pageNum].fnum;

	if(fTabIndex < TABLE_ENTRIES)
	{
	TabI = fTabIndex;
	++fTabIndex;
	}
	
	else
	{
	temp1 = tCounter[0];
	temp2 = 0;
	
		for(i = 0; i < TABLE_ENTRIES; ++i)
		{
			if(tCounter[i] < temp1)
			{
			temp1 = tCounter[i];
			temp2 = i;
			}
		}
		TabI = temp2;
	}
	Table[TabI].pnum = pageNum;
	Table[TabI].fnum = frameNum;
}
pPtr = pageTable[pageNum].node -> next;
pageTable[pageNum].node -> next = ptop -> next;
ptop -> next -> prev = pageTable[pageNum].node;
ptop -> next = pageTable[pageNum].node;
pPtr -> prev = pageTable[pageNum].node -> prev;
pageTable[pageNum].node -> prev -> next = pPtr;
pageTable[pageNum].node -> prev = ptop;

frameNum = Table[TabI].fnum;
tCounter[TabI] = tCount;
pAdd = (frameNum << 8) | offset;
fseek(fBackingStore, 0, SEEK_SET);

return pAdd;
}

/**********************************************
***********************************************/
char readMemory(int pAdd)
{
return memory[pAdd];
}

/*********************************************
**********************************************/
void ClearPageTable()
{
int i;

for(i = 0; i < PAGE_TABLE_ENTRIES; ++i)
	free(pageTable[i].node);

free(ptop);
free(pbottom);
free(pageTable);
free(Table);
free(tCounter);
}

/*********************************************
**********************************************/
void ClearMemory()
{
fflentry *temp;
free(memory);

while(frameList)
{
temp = frameList;
frameList = temp -> next;
free(temp);
}
}

/********************************************
*********************************************/
int main(int argc, char *argv[])
{
int lAdd; 
int pAdd;
int cLAdd; 
int cPAdd; 
int cValue;

FILE *fAdd; 
FILE *fBackingStore; 
FILE *fCorrect;

int i;
char content;

InitializePageTable();
InitializeMemory();

fBackingStore = fopen("BACKING_STORE.bin", "rb");
fCorrect = fopen("correct.txt", "r");

if(fBackingStore == NULL)
{
printf("ERROR: FILE DOES NOT EXIST. \n");
return 1;
}

if(fCorrect == NULL)
{
printf("ERROR: FILE DOES NOT EXIST. \n");
return 1;
}

nRefer = 0;
nPageFaults = 0;
nTableHits = 0;
nCorrectReads = 0;

fAdd = fopen( argv[1], "r");

if(fAdd == NULL)
{
printf("ERROR: UNABLE TO OPEN FILE. \n");
return 1;
}

i = 0;

while(~fscanf(fAdd, "%d", &lAdd))
{
pAdd = translate(lAdd,fBackingStore, i);
content = readMemory(pAdd);
printf("VIRTUAL ADDRESS: %d PHYSICAL ADDRESS: %d VALUE: %d \n", lAdd, pAdd, content);
++i;
fscanf(fCorrect, "VIRTUAL ADDRESS: %d PHYSICAL ADDRESS: %d VALUE: %d \n", &cLAdd, &cPAdd, &cValue);
if(lAdd != cLAdd)
printf("ADDRESS IS INCORRECT \n");
if(cValue == (int)content)
++nCorrectReads;
}

printf("PHYSICAL MEMORY SIZE: %d \n", MEMORY_SIZE);
printf("VIRUAL MEMORY SIZE: %d \n", PAGE_TABLE_ENTRIES * PAGE_SIZE);
printf("NUMBER OF FRAMES: %d \n", FRAMES);
printf("NUMBER OF PAGE TABLE ENTRIES %d", PAGE_TABLE_ENTRIES);
printf("\n");
printf("PAGE FAULT RATE %f \n", (double) nPageFaults / nRefer);
printf("TABLE HIT RATE %f \n", (double) nTableHits / nRefer);

fclose(fAdd);
fclose(fBackingStore);
fclose(fCorrect);

ClearPageTable();
ClearMemory();

return 0;
}
