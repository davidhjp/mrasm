typedef struct _macstate MACSTATE;
struct _macstate
{
	int condFlag;
	int condMode;
	int argCount;
	int instrCount;
	char macroName[20];
	char macroArgs[10][20];
	MACSTATE *next;
};

int macro_push(int condFlag, int condMode, int argCount, int instrCount, char *macroName);
MACSTATE macro_pop(void);
