#include <stdio.h>
#include <stdlib.h>
#include "table.h"
#include "iniparser.h"

ENTRY *table_head = NULL;

int table_add(char *text, TYPE type, int value)
{
	ENTRY *ptr;

	// check if already exists
	if (table_find(text) != NULL)
	{
		printf("keyword already defined: %s\n", text);
		return 0;
	}

	// allocate new definition
	ptr = (ENTRY *) malloc(sizeof(ENTRY));
	if (ptr == NULL)
	{
		printf("bad malloc!!\n");
		return 0;
	}
	ptr->next = table_head;

	// copy in new data
	ptr->type = type;
	ptr->value = value;
	ptr->text = (char *) malloc(strlen(text)+1);
	strcpy(ptr->text, text);

	table_head = ptr;

	return 1;
}
int table_mod(char *text, TYPE type, int value)
{
	ENTRY *ptr = table_find(text);

	if (ptr == NULL) return 0;

	// replace data in existing entry
	ptr->type = type;
	ptr->value = value;

	return 1;
}
ENTRY* table_find(char *text)
{
	ENTRY *ptr = table_head;

	// look through stack for existing entry based on text field
	while (ptr != NULL)
	{
		if (strcmp(ptr->text, text) == 0) return ptr;
		ptr = ptr->next;
	}

	return NULL;
}
// this function reads in assembler data from external file and
// stores in the internal table structure
int populate_table(char *inifile)
{
	ENTRY *ptr;
	char tempStr[50], tempStr2[50];
	char upperText[50], sectName[50];
	char type;
	unsigned long code;
	dictionary *d;		// iniparser dictionary
	int x, y;


	// iniparser method to open instruction set file and create dictionary
	d = iniparser_new(inifile);
	if (d == NULL)
	{
		printf("Couldn't open the instruction set file: %s!\n", inifile);
		return -1;
	}

	// begin parsing the sections in the ini file
	for (x = 0; x < iniparser_getnsec(d); x++)
	{
		// read ini file section name (mnemonic)
		sprintf(sectName, "%s", iniparser_getsecname(d, x));

		// read mnemonic type
		sprintf(tempStr, "%s:type", sectName);
		type = *(iniparser_getstr(d, tempStr));

		// convert section name from lowercase to upper for case sensitivity
		for (y = 0; y < strlen(sectName); y++)
		{
			upperText[y] = toupper(sectName[y]);
		}
		upperText[strlen(sectName)] = '\0';

		// add instruction to table
		switch (type)
		{
			case 'd':	// directive
				table_add(upperText, directive, x);
				break;
			case 'o':	// standard operation
				table_add(upperText, opcode, x);
				ptr = table_find(upperText);

				// read in base opcode
				sprintf(tempStr, "%s:base", sectName);
				strcpy(tempStr2, iniparser_getstr(d, tempStr));
				sscanf(tempStr2, "%x", &code);

				// allocate memory to store instruction format
				ptr->instr = (OPERATION *) malloc(sizeof(OPERATION));

				// check for bad malloc
				if (ptr->instr == NULL)
				{
					printf("bad malloc!\n");
					return 0;
				}
				else
				{
					// load base opcode
					(ptr->instr)->opcode = code;

					// load all instruction formats
					for (y = 0; y < 10; y++)
					{
						sprintf(tempStr, "%s:format%d", sectName, y);

						// check if format is specified for this instruction
						if (iniparser_getstr(d, tempStr) == NULL)
						{
							// if not write null
							(ptr->instr)->format[y] = NULL;
						}
						else
						{
							// otherwise allocate memory and store string
							(ptr->instr)->format[y] = (char *) malloc(strlen(sectName)+1);
							if ((ptr->instr)->format[y] == NULL)
							{
								printf("bad malloc!!\n");
								return 0;
							}
							strcpy((ptr->instr)->format[y], iniparser_getstr(d, tempStr));
						}
					}
				}
				break;
		}
	}

	return 1;
}
void show_table(void)
{
	int i;
	ENTRY *ptr = table_head;

	while (ptr != NULL)
	{
		if (ptr->type == label)
		{
			printf("  LABEL \t%12s\t  %04x  |\n",	ptr->text, ptr->value);
		}
		if (ptr->type == symbol)
		{
			printf("  SYMBOL\t%12s\t  %04x  |\n", ptr->text, ptr->value);
		}		
		else if (ptr->type == macro)
		{
			printf("  MACRO \t%12s\t        |\n", ptr->text);
		}
		ptr = ptr->next;
	}
}
