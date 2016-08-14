#include <stdlib.h>
#include "macstack.h"

MACSTATE *stack_head = NULL;

int macro_push(int condFlag, int condMode, int argCount, int instrCount, char *macroName) //, char macroArgs[][12])
{
	MACSTATE *ptr;
	int x, y;

	// allocate a new macro state 
	ptr = (MACSTATE *) malloc(sizeof(MACSTATE));
	if (ptr == NULL)
	{
		printf("malloc failed!\n");
		exit(0);
		return 0;
	}
	ptr->next = stack_head;

	// copy in state variables
	ptr->condFlag = condFlag;
	ptr->condMode = condMode;
	ptr->argCount = argCount;
	ptr->instrCount = instrCount;

	stack_head = ptr;

	return 1;
}

MACSTATE macro_pop(void)
{
	MACSTATE rval;
	MACSTATE *ptr;
	int x;
	
	// copy state variables from stack entry
	rval.condFlag = stack_head->condFlag;
	rval.condMode = stack_head->condMode;
	rval.argCount = stack_head->argCount;
	rval.instrCount = stack_head->instrCount;
	strcpy(rval.macroName, stack_head->macroName);

	// copy in macro arguments from stack entry
	for (x = 0; x < 10; x++)
	{
		strcpy(rval.macroArgs[x], stack_head->macroArgs[x]);
	}
	
	// remove stack entry from stack
	ptr = stack_head->next;
	free(stack_head);
	stack_head = ptr;

	return rval;
}
