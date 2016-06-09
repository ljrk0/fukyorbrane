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
#include <stdbool.h>

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
	bool spent;
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
	struct Program *owner;
	/* defected processes */
	bool def;
	/* program pointer for each proc */
	int pptrs;
	/* data pointer for each proc */
	int dptrs;
};

struct Process processes[MAX_PROC_COUNT];

/* process count */
int procc;

#define CMD_NOP 0
#define CMD_INC 1
#define CMD_LFT 2
#define CMD_RHT 3
#define CMD_INP 4
#define CMD_OUT 5
#define CMD_COM 6 /* commit */
#define CMD_UNC 7 /* uncommit */
#define CMD_L1O 8 /* "Loop #1"; []-Loop: Value at data-pointer != 0 */
#define CMD_L1C 9
#define CMD_L2O 10 /* {}-Loop: Enemy program pointer is NOT at data pointer */
#define CMD_L2C 11
#define CMD_L3O 12 /* :;-Loop: Fork */
#define CMD_L3C 13
#define CMD_BOM 14
#define CMD_OKB 15 /* "OK bomb"; Kill own process w/o declaring enemy winner */
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
	long turnn;

	if (argc <= 2) {
		fprintf(stderr,
"%s (FukYorBrane)\n"
"Copyright (c) 2016 Leonard König\n"
"Copyright (c) 2005 Gregor Richards\n"
"This is free software; see the source for copying conditions.  There is NO\n"
"warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE."
"\n\n",
argv[0]);

		fprintf(stderr, "Use: %s <program 1> <program 2> [options]\n"
			"   Options:\n"
			"   v: verbose mode\n"
			"   t: output turn numbers\n", argv[0]);
		return 1;
	}


	/* verbose? */
	unsigned char verbose = 0;
	/* output turn numbers? */
	unsigned char outt = 0;

	if (argv[3]) {
		for (int i = 0; argv[3][i]; i++) {
			switch (argv[3][i]) {
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
	for (int prog = 0; prog <= 1; prog++) {
		int incomment = 0;
		programs[prog].len = 0;

		int c = 0;
		while ((c = getc(fps[prog])) != EOF) {
			int cmd = cmdToInt((unsigned char)(c));
			/* TODO: possibly better error-handling? */
			if (cmd == -1) continue;

			if (cmd == CMD_CIN) {
				incomment = 1;
				continue;
			}
			if (cmd == CMD_COT) {
				incomment = 0;
				continue;
			}

			if (incomment) continue;

			programs[prog].pdata[programs[prog].len].data = cmd;
			programs[prog].pdata[programs[prog].len].mods = cmd;
			programs[prog].pdata[programs[prog].len].spent = false;
			programs[prog].len++;
		}

		fclose(fps[prog]);
	}

	/* the programs must be of equal length to be fair */
	while (programs[0].len < programs[1].len) {
		programs[0].pdata[programs[0].len].data = CMD_NOP;
		programs[0].pdata[programs[0].len].mods = CMD_NOP;
		programs[0].pdata[programs[0].len].spent = false;
		programs[0].len++;
	}
	while (programs[1].len < programs[0].len) {
		programs[1].pdata[programs[1].len].data = CMD_NOP;
		programs[1].pdata[programs[1].len].mods = CMD_NOP;
		programs[1].pdata[programs[1].len].spent = false;
		programs[1].len++;
	}

	/* now bootstrap the processes ... */
	processes[0].owner = &programs[0];
	processes[0].def = false;
	processes[0].pptrs = 0;
	processes[0].dptrs = 0;
	processes[1].owner = &programs[1];
	processes[1].def = false;
	processes[1].pptrs = 0;
	processes[1].dptrs = 0;
	procc = 2;

	for (turnn = 0; turnn < 10000000; turnn++) {
		int winner;
		unsigned char alive[2];

		/* output the board if we're verbose */
		if (verbose) {
			char outline[MAX_PROG_LEN];

			for (int prog = 0; prog < 2; prog++) {

				fprintf(stderr, "==============================\n");
				fprintf(stderr, "%s (%d)\n\t", argv[prog + 1], prog+1);
				/* output sourcecode */
				for (int cmd = 0; cmd < programs[0].len; cmd++) {
					fprintf(stderr, "%c", intToCmd(programs[prog].pdata[cmd].data));
				}
				fprintf(stderr, "\n");

				/**
				 * go through list of processes and print
				 * position of data & program pointers
				 */
				for (int vproc = 0; vproc < procc; vproc++) {
					/* position of the '\0' character
					 * terminating the outline */
					int outll = 0;
					memset(outline, ' ', 32256);

					/* if current process is owned by current program */
					if (processes[vproc].owner == &programs[prog]) {
						outline[processes[vproc].pptrs] = 'p';
						if (processes[vproc].pptrs >= outll) {
							outll = processes[vproc].pptrs + 1;
						}

						if (processes[vproc].def) {
							/* if processes[vproc].pptrs == processes[vproc].dptrs ? */
							if (outline[processes[vproc].dptrs] == 'p') {
								outline[processes[vproc].dptrs] = 'b';
							} else {
								outline[processes[vproc].dptrs] = 'd';
							}
							if (processes[vproc].dptrs >= outll) {
								outll = processes[vproc].dptrs + 1;
							}
						}
					/* if the current process is NOT owned by the current program
					 * and also is NOT defected */
					} else if (processes[vproc].owner != NULL && !processes[vproc].def) {
						outline[processes[vproc].dptrs] = 'd';
						if (processes[vproc].dptrs >= outll) {
							outll = processes[vproc].dptrs + 1;
						}
					}

					outline[outll] = '\0';
					fprintf(stderr, "%d:\t%s\n", vproc, outline);
				}
			}

			fprintf(stderr, "==============================\n\n");
		} else if (outt) {
			fprintf(stderr, "%ld          \r", turnn);
		}

		for (int proc = 0; proc < procc; proc++) {
			if (processes[proc].owner != NULL) {
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
		for (int proc = 0; proc < procc; proc++) {
			if (processes[proc].owner == &programs[0]) {
				alive[0] = 1;
			} else if (processes[proc].owner == &programs[1]) {
				alive[1] = 1;
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
	struct Program *procowner = processes[procnum].owner;
	int *pptr = &processes[procnum].pptrs;
	int *dptr = &processes[procnum].dptrs;
	int i, fnd, origpptr;
	struct Program *procedits;
	int depth;

	/**
	 * if the process is defected, edit the own program, otherwise
	 * edit enemy program
	 */
	if (processes[procnum].def) {
		procedits = procowner;
	} else {
		if (procowner == &programs[0])
			procedits = &programs[1];
		else
			procedits = &programs[0];
	}

	switch (procowner->pdata[*pptr].data) {
	case CMD_INC:
		/**
		 * increase the modified instruction, loop if necessary
		 */
		procedits->pdata[*dptr].mods++;
		if (procedits->pdata[*dptr].mods >= CMD_CNT) {
			procedits->pdata[*dptr].mods = 0;
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
		procedits->pdata[*dptr].data
			= procedits->pdata[*dptr].mods;
		/* if you commit a defect, you defect! */
		if (procedits->pdata[*dptr].data == CMD_DEF) {
			processes[procnum].def = !processes[procnum].def;
			*dptr = *pptr;
		}
		/* perhaps make commit only work once? */
		break;
	case CMD_UNC:
		procedits->pdata[*dptr].mods
			= procedits->pdata[*dptr].data;
		break;

	case CMD_L1O:
		/**
		 * As long as pointer on enemy program doesn't point to NOP
		 * continue (ie. loop). Otherwise try to find the counterpart
		 * and just continue from there.
		 *
		 * If there's none, just ignore the opened loop.
		 */
		if (procedits->pdata[*dptr].mods) break;
		/* find the counterpart */
		depth = 0;
		origpptr = *pptr;
		(*pptr)++;
		while (procowner->pdata[*pptr].data != CMD_L1C
				|| depth != 0) {
			if (procowner->pdata[*pptr].data
					== CMD_L1O) {
				depth++;
			}
			if (procowner->pdata[*pptr].data
					== CMD_L1C) {
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
	case CMD_L1C:
		/* find the counterpart */
		depth = 0;
		origpptr = *pptr;
		(*pptr)--;
		if (*pptr < 0) {
			*pptr = 0;
		}
		while (procowner->pdata[*pptr].data != CMD_L1O
				|| depth != 0) {
			if (procowner->pdata[*pptr].data
					== CMD_L1O) {
				depth++;
			}
			if (procowner->pdata[*pptr].data
					== CMD_L1C) {
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
		/**
		 * loop through every process and try to find one of the enemy
		 * whiches program pointer is on the same position as ones own
		 * data pointer
		 */
		fnd = 0;
		for (i = 0; i < procc; i++) {
			if (processes[i].owner
					&& processes[i].owner != procowner) {
				if (processes[i].pptrs == *dptr) {
					/* got em */
					fnd = 1;
				}
			}
		}
		if (!fnd) break;

		/* find the counterpart */
		depth = 0;
		origpptr = *pptr;
		(*pptr)++;
		while (procowner->pdata[*pptr].data != CMD_L2C
				|| depth != 0) {
			if (procowner->pdata[*pptr].data
					== CMD_L2O) {
				depth++;
			}
			if (procowner->pdata[*pptr].data
					== CMD_L2C) {
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
	case CMD_L2C:
		/* find the counterpart */
		depth = 0;
		origpptr = *pptr;
		(*pptr)--;
		if (*pptr < 0) {
			*pptr = 0;
		}
		while (procowner->pdata[*pptr].data != CMD_L2O
				|| depth != 0) {
			if (procowner->pdata[*pptr].data
					== CMD_L2O) {
				depth++;
			}
			if (procowner->pdata[*pptr].data
					== CMD_L2C) {
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
		/* start a new process! */
		if (!procowner->pdata[*pptr].spent) {
			processes[procc].owner = procowner;
			processes[procc].pptrs = *pptr + 1;
			processes[procc].dptrs = *dptr;
			processes[procc].def = processes[procnum].def;

			procc++;

			procowner->pdata[*pptr].spent = true;
		}

		/* now this process needs to skip to the end */
		/* find the counterpart */
		depth = 0;
		origpptr = *pptr;
		(*pptr)++;
		while (procowner->pdata[*pptr].data != CMD_L3C ||
		       depth != 0) {
			if (procowner->pdata[*pptr].data == CMD_L3O) {
				depth++;
			}
			if (procowner->pdata[*pptr].data == CMD_L3C) {
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
		/**
		 * We must be in the child process, skip to the beginning of
		 * this process (really, you shouldn't let a process do this,
		 * you should bomb it)
		 */

		/* find the counterpart */
		depth = 0;
		origpptr = *pptr;
		(*pptr)--;
		if (*pptr < 0) {
			*pptr = 0;
		}
		while (procowner->pdata[*pptr].data != CMD_L3O
				|| depth != 0) {
			if (procowner->pdata[*pptr].data
					== CMD_L3O) {
				depth++;
			}
			if (procowner->pdata[*pptr].data
					== CMD_L3C) {
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
		return (procowner == &programs[0]) ? 2 : 1;
	case CMD_OKB:
		/**
		 * sorta boom! Kill the current process.
		 */
		if (procnum < 2) {
			processes[procnum].owner = 0;
		} else if (procowner->pdata[(*pptr)+1].data
					== CMD_L3C) {
			/* only bomb if it's the last command of a thread */
			processes[procnum].owner = 0;
		}
		break;

	case CMD_DEF:
		/* defect - edit your own program */
		processes[procnum].def = !processes[procnum].def;
		*dptr = *pptr;
		break;
	}


	(*pptr)++;
	if (*pptr >= programs[0].len) {
		*pptr = 0;
	}

	return 0;
}
