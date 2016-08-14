//
// MACRO ReMiCORE ASSEMBER 1.00 / 2005
//

#include <stdio.h>
#include <string.h>

#include "table.h"

int pass = 0;
FILE *output;
extern FILE *yyin;

void printLine(char *str)
{
	printf("  %-37s |\n", str);
}

int mrasm(char *inputFile, char *instFile, char *debugFile)
{
	char tempString[1024];

	yyin = fopen(inputFile, "r"); /*Setting the input file source*/

	sprintf(tempString, "Opened file: %s", inputFile);
	printLine(tempString);

	// open debugging output file
	output = fopen(debugFile, "w");
	if (!output)
	{
		printf("Problem opening debugging output file: %s\n", debugFile);
		exit(0);
	}

	// attempt to open and read the assembler instruction set table file
	//This is the most important instruction since this fills the table with all the possible opcodes etc..
	if (populate_table(instFile) < 1)
	{
		printf("Problem loading instruction set\n");
		exit(0);
	}

	// begin preliminary parse to calculate correct labels
	printLine("");
	printLine("Starting first parse");
	yyparse(); //where does this function call end up
	printLine("Finished first parse");
	// restart debugging output file
	fclose(output);
	output = fopen(debugFile, "w");
	if (!output)
	{
		printf("Problem opening debugging output file: %s\n", debugFile);
		exit(0);
	}

	// start second and final parse
	rewind(yyin);
	pass = 1;
	printLine("Starting second parse");
	yyparse();
	printLine("Finished second parse");
	printLine("");
	printLine("Label, macro, and symbol table:");
	printLine("");
	show_table();
	printLine("");
	fclose(output);

	return 1;
}

