//
// SystemJ assembler v1.0 /2006
// Coded by: The professor.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string.h>
#include "snowprocessor_snowsimulateprocs.h" /* REPLACE HERE ... (sungchul) */
#include "simulate.h"

extern INSTRUCTION *prog_head;
extern int instrCount;

int romSize = 1024;
int x, y, z;
char tempString[50];

void showUsage(void);
void generateMifOutput(char *fileName);
void generateHexOutput(char *fileName);

JNIEXPORT void JNICALL Java_snowprocessor_snowsimulateprocs_maincall /* REPLACE HERE ... (sungchul) */
  (JNIEnv *env, jobject obj, jstring asmfile)
{
	const char* str; //This will hold the asfilename
	FILE *ftest=NULL;
	char fileNameTable[4][30];
	char outputFileName[] = "out";	// default output filename
	
	str = (*env)->GetStringUTFChars(env,asmfile,NULL);
	if(str == NULL){
		printf("Memory allocation for filename string failed\n");
		exit(1);
	}	
	
	printf(".----------------------------- ---- --\n");
	printf("| SystemJ ASSEMBLER-v 1.0   (c) 2008\n");
	printf("`----------------------------- -- ------.\n");
	
	printf("This is the filename C got: %s\n",str);
	
	//File name cannot exceed 128 chars
	char filebufname[128]; bzero(filebufname,128);

	strcpy(filebufname,str); (*env)->ReleaseStringUTFChars(env,asmfile,str);

	ftest = fopen(filebufname, "r");
	if (!ftest)
	{
		printf("problem opening file: %s!\n", str);
		exit(0);
	}
	fclose(ftest);
	
	//Just fill the parse-lookup-table with "r.ini"
	strcpy(fileNameTable[3], "newr.ini"); //I have renamed this to r.ini should be mrasm.ini
	
	//setup variables holding output filenames
	sprintf(tempString, "%s.txt", outputFileName);
	strcpy(fileNameTable[0], tempString);
	sprintf(tempString, "%s.mif", outputFileName);
	strcpy(fileNameTable[1], tempString);
	sprintf(tempString, "%s.hex", outputFileName);
	strcpy(fileNameTable[2], tempString);

	if (mrasm(filebufname, fileNameTable[3], fileNameTable[0]) !=1)
	{
		
		printf("fatal error: assembly process did not complete successfully\n");
		exit(0);
	}
	
	printf("Doing others\n");
	
	printLine("Generating output machine code");

	generateMifOutput(fileNameTable[1]);
	generateHexOutput(fileNameTable[2]);

	printLine("Assembly process complete");
	printLine("");

	printf("  - --- --------------------------------'\n");
	
	char input;
	//printf("Please Enter s to simulate\n");
	//scanf("%c",&input);

	input = 's';
	if(input == 's'){		
		simulate(prog_head,env,obj);
	}
	else{
		printf("Completed Processing\n");
	}
	
}

void showUsage(void)
{
	printf("  Usage:                                |\n");
	printf("    mrasm <input_filename> [option]     |\n");
	printf("  Options:                              |\n");
	printf("    -s<NUM>     set rom size to NUM     |\n");
	printf("    -o<NAME>    set output file name    |\n");
	printf("                to NAME (no suffix)     |\n");
	printf("    -i<NAME>    set instruction set to  |\n");
	printf("                NAME (no suffix)        |\n");
	printf("  - --- --------------------------------'\n");
}

void generateMifOutput(char *fileName)
{
	INSTRUCTION *ptr;
	FILE *output;
	unsigned long complete;

	// open mif file and write header
	output = fopen(fileName, "w");
	fprintf(output, "WIDTH = 32;\n");
	fprintf(output, "DEPTH = %d;\n\n", romSize);
	fprintf(output, "ADDRESS_RADIX = HEX;\n");
	fprintf(output, "DATA_RADIX = HEX;\n\n");
	fprintf(output, "CONTENT\n");
	fprintf(output, "\tBEGIN\n");
	fprintf(output, "\t\t[00..%X]: FFFFFFFF;\n", romSize-1);

	ptr = prog_head;
	while ( ptr != NULL )
	{
		switch (ptr->addrMode)
		{
			case 'I': complete = 0x00000000; break;
			case 'D': complete = 0x80000000; break;
			case 'R': complete = 0xc0000000; break;
			case 'S': complete = 0x40000000; break;
		}
		// add other fields if non-zero
		complete += ptr->opcode;
		if(ptr->opcode == 0x02000000)
			printf("Complete: %08X\n",complete);
		complete += (ptr->operand << 9);
		if(ptr->opcode == 0x02000000){
			printf("Operand: %08X",ptr->operand);
			printf("Complete: %08X\n",complete);
		}
		complete += (ptr->zReg << 6);
		if(ptr->opcode == 0x02000000)
			printf("Complete: %08X\n",complete);
		complete += (ptr->yReg << 3);
		if(ptr->opcode == 0x02000000)
			printf("Complete: %08X\n",complete);		
		complete += ptr->xReg;
		if(ptr->opcode == 0x02000000)
			printf("Complete: %08X\n",complete);
		complete += (ptr->aSig << 5);
		if(ptr->opcode == 0x02000000)
			printf("Complete: %08X\n",complete);
		complete += ptr->bSig;
		if(ptr->opcode == 0x02000000)
			printf("Complete: %08X\n",complete);

		fprintf(output, "\t\t%X\t:%08X;\n", ptr->instrCount, complete);

		ptr = ptr->next;
	}

	fprintf(output, "\tEND;\n");
	fclose(output);
}

void generateHexOutput(char *fileName)
{
	unsigned long complete;
	INSTRUCTION *ptr;
	FILE *output;

	// open hex proramming file
	output = fopen(fileName, "w");

	ptr = prog_head;
	while ( ptr != NULL )
	{
		// add addresing mode offset
		switch (ptr->addrMode)
		{
			case 'I': complete = 0x00000000; break;
			case 'D': complete = 0x80000000; break;
			case 'R': complete = 0xc0000000; break;
			case 'S': complete = 0x40000000; break;
		}
		// add other fields if non-zero
		complete += ptr->opcode;
		complete += (ptr->operand << 9);
		complete += (ptr->zReg << 6);
		complete += (ptr->yReg << 3);
		complete += ptr->xReg;
		complete += (ptr->aSig << 5);
		complete += ptr->bSig;

		// calculate checksum and one's complement for hex file
		x = 0;
		sprintf(tempString, "04%04X00%08X", ptr->instrCount, complete);
		for (y = 0; y < 16; y += 2)
		{
			sscanf(&tempString[y], "%02X", &z);
			x += z;
		}
		x = -x;
		x &= 0xff;
		fprintf(output, ":%s%02X\n", tempString, x);

		ptr = ptr->next;
	}

	fprintf(output, ":00000001FF\n");
	fclose(output);
}

