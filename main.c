//
// TP assembler for SystemJ... hacked out by me
// for 16-bit PM the professor
//

#include <stdlib.h>
#include <stdio.h>
#include "machine.h"
#include <string.h>

extern INSTRUCTION *prog_head;
extern int instrCount;

int romSize = 1024;
int x, y, z;
char tempString[1024];

void showUsage(void);
void generateMifOutput(char *fileName);
void generateHexOutput(char *fileName);
void generateRawOutput(char *fileName);

void main(int argc, char **argv)
{
	FILE *ftest;
	char fileNameTable[4][1024];
	char outputFileName[] = "out";	// default output filename

	printf(".----------------------------- ---- --\n");
	printf("| SystemJ ASSEMBLER v1.0(16-bit)  (c) 2008\n");
	printf("`----------------------------- -- ------.\n");

	// check for mandatory input file argument
	if (argc > 1)
	{
		ftest = fopen(argv[1], "r");
		if (!ftest)
		{
			printf("problem opening file: %s!\n", argv[1]);
			exit(0);
		}
		fclose(ftest);
	}
	else
	{
		showUsage();
		exit(0);
	}

	// handle command line options
	strcpy(fileNameTable[3], "newr.ini"); //I have renamed this to r.ini should be mrasm.ini
	for (x = 2; x < argc; x++)
	{
		if (argv[x][0] == '-')
		{
			switch (argv[x][1])
			{
				case 's':
					romSize = atoi(argv[x]+2);
					break;
				case 'o':
					strcpy(outputFileName, argv[x]+2);
					break;
				case 'i':
					sprintf(fileNameTable[3], "%s.ini", argv[x]+2);
					break;
				default:
					printf("Invalid arguments!\n\n");
					showUsage();
					break;
			}
		}
	}

	// setup variables holding output filenames
	sprintf(tempString, "%s.txt", outputFileName);
	strcpy(fileNameTable[0], tempString);
	sprintf(tempString, "%s.mif", outputFileName);
	strcpy(fileNameTable[1], tempString);
	sprintf(tempString, "%s.hex", outputFileName);
	strcpy(fileNameTable[2], tempString);

	if (mrasm(argv[1], fileNameTable[3], fileNameTable[0]) !=1)
	{

		printf("fatal error: assembly process did not complete successfully\n");
		exit(0);
	}

	printf("Doing others\n");

	printLine("Generating output machine code");

	generateMifOutput("rawOutput.mif");
	//generateHexOutput(fileNameTable[2]);
	generateRawOutput("rawOutput.hex");

	printLine("Assembly process complete");
	printLine("");

	printf("  - --- --------------------------------'\n");
	/*
	char input;
	printf("Please Enter s to simulate\n");
	scanf("%c",&input);
	input  = 'c';
	if(input == 's'){
		;//simulate(prog_head); Currently the simulator is broken needs to be fixed OK..
	}
	else{
		printf("Completed Processing\n");
	}*/
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
	unsigned short complete;
	unsigned short complete2;

	// open mif file and write header
	output = fopen(fileName, "w");
	fprintf(output, "WIDTH = 16;\n");
	fprintf(output, "DEPTH = %d;\n\n", romSize);
	fprintf(output, "ADDRESS_RADIX = HEX;\n");
	fprintf(output, "DATA_RADIX = HEX;\n\n");
	fprintf(output, "CONTENT\n");
	fprintf(output, "\tBEGIN\n");
	fprintf(output, "\t\t[00..%X]: FFFF;\n", romSize-1);

	ptr = prog_head;
	while ( ptr != NULL )
	{
		// add addresing mode offset
		if(ptr->addrMode == 'I' && (ptr->opcode & 0xFFFFFFFF) == 0xbc000000 ){
			complete = 0x0000;
			complete += (unsigned short)((ptr->opcode & 0xFFFFFFFF) >> 18);
			fprintf(output,"%04X\n",complete);
		}else{
			switch (ptr->addrMode)
			{
				case 'I': complete = 0x4000; break;
				case 'D': complete = 0x8000; break;
				case 'R': complete = 0xc000; break;
				case 'S': complete = 0x0000; break;
			}
			// add other fields if non-zero
			printf("instrCount: %0X\n",ptr->instrCount);
			complete += ((unsigned short)((ptr->opcode & 0xFFFFFFFF) >> 18));
	//		complete += (ptr->operand << 9);
			complete += (ptr->zReg << 4);
	//		complete += (ptr->yReg << 3);
			complete += (ptr->xReg);
	//		complete += (ptr->aSig << 5);
	//		complete += ptr->bSig;

			//This should always happen OK....
			if(ptr->addrMode == 'R' || ptr->addrMode == 'S') {fprintf(output, "\t\t%X\t:%04X;\n", ptr->instrCount, complete); }
			else if(ptr->addrMode == 'D') {fprintf(output, "\t\t%X\t:%04X;\n", (ptr->instrCount - 1),complete);
				fprintf(output,"\t\t%X\t:%04X;\n", ptr->instrCount, ptr->operand);
			}
			else if(ptr->addrMode == 'I'){
				int c = ptr->instrCount;
				fprintf(output, "\t\t%X\t:%04X;\n", --c, complete);
				fprintf(output,"\t\t%X\t:%04X;\n", ptr->instrCount, ptr->operand);
			}
		}
		ptr = ptr->next;
	}

	fprintf(output, "\tEND;\n");
	fclose(output);
}


void generateRawOutput(char *fileName){
	unsigned short complete;
	INSTRUCTION *ptr;
	FILE *output;

	FILE *LUT = NULL;
/** (+)sungchul **/
  FILE *debug;
  int line = 1;
  debug = fopen("debug.lines", "w");
/** **/
	LUT = fopen("LUT","w");

	output = fopen(fileName, "w");

	if(LUT == NULL) {fprintf(stderr, "Could not open file LUT for writing\n"); exit(1); }
	ptr = prog_head;
	while(ptr != NULL){
		/*This is the special ESL instruction transformation into inherent one*/
		if(ptr->addrMode == 'I' && (ptr->opcode & 0xFFFFFFFF) == 0xbc000000 ){
			complete = 0x0000;
			complete += ((ptr->opcode & 0xFFFFFFFF) >> 18);
			fprintf(output,"%04X\n",complete);
/** (+)sungchul **/
      fprintf(debug, "%04x\n", line); line++;
/** **/
			fprintf(LUT,"%d\n%d\n",ptr->instrCount,ptr->operand);
		}else{
		//Lets start building the raw hex data for putting in PM.
			switch(ptr->addrMode){
				case 'I': complete = 0x4000; break;
				case 'D': complete = 0x8000; break;
				case 'R': complete = 0xc000; break;
				case 'S': complete = 0x0000; break;
			}
			complete += ((unsigned short)((ptr->opcode & 0xFFFFFFFF) >> 18));
			complete += (ptr->zReg << 4);
			complete += (ptr->xReg);
			if(ptr->addrMode == 'S' || ptr->addrMode == 'R'){
				fprintf(output,"%04X\n",complete);
/** (+)sungchul **/
      fprintf(debug, "%04x\n", line); line++;
/** **/
			}
			else if(ptr->addrMode == 'I' || ptr->addrMode == 'D'){
				fprintf(output,"%04X\n",complete);
				fprintf(output,"%04X\n",ptr->operand);
/** (+)sungchul **/
      fprintf(debug, "%04x\n", line);
      fprintf(debug, "%04x\n", line); line++;
/** **/
			}
			else {printf("Areya Nai\n");exit(1); }
		}
		ptr = ptr->next;
	}
/** (+)sungchul **/
  fclose(debug);
/** **/
	fclose(output);
	fclose(LUT);
}

void generateHexOutput(char *fileName)
{
	unsigned short complete;
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
			case 'I': complete = 0x4000; break; //01
			case 'D': complete = 0x8000; break; //10
			case 'R': complete = 0xc000; break; //11
			case 'S': complete = 0x0000; break; //00 (inherent)
		}
		// add other fields if non-zero
		complete += ((unsigned short)((ptr->opcode & 0xFFFFFFFF) >> 18));
		//complete += (ptr->operand << 9);
		complete += (ptr->zReg << 4);
		//complete += (ptr->yReg << 3);
		complete += (ptr->xReg);
//		complete += (ptr->aSig << 5);
//		complete += ptr->bSig;
		printf("Complete is: %04X\n",complete);
		// calculate checksum and one's complement for hex file
		x = 0;
		if(ptr->addrMode == 'S' || ptr->addrMode == 'R'){
			sprintf(tempString, "02%04X00%04X", ptr->instrCount, complete);
			for (y = 0; y < 12; y += 2)
			{
				sscanf(&tempString[y], "%02X", &z);
				printf("z: %02X\n",z);
				x += z;
			}
			x = -x;
			x &= 0xff;
			fprintf(output, ":%s%02X\n", tempString, x);
		}
		else if(ptr->addrMode == 'D'){
			x=0;
			sprintf(tempString, "02%04X00%04X", (ptr->instrCount-1), complete);
			for (y = 0; y < 12; y += 2)
			{
				sscanf(&tempString[y], "%02X", &z);
				x += z;
			}
			x = -x;
			x &= 0xff;
			fprintf(output, ":%s%02X\n", tempString, x);
			x=0;
			sprintf(tempString, "02%04X00%04X", ptr->instrCount, ptr->operand);
			for (y = 0; y < 12; y += 2)
			{
				sscanf(&tempString[y], "%02X", &z);
				x += z;
			}
			x = -x;
			x &= 0xff;
			fprintf(output, ":%s%02X\n", tempString, x);
		}
		else{
			int c = ptr->instrCount;
			sprintf(tempString,"02%04X00%04X", --c, complete);
			for (y = 0; y < 12; y += 2)
			{
				sscanf(&tempString[y], "%02X", &z);
				x += z;
			}
			x = -x;
			x &= 0xff;
			fprintf(output, ":%s%02X\n", tempString, x);

			//For the operand...
			x=0;
			sprintf(tempString, "02%04X00%04X", ptr->instrCount, ptr->operand);
			for (y = 0; y < 12; y += 2)
			{
				sscanf(&tempString[y], "%02X", &z);
				x += z;
			}
			x = -x;
			x &= 0xff;
			fprintf(output, ":%s%02X\n", tempString, x);
		}
		ptr = ptr->next;
	}

	fprintf(output, ":00000001FF\n");
	fclose(output);
}
