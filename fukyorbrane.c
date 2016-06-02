/*
 * Copyright (c) 2016 Leonard König
 * Copyright (c) 2005 Gregor Richards
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef MAX_PROG_LEN
#define MAX_PROG_LEN 32256
#endif

#ifndef MAX_PROC_COUNT
#define MAX_PROC_COUNT 1024
#endif

struct PData {
	/* program data */
	int data;
	/* a command is "spent" if it's a fork that's already forked
	 * perhaps later commits will be spendable too */
	unsigned char spent;
	/* uncommitted program data */
	int mods;
};

struct Program {
	struct PData pdata[MAX_PROG_LEN];
	/* lengths of program */
	int len;
};

struct Program programs[2];

struct Process {
	/* owner of each process */
	unsigned char owner;
	/* defected processes */
	unsigned char def;
	/* program pointer for each proc */
	int pptrs;
	/* data pointer for each proc */
	int dptrs;
};

struct Process process[MAX_PROC_COUNT];

/* process count */
int procc;

/* verbose? */
unsigned char verbose = 0;
/* output turn numbers? */
unsigned char outt = 0;

#define CMD_NOP 0
#define CMD_INC 1
#define CMD_LFT 2
#define CMD_RHT 3
#define CMD_INP 4
#define CMD_OUT 5
#define CMD_COM 6 /* commit */
#define CMD_UNC 7 /* uncommit */
#define CMD_L1O 8 /* L1 = loop1 = [] = value at pointer != 0 */
#define CMD_L1C 9
#define CMD_L2O 10 /* L2 = loop2 = {} = enemy program pointer is NOT at data pointer */
#define CMD_L2C 11
#define CMD_L3O 12 /* L3 = loop3 = :; = fork */
#define CMD_L3C 13
#define CMD_BOM 14
#define CMD_OKB 15 /* "OK" bomb - for killing your own process, doesn't declare enemy winner */
#define CMD_DEF 16 /* defect - edit your own program */
#define CMD_CNT 17

/* not actually commands, just here to tell us when we're in comments */
#define CMD_CIN 18
#define CMD_COT 19

int cmdToInt(unsigned char cmd);
char intToCmd(int cmd);
/**
 * Executes a process and checks for a win-condition
 * @param procnum - The process to execute
 *
 * @returns - The winner of the current turn, 0 if it's still to be played
 */
int execcmd(int procnum);

int main(int argc, char **argv)
{
	FILE *fps[2];
	int prog, inc, proc;
	long turnn;

	if (argc <= 2) {
		fprintf(stderr,
"Copyright (c) 2005 Gregor Richards\n\n"
"Permission is hereby granted, free of charge, to any person obtaining a copy of\n"
"this software and associated documentation files (the \"Software\"), to deal in\n"
"the Software without restriction, including without limitation the rights to\n"
"use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies\n"
"of the Software, and to permit persons to whom the Software is furnished to do\n"
"so, subject to the following conditions:\n\n"
"The above copyright notice and this permission notice shall be included in all\n"
"copies or substantial portions of the Software.\n\n"
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n"
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS\n"
"FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR\n"
"COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER\n"
"IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN\n"
"CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\n\n\n");

		fprintf(stderr, "Use: %s <program 1> <program 2> [options]\n"
			"   Options:\n"
			"   v: verbose mode\n"
			"   t: output turn numbers\n", argv[0]);
		return 1;
	}

	if (argv[3]) {
		for (proc = 0; argv[3][proc]; proc++) {
			switch (argv[3][proc]) {
			case 'v':
				verbose = 1;
				break;

			case 't':
				outt = 1;
				break;
			}
		}
	}

	fps[0] = fopen(argv[1], "r");
	if (!fps[0]) {
		perror(argv[1]);
		return 1;
	}

	fps[1] = fopen(argv[2], "r");
	if (!fps[1]) {
		perror(argv[2]);
		return 1;
	}

	/* parse input */
	for (prog = 0; prog <= 1; prog++) {
		int incomment = 0;
		programs[prog].len = 0;

		int c = 0;
		while ((c = getc(fps[prog])) != EOF) {
			inc = cmdToInt((unsigned char)(c));

			if (inc == CMD_CIN) {
				incomment = 1;
				inc = -1;
			}
			if (inc == CMD_COT) {
				incomment = 0;
				inc = -1;
			}

			if (inc != -1 && !incomment) {
				programs[prog].pdata[programs[prog].len].data = inc;
				programs[prog].pdata[programs[prog].len].mods = inc;
				programs[prog].pdata[programs[prog].len].spent = 0;
				programs[prog].len++;
			}
		}

		fclose(fps[prog]);
	}

	/* the programs must be of equal length to be fair */
	while (programs[0].len < programs[1].len) {
		programs[0].pdata[programs[0].len].data = CMD_NOP;
		programs[0].pdata[programs[0].len].mods = CMD_NOP;
		programs[0].pdata[programs[0].len].spent = 0;
		programs[0].len++;
	}
	while (programs[1].len < programs[0].len) {
		programs[1].pdata[programs[1].len].data = CMD_NOP;
		programs[1].pdata[programs[1].len].mods = CMD_NOP;
		programs[1].pdata[programs[1].len].spent = 0;
		programs[1].len++;
	}

	/* now bootstrap the processes ... */
	process[0].owner = 1;
	process[0].def = 0;
	process[0].pptrs = 0;
	process[0].dptrs = 0;
	process[1].owner = 2;
	process[1].def = 0;
	process[1].pptrs = 0;
	process[1].dptrs = 0;
	procc = 2;

	for (turnn = 0; turnn < 10000000; turnn++) {
		int winner;
		unsigned char alive[2];

		/* output the board if we're verbose */
		if (verbose) {
			int vproc;
			char *outline;
			int outll;

			outline = (char *) malloc(MAX_PROG_LEN * sizeof(char));

			for (proc = 0; proc < 2; proc++) {

				fprintf(stderr, "==============================\n");
				fprintf(stderr, "%s (%d)\n\t", argv[proc + 1], proc+1);
				/* output sourcecode */
				for (inc = 0; inc < programs[0].len; inc++) {
					fprintf(stderr, "%c", intToCmd(programs[proc].pdata[inc].data));
				}
				fprintf(stderr, "\n");

				/* go through list of processes and ... TODO */
				for (vproc = 0; vproc < procc; vproc++) {
					/* position of the '\0' character
					 * terminating the outline */
					outll = 0;
					memset(outline, ' ', 32256);

					/* if current process is owned by current program */
					if (process[vproc].owner == proc + 1) {
						outline[process[vproc].pptrs] = 'p';
						if (process[vproc].pptrs >= outll) {
							outll = process[vproc].pptrs + 1;
						}

						if (process[vproc].def) {
							/* if process[vproc].pptrs == process[vproc].dptrs ? */
							if (outline[process[vproc].dptrs] == 'p') {
								outline[process[vproc].dptrs] = 'b';
							} else {
								outline[process[vproc].dptrs] = 'd';
							}
							if (process[vproc].dptrs >= outll) {
								outll = process[vproc].dptrs + 1;
							}
						}
					/* if the current process is NOT owned by the current program
					 * and also is NOT defected */
					} else if (process[vproc].owner != 0 && !process[vproc].def) {
						outline[process[vproc].dptrs] = 'd';
						if (process[vproc].dptrs >= outll) {
							outll = process[vproc].dptrs + 1;
						}
					}

					outline[outll] = '\0';
					fprintf(stderr, "%d:\t%s\n", vproc, outline);
				}
			}

			fprintf(stderr, "==============================\n\n");

			free(outline);
		} else if (outt) {
			fprintf(stderr, "%ld          \r", turnn);
		}

		for (proc = 0; proc < procc; proc++) {
			if (process[proc].owner) {
				if ((winner = execcmd(proc))) {
					if (outt) {
						fprintf(stderr, "\n");
					}
					printf("%s (%d) wins!\n", argv[winner], winner);
					return 0;
				}
			}
		}

		/* see if anybody accidentally quit themselves */
		alive[0] = 0;
		alive[1] = 0;
		for (proc = 0; proc < procc; proc++) {
			if (process[proc].owner) {
				alive[process[proc].owner-1] = 1;
			}
		}
		if (!alive[0]) {
			if (outt) {
				fprintf(stderr, "\n");
			}
			printf("%s wins!\n", argv[2]);
			return 0;
		}
		if (!alive[1]) {
			if (outt) {
				fprintf(stderr, "\n");
			}
			printf("%s wins!\n", argv[1]);
			return 0;
		}
	}

	printf("It's a draw due to 10,000,000 clocks with no winner!\n");

	return 0;
}

int cmdToInt(unsigned char cmd)
{
	switch (cmd) {
	case '%':
		return CMD_NOP;
	case '+':
		return CMD_INC;
	case '<':
		return CMD_LFT;
	case '>':
		return CMD_RHT;
	case ',':
		return CMD_INP;
	case '.':
		return CMD_OUT;
	case '!':
		return CMD_COM;
	case '?':
		return CMD_UNC;
	case '[':
		return CMD_L1O;
	case ']':
		return CMD_L1C;
	case '{':
		return CMD_L2O;
	case '}':
		return CMD_L2C;
	case ':':
		return CMD_L3O;
	case ';':
		return CMD_L3C;
	case '*':
		return CMD_OKB;
	case '@':
		return CMD_DEF;
	case '(':
		return CMD_CIN;
	case ')':
		return CMD_COT;
	}
	return -1;
}

char intToCmd(int cmd)
{
	switch (cmd) {
	case CMD_NOP:
		return '%';
	case CMD_INC:
		return '+';
	case CMD_LFT:
		return '<';
	case CMD_RHT:
		return '>';
	case CMD_INP:
		return ',';
	case CMD_OUT:
		return '.';
	case CMD_COM:
		return '!';
	case CMD_UNC:
		return '?';
	case CMD_L1O:
		return '[';
	case CMD_L1C:
		return ']';
	case CMD_L2O:
		return '{';
	case CMD_L2C:
		return '}';
	case CMD_L3O:
		return ':';
	case CMD_L3C:
		return ';';
	case CMD_BOM:
		return 'B';
	case CMD_OKB:
		return '*';
	case CMD_DEF:
		return '@';
	}
	return ' ';
}

int execcmd(int procnum)
{
	unsigned char procowner = process[procnum].owner;
	int *pptr = &process[procnum].pptrs;
	int *dptr = &process[procnum].dptrs;
	int i, fnd, origpptr;
	int procedits;
	int depth;

	if (process[procnum].def) {
		procedits = procowner - 1;
	} else {
		procedits = !(procowner - 1);
	}

	switch (programs[procowner-1].pdata[*pptr].data) {
	case CMD_INC:
		programs[procedits].pdata[*dptr].mods++;
		if (programs[procedits].pdata[*dptr].mods >= CMD_CNT) {
			programs[procedits].pdata[*dptr].mods = 0;
		}
		break;

	case CMD_LFT:
		(*dptr)--;
		if (*dptr < 0) {
			*dptr = 0;
		}
		break;
	case CMD_RHT:
		(*dptr)++;
		if (*dptr >= programs[0].len) {
			*dptr -= programs[0].len;
		}
		break;

	case CMD_INP:
	case CMD_OUT:
		/* no implementation, but reserved! */
		break;

	case CMD_COM:
		programs[procedits].pdata[*dptr].data = programs[procedits].pdata[*dptr].mods;
		/* if you commit a defect, you defect! */
		if (programs[procedits].pdata[*dptr].data == CMD_DEF) {
			process[procnum].def = !process[procnum].def;
			*dptr = *pptr;
		}
		/* perhaps make commit only work once? */
		break;
	case CMD_UNC:
		programs[procedits].pdata[*dptr].mods = programs[procedits].pdata[*dptr].data;
		break;

	case CMD_L1O:
		if (!programs[procedits].pdata[*dptr].mods) {
			/* find the counterpart */
			depth = 0;
			origpptr = *pptr;
			(*pptr)++;
			while (programs[procowner-1].pdata[*pptr].data != CMD_L1C ||
			       depth != 0) {
				if (programs[procowner-1].pdata[*pptr].data == CMD_L1O) {
					depth++;
				}
				if (programs[procowner-1].pdata[*pptr].data == CMD_L1C) {
					depth--;
				}
				(*pptr)++;

				if (*pptr >= programs[0].len) {
					/* our loop-end was evicerated! */
					*pptr = origpptr;
					break;
				}
			}
		}
		break;
	case CMD_L1C:
		/* find the counterpart */
		depth = 0;
		origpptr = *pptr;
		(*pptr)--;
		if (*pptr < 0) {
			*pptr = 0;
		}
		while (programs[procowner-1].pdata[*pptr].data != CMD_L1O ||
		       depth != 0) {
			if (programs[procowner-1].pdata[*pptr].data == CMD_L1O) {
				depth++;
			}
			if (programs[procowner-1].pdata[*pptr].data == CMD_L1C) {
				depth--;
			}
			(*pptr)--;

			if (*pptr < 0) {
				/* our loop-end was evicerated! */
				*pptr = origpptr + 1;
				break;
			}
		}
		(*pptr)--;
		break;

	case CMD_L2O:
		/* loop through every proc */
		fnd = 0;
		for (i = 0; i < procc; i++) {
			if (process[i].owner && process[i].owner != procowner) {
				if (process[i].pptrs == *dptr) {
					/* got em */
					fnd = 1;
				}
			}
		}
		if (fnd) {
			/* find the counterpart */
			depth = 0;
			origpptr = *pptr;
			(*pptr)++;
			while (programs[procowner-1].pdata[*pptr].data != CMD_L2C ||
			       depth != 0) {
				if (programs[procowner-1].pdata[*pptr].data == CMD_L2O) {
					depth++;
				}
				if (programs[procowner-1].pdata[*pptr].data == CMD_L2C) {
					depth--;
				}
				(*pptr)++;

				if (*pptr >= programs[0].len) {
					/* our loop-end was evicerated! */
					*pptr = origpptr;
					break;
				}
			}
		}
		break;
	case CMD_L2C:
		/* find the counterpart */
		depth = 0;
		origpptr = *pptr;
		(*pptr)--;
		if (*pptr < 0) {
			*pptr = 0;
		}
		while (programs[procowner-1].pdata[*pptr].data != CMD_L2O ||
		       depth != 0) {
			if (programs[procowner-1].pdata[*pptr].data == CMD_L2O) {
				depth++;
			}
			if (programs[procowner-1].pdata[*pptr].data == CMD_L2C) {
				depth--;
			}
			(*pptr)--;

			if (*pptr < 0) {
				/* our loop-end was evicerated! */
				*pptr = origpptr + 1;
				break;
			}
		}
		(*pptr)--;
		break;

	case CMD_L3O:
		/* start a new proc! */
		if (!programs[procowner-1].pdata[*pptr].spent) {
			process[procc].owner = procowner;
			process[procc].pptrs = *pptr + 1;
			process[procc].dptrs = *dptr;
			process[procc].def = process[procnum].def;

			procc++;

			programs[procowner-1].pdata[*pptr].spent = 1;
		}

		/* now this proc needs to skip to the end */
		/* find the counterpart */
		depth = 0;
		origpptr = *pptr;
		(*pptr)++;
		while (programs[procowner-1].pdata[*pptr].data != CMD_L3C ||
		       depth != 0) {
			if (programs[procowner-1].pdata[*pptr].data == CMD_L3O) {
				depth++;
			}
			if (programs[procowner-1].pdata[*pptr].data == CMD_L3C) {
				depth--;
			}
			(*pptr)++;

			if (*pptr >= programs[0].len) {
				/* our loop-end was evicerated! */
				*pptr = origpptr;
				break;
			}
		}

		break;
	case CMD_L3C:
		/* we must be in the child proc, skip to the beginning of this proc
		   (really, you shouldn't let a proc do this, you should bomb it */
		/* find the counterpart */
		depth = 0;
		origpptr = *pptr;
		(*pptr)--;
		if (*pptr < 0) {
			*pptr = 0;
		}
		while (programs[procowner-1].pdata[*pptr].data != CMD_L3O ||
		       depth != 0) {
			if (programs[procowner-1].pdata[*pptr].data == CMD_L3O) {
				depth++;
			}
			if (programs[procowner-1].pdata[*pptr].data == CMD_L3C) {
				depth--;
			}
			(*pptr)--;

			if (*pptr < 0) {
				/* our loop-end was evicerated! */
				*pptr = origpptr + 1;
				break;
			}
		}
		break;

	case CMD_BOM:
		/* boom! */
		return ((!(procowner-1)) + 1);
	case CMD_OKB:
		/* sorta boom! */
		if (procnum < 2) {
			process[procnum].owner = 0;
		} else {
			/* only bomb if it's the last command of a thread */
			if (programs[procowner-1].pdata[(*pptr)+1].data == CMD_L3C) {
				process[procnum].owner = 0;
			}
		}
		break;

	case CMD_DEF:
		/* defect - edit your own program */
		process[procnum].def = !process[procnum].def;
		*dptr = *pptr;
		break;
	}


	(*pptr)++;
	if (*pptr >= programs[0].len) {
		*pptr = 0;
	}

	return 0;
}
