#include <stdlib.h>
#include "machine.h"

// two global variables pointing to start and end of queue
INSTRUCTION *prog_head = NULL;
INSTRUCTION *prog_current = NULL;

int prog_add(int instrCount, char addrMode, unsigned long opcode, int operand, char zReg, char yReg, char xReg, char aSig, char bSig)
{
	INSTRUCTION *ptr;

	// allocate a new line
	ptr = (INSTRUCTION *) malloc(sizeof(INSTRUCTION));
	if (ptr == NULL)
	{
		printf("malloc failed!\n");
		return 0;
	}
	
	// setup first entry
	if (prog_head == NULL)
	{
		prog_current = prog_head = ptr;
		
	}
	else
	{		
		prog_current->next = ptr;
		prog_current = ptr;
	}
	ptr->next = NULL;

	// copy in instruction data
	ptr->instrCount = instrCount;
	ptr->addrMode = addrMode;
	ptr->opcode = opcode;
	ptr->operand = operand;
	ptr->zReg = zReg;
	ptr->yReg = yReg;
	ptr->xReg = xReg;
	ptr->aSig = aSig;
	ptr->bSig = bSig;

	printf("%08X\n",ptr->opcode);
	return 1;
}
