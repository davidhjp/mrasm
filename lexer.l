%{
	// test lexer 0302

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "table.h"
#include "y.tab.h"
#include "macstack.h"
//#include "macstack.c"

int lineNum = 1;
int x;
char macroArgs[10][20];
char macroName[20];
char macroFileName[30];
int argCount;
int macInstrCount;
FILE *macroFile;
int macroDepth = 0;
int macroMode = 0;				// 0 = read macro, 1 = expand macro
int string = 0;
char scanString[50];
FILE *macroArgument;

int condFlag = 0;
int condMode = 0;

int startMacro = 0;

int macroExpArgsFlag = 0;

extern int pass;
extern int instrCount;
extern char previousLabelText[20];

extern MACSTATE *stack_head;

MACSTATE mstate;

%}

%x INCL 
%x MACBODY
%x IGNORE
%x MACPARAM
%s SPECIAL

%%

<INCL>[a-zA-Z0-9_\-.]+	{
	yyin = fopen(yytext, "r");

	if (!yyin)
	{
		printf("couldn't open include file %s!\n", yytext);
		exit(0);
	}

	yypush_buffer_state( yy_create_buffer( yyin, YY_BUF_SIZE ) );
	BEGIN(INITIAL);
}

<IGNORE>[ \t\r\n]	;
<IGNORE>ELSE {
	if ((condFlag == 1) && (condMode == 0))	BEGIN(INITIAL);
}
<IGNORE>ENDIF {
	condFlag = 0;
	condMode = 0;
	BEGIN(INITIAL);
}
<IGNORE>[^ \t\r\n]+	;

<MACPARAM>[ \t\r\n]		;
<MACPARAM>[,\(\)]	{
	if (yytext[0] == ')')
	{
		sprintf(macroFileName, "macro.%s", previousLabelText);
		macroFile = fopen(macroFileName, "w");

		if (!macroFile)
		{
			printf("couldn't open macro file %s!\n", macroFileName);
			exit(0);
		}
		BEGIN(MACBODY);
	}
	return yytext[0];
}
<MACPARAM>[a-zA-Z0-9_]+	{
	yylval.str = strdup(yytext);
	return MACARG;
}

<MACBODY>\n			{
	fprintf(macroFile, "\n");
	return yytext[0];
}
<MACBODY>[ \r\t]+	;	// ignore whitespace
<MACBODY>;.*		;	// eat up comments
<MACBODY>ENDMAC	{
	BEGIN(INITIAL);
	fclose(macroFile);
}
<MACBODY>^[a-zA-Z][a-zA-Z0-9_]+	{
	yylval.str = strdup(yytext);
	return MACLABEL;
}
<MACBODY>[@$a-zA-Z0-9_]+	{
	yylval.str = strdup(yytext);
	return MACWORD;
}
<MACBODY>[\(\)\-+*/#,]	{
	yylval.str = strdup(yytext);
	return MACWORD;
}
<MACBODY><|==|>=|<=|!=|>	{
	yylval.str = strdup(yytext);
	return MACWORD;
}

<SPECIAL>\)		{
	macroExpArgsFlag = 0;

	BEGIN(INITIAL);
	return ')';
}

\n			{
	if (startMacro == 1)
	{
		startMacro = 0;
		sprintf(macroFileName, "macro.%s", macroName);
		
		yyin = fopen(macroFileName, "r");
		if (!yyin)
		{
			printf("couldn't open macro file %s for macro %s!\n", macroFileName, macroName);
			exit(0);
		}
		yypush_buffer_state( yy_create_buffer( yyin, YY_BUF_SIZE ) );
		return '\n';
	}
	lineNum++;
	return yytext[0];
}

[ \r\t]+	;	// ignore whitespace

;.*			;	// eat up comments

ENDPROG		{
	instrCount = 0;
	return 0;	// terminate
}

INCLUDE { BEGIN(INCL); }

THEN	{
	if ((condFlag == 1) && (condMode == 0))
	{
		BEGIN(IGNORE);
	}
}
ELSE	{
	if ((condFlag == 1) && (condMode == 0))
	{
		BEGIN(INITIAL);
	}
	else
	{
		BEGIN(IGNORE);
	}
}
ENDIF	{
	condFlag = 0;
	condMode = 0;
	BEGIN(INITIAL);
}

<<EOF>>		{
	yypop_buffer_state();

	if ( string == 1 ) string = 0;
	else if (macroExpArgsFlag == 1)
	{
		// continue evaluating macro arguments
		macroExpArgsFlag = 0;
		BEGIN(SPECIAL);
	}
	else if (macroMode == 1)
	{
		if (--macroDepth != 0)
		{
			// if a recursive macro has been successfully parsed then pop the stack
			mstate = macro_pop();
			condFlag = mstate.condFlag;
			condMode = mstate.condMode;
			argCount = mstate.argCount;
			strcpy(macroName, mstate.macroName);
			for (x = 0; x < 10; x++)
			{
				strcpy(macroArgs[x], mstate.macroArgs[x]);
			}
			sprintf(macroFileName, "macro.%s", macroName);
		}
		else
		{
			macroMode = 0;
		}
	}

	// terminate when there are no more files to parse
	if ( !YY_CURRENT_BUFFER ) yyterminate();
}

^[a-zA-Z][a-zA-Z0-9_]+	{
	if (macroDepth > 0)
	{
		// if processing a macro, modify the label for uniqueness
		sprintf(scanString, "%s%04x%d", yytext, macInstrCount, macroDepth);
	}
	else
	{
		strcpy(scanString, yytext);
	}
	yylval.str = strdup(scanString);
	return LABEL;
}

@[0-9]		{
	// evaluate macro argument
	macroArgument = fopen("macro-temp", "w");
	
	x = yytext[1] - 0x30;
	if (macroExpArgsFlag == 1)
	{
		fprintf(macroArgument, "\t%s", stack_head->macroArgs[x]); 
	}
	else
	{
		fprintf(macroArgument, "\t%s", macroArgs[x]);
	}

	fclose(macroArgument);
	
	yyin = fopen("macro-temp", "r");

	if (!yyin)
	{
		printf("couldn't open file macro-temp!\n");
		exit(0);
	}
	string = 1;
	yypush_buffer_state( yy_create_buffer( yyin, YY_BUF_SIZE ) );
	
}

([Cc]?)([Zz]?)([Vv]?)([Nn]?) {
	x = 0;
	
	if (strchr(yytext, 'C') || strchr(yytext, 'c'))	x += 0x8;
	if (strchr(yytext, 'Z') || strchr(yytext, 'z'))	x += 0x4;
	if (strchr(yytext, 'V') || strchr(yytext, 'v'))	x += 0x2;
	if (strchr(yytext, 'N') || strchr(yytext, 'n'))	x += 0x1;

	yylval.num = (x << 12);
	return NUMBER;
}

[RS][0-9]{1,2}		{
	yylval.num = atoi(&yytext[1]);
	return REGISTER;
}
P[0-9]	{
	x = yytext[1] - 0x30;
	x <<= 1;
	yylval.num = x + 1;
	return REGISTER;
}

[a-zA-Z][a-zA-Z0-9_]+	{
	ENTRY *ptr = table_find(yytext);

	if (ptr == NULL)
	{
		sprintf(scanString, "%s%04x%d", yytext, macInstrCount, macroDepth);
		ptr = table_find(scanString);
	}
	
	
	if (ptr == NULL)
	{
		if (pass == 1)
		{
			printf("error: found reference to unknown text: %s\n", yytext);
			exit(0);
		}
		yylval.num = 0;		// probably forward referenced label in first pass!
		return NUMBER;
	}
	else
	{
		switch (ptr->type)
		{
			case macro:
				// macro expansion
				strcpy(macroName, ptr->text);
				macroDepth++;
				if (macroDepth > 1)
				{
					// store the current processing state onto the stack
					if (macro_push(condFlag, condMode, argCount, macInstrCount, macroName) == 1)
					{
						for (x = 0; x < 10; x++)
						{
							strcpy(stack_head->macroArgs[x], macroArgs[x]);
						}
					}
					else
					{
						printf("MACRO STACK PUSH FAAAAAAIL!!!\n");
						exit(0);
					}
				}
				macInstrCount = instrCount;
				macroMode = 1;
				argCount = 0;
				yylval.str = strdup(ptr->text);
				macroExpArgsFlag = 1;
				BEGIN(SPECIAL);
				return MACEXP;
			case opcode:
                                yylval.ptr = ptr;
				return OPCODE;
			case directive:
				yylval.ptr = ptr;
				if (strcmp(ptr->text, "MACRO") == 0)
				{
					// macro definition
					argCount = 0;
					macroMode = 0;
					BEGIN(MACPARAM);
				}
				return DIRECTIVE;
			case label:
				yylval.num = ptr->value;
				return NUMBER;
			case symbol:
				yylval.num = ptr->value;
				return NUMBER;
		}
	}
}

#	{
	return yytext[0];
}

[,\(\)\-+/*]	{
	return yytext[0];
}

>|<|==|>=|<=|!=	{
	yylval.str = strdup(yytext);
	return CONDOP;
}

[0-9]+		{
	x = atoi(yytext);
	yylval.num = x;
	return NUMBER;
}

($)[0-9a-fA-F]+ {
	sscanf(&yytext[1], "%x", &x);
	yylval.num = x;
	return NUMBER;
}

%%
