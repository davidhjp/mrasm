%{

#include <stdio.h>
#include <string.h>

#include "table.h"
#include "machine.h"

/* Address Modes */
const unsigned short IM_AM = 0X0000;
const unsigned short DIR_AM = 0X8000;
const unsigned short IND_AM = 0XC000;
const unsigned short ST_AM = 0X4000;

//extern int lineNum;
/*You are exporting these definitions so that they can be used somewhere else*/
extern char macroName[20];
extern FILE *macroFile;
extern char macroArgs[][20];
extern int argCount;
extern int macroDepth;

extern int condFlag;
extern int condMode;

extern int startMacro;

int x, y;

extern int pass;

extern FILE *output;
int instrCount = 0;
ENTRY *previousLabel = NULL;
char previousLabelText[20];
char tempString[50];

unsigned long instrParam[3];
int store_line(OPERATION *ptr, int format, int numParam);

%}

%union {
	char *str;
	ENTRY *ptr;
	int num;
}
%type <num> expression instruction macexp macexpexp
%token <str> LABEL MACLABEL MACWORD CONDOP MACARG MACEXP
%token <ptr> OPCODE DIRECTIVE
%token <num> NUMBER REGISTER
%left '-' '+'
%left '*' '/'
%left ','

%%

statement_list:	statement
	|	statement_list statement
	;

statement: LABEL 				{
		// labels are added in first pass
		printf("This is the LABEL to be added %s\n",$1);
		//if(strcmp($1,"RUN0") == 0){printf("put break point here and see what happens next\n");}
		if (( table_add($1, label, instrCount) == 0 ) && (!pass)) printf("problem, label not added!\n");
		previousLabel = table_find($1);
		strcpy(previousLabelText, $1);
	}
    |   MACLABEL    {
		fprintf(macroFile, "%s", $1);
	}
	|	instruction statement
	|	macwords statement
	|	'\n'
	;

macwords: MACWORD	{
		// each word in the macro definition is checked with params
		for (y = 0, x = 0; x < argCount; x++)
		{
			if (strcmp(macroArgs[x], $1) == 0) {
				y = 1;
				break;
			}
		}
		if (y) fprintf(macroFile, "\t@%d", x);
		else fprintf(macroFile, "\t%s", $1);
	}
	;

instruction: DIRECTIVE expression	{
		if (strcmp($1->text, "ORG") == 0)
		{
			//fprintf(output, "\t\t&ORG\t$%04x\n", $2);
			instrCount = $2;
		}
		else if (strcmp($1->text, "EQU") == 0)
		{
			table_mod(previousLabel->text, symbol, $2);
			//fprintf(output, "%s\t\t&EQU\t%04x\n", previousLabel->text, $2);
		}
	}
	// this rule is to handle macro definition
	|	DIRECTIVE '(' macexp ')'	{
		table_mod(previousLabel->text, macro, $3);
		strcpy(macroName, previousLabel->text);
	}
	// this rule is to handle macro expansion
	|	MACEXP '(' macexpexp ')'		{
		startMacro = 1;
	}
	|	DIRECTIVE expression CONDOP expression	{
		// signal lexer that conditional assembly has been evaluated
		condFlag = 1;

		if (strcmp($3, "==") == 0) condMode = ($2 == $4) ? 1 : 0;
		else if (strcmp($3, "!=") == 0) condMode = ($2 != $4) ? 1 : 0;
		else if (strcmp($3, ">") == 0) condMode = ($2 > $4) ? 1 : 0;
		else if (strcmp($3, "<") == 0) condMode = ($2 < $4) ? 1 : 0;
		else if (strcmp($3, ">=") == 0) condMode = ($2 >= $4) ? 1 : 0;
		else if (strcmp($3, "<=") == 0) condMode = ($2 <= $4) ? 1 : 0;

	}
	// now follow rules for the 10 types of instruction format
	|	OPCODE REGISTER REGISTER REGISTER {
		//fprintf(output, "\t\t%s\tR%d\tR%d\tR%d\n", $1->text, $2, $3, $4);
		instrCount++;

		instrParam[0] = $2;	instrParam[1] = $3;	instrParam[2] = $4;
		store_line($1->instr, 0, 3);
	}
	|	OPCODE REGISTER REGISTER expression	{
		//fprintf(output, "\t\t%s\tR%d\tR%d\t$%04x\n", $1->text, $2, $3, $4);
		instrCount += 2;

		instrParam[0] = $2; instrParam[1] = $3; instrParam[2] = $4;
		store_line($1->instr, 1, 3);
	}
	|	OPCODE REGISTER REGISTER '#' expression	{
		printf("Found: %s\tR%d\tR%d\t%d\n",$1->text,$2,$3,$5);
		//fprintf(output, "\t\t%s\tR%d\tR%d\t#$%04x\n", $1->text, $2, $3, $5);
		instrCount += 2;

		instrParam[0] = $2; instrParam[1] = $3; instrParam[2] = $5;
		store_line($1->instr, 2, 3);
	}
	|	OPCODE REGISTER expression	{
		fprintf(stdout, "\t\t%s\tR%d\t$%04x\n", $1->text, $2, $3);
		//fprintf(output, "\t\t%s\tR%d\t$%04x\n", $1->text, $2, $3);
		instrCount += 2;

		instrParam[0] = $2; instrParam[1] = $3;
		store_line($1->instr, 3, 2);
	}
	|	OPCODE REGISTER REGISTER {
		//fprintf(output, "\t\t%s\tR%d\tR%d\n", $1->text, $2, $3);
		instrCount++;

		instrParam[0] = $2; instrParam[1] = $3;
		store_line($1->instr, 4, 2);
	}
	|	OPCODE REGISTER '#' expression	{
		printf("Found: %s\t%d\t%d\n",$1->text,$2,$4);
                //fprintf(output, "\t\t%s\tR%d\t#$%04x\n", $1->text, $2, $4);
		instrCount += 2;

		instrParam[0] = $2; instrParam[1] = $4;
		store_line($1->instr, 5, 2);
	}
	|	OPCODE REGISTER {
		//fprintf(output, "\t\t%s\tR%d\n", $1->text, $2);
		instrCount++;

		instrParam[0] = $2;
		store_line($1->instr, 6, 1);
	}
	|	OPCODE '#' expression	{
		printf("Found: %s\t%d\n",$1->text,$3);
		//fprintf(output, "\t\t%s\t#$%04x\n", $1->text, $3);
		switch($1->instr->opcode){  
			case 0xbc000000 : 
				printf("YUMMY\n"); ++instrCount; break;
			default : instrCount += 2; break; 
		}
		//instrCount += 2;

		instrParam[0] = $3;
		store_line($1->instr, 7, 1);
	}
	|	OPCODE expression	{
		printf("Found: %s\t%d\n",$1->text,$2);
		//fprintf(output, "\t\t%s\t$%04x\n", $1->text, $2);
		printf("%08X\n",$1->instr->opcode);
		switch($1->instr->opcode){  case 0xbc000000 : printf("found ESL\n"); ++instrCount; break;
		default : instrCount += 2; break; }
		// instrCount += 2;

		instrParam[0] = $2;
		store_line($1->instr, 8, 1);
	}
	|	OPCODE {
		//fprintf(output, "\t\t%s\n", $1->text);
		instrCount++;
		printf("Instruction Count: %X\n",instrCount);

		store_line($1->instr, 9, 0);
	}
	;

expression: NUMBER	{ $$ = $1; }
	|	expression '+' expression	{ $$ = $1 + $3; }
	|	expression '-' expression	{ $$ = $1 - $3; }
	|	expression '*' expression	{ $$ = $1 * $3; }
	|	expression '/' expression	{ $$ = $1 / $3; }
	;

macexp:	MACARG	{
		strcpy(macroArgs[argCount++], $1);
	}
	|	macexp ',' macexp	;
	;

macexpexp: expression	{
		sprintf(tempString, "%d", $1);
		strcpy(macroArgs[argCount++], tempString);
	}
	|	'#' expression	{
		sprintf(tempString, "#%d", $2);
		strcpy(macroArgs[argCount++], tempString);
	}
	|	REGISTER	{
		sprintf(tempString, "R%d", $1);
		strcpy(macroArgs[argCount++], tempString);
	}
	|	macexpexp ',' macexpexp	;
	;

%%

int yyerror(char *s)
{
	fprintf(stderr, "YACC ERROR: %d %s\n", instrCount, s);
	exit(1);
}

int store_line(OPERATION *ptr, int format, int numParams)
{
	char addrMode;
	int operand;
	char zReg, yReg, xReg, aSig, bSig;
	int i;

	if (!pass) return 1;

	operand = 0;
	zReg = yReg = xReg = aSig = bSig = 0;

	// select addressing mode
	addrMode = ptr->format[format][0];

	// select operands
	for (i = 0; i < numParams; i++)
	{
		switch ( ptr->format[format][i+1] )
		{
			case 'v': operand = instrParam[i]; break; //I get the operand for Jump from here
			case 'z': zReg = instrParam[i]; break;
			case 'y': yReg = instrParam[i]; break;
			case 'x': xReg = instrParam[i]; break;
			case 's': aSig = instrParam[i]; break;
			case 'b': bSig = instrParam[i]; break;
		}
	}

	// store instruction
	if ( prog_add(instrCount-1, addrMode, ptr->opcode, operand, zReg, yReg, xReg, aSig, bSig) == 0 )
	{
		printf("failed to add instruction!\n");
		return 0;
	}

	return 1;
}
