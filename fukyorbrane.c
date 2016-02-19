/*
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
#include <unistd.h>

#ifndef MAX_PROG_LEN
#define MAX_PROG_LEN 32256
#endif

/* program data */
int *progs[2];
/* a command is "spent" if it's a fork that's already forked 
 * perhaps later commits will be spendable too */
unsigned char *progSpent[2];
/* uncommitted program data */
int *progMods[2];
/* lengths of each program */
int proglens[2];
/* owner of each process */
unsigned char *procs;
/* defected processes */
unsigned char *procdef;
/* program pointer for each proc */
int *procpptrs;
/* data pointer for each proc */
int *procdptrs;
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

int cmdToInt(char cmd);
char intToCmd(int cmd);
char intToCmd(int cmd);
int execcmd(int procnum, unsigned char procowner, int *pptr, int *dptr);

int main(int argc, char **argv)
{
    FILE *fps[2];
    int prog, inc, proc;
    long turnn;
    
    if (argc <= 2) {
        fprintf(stderr, "\n\nCopyright (c) 2005 Gregor Richards\n\n"
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
    
    progs[0] = (int *) malloc(MAX_PROG_LEN * sizeof(int));
    progs[1] = (int *) malloc(MAX_PROG_LEN * sizeof(int));
    progSpent[0] = (unsigned char *) malloc(MAX_PROG_LEN * sizeof(unsigned char));
    progSpent[1] = (unsigned char *) malloc(MAX_PROG_LEN * sizeof(unsigned char));
    progMods[0] = (int *) malloc(MAX_PROG_LEN * sizeof(int));
    progMods[1] = (int *) malloc(MAX_PROG_LEN * sizeof(int));
    procs = (unsigned char *) malloc(1024 * sizeof(unsigned char));
    procdef = (unsigned char *) malloc(1024 * sizeof(unsigned char));
    procpptrs = (int *) malloc(1024 * sizeof(int));
    procdptrs = (int *) malloc(1024 * sizeof(int));

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
    
    for (prog = 0; prog <= 1; prog++) {
        int incomment = 0;
        proglens[prog] = 0;
        
        while (!feof(fps[prog])) {
            inc = cmdToInt(getc(fps[prog]));
            if (inc == CMD_CIN) {
                incomment = 1;
                inc = -1;
            }
            if (inc == CMD_COT) {
                incomment = 0;
                inc = -1;
            }
            
            if (inc != -1 && !incomment) {
                progs[prog][proglens[prog]] = inc;
                progMods[prog][proglens[prog]] = inc;
                progSpent[prog][proglens[prog]] = 0;
                proglens[prog]++;
            }
        }
    }
    
    /* the programs must be of equal length to be fair */
    while (proglens[0] < proglens[1]) {
        progs[0][proglens[0]] = CMD_NOP;
        progMods[0][proglens[0]] = CMD_NOP;
        progSpent[0][proglens[0]] = 0;
        proglens[0]++;
    }
    while (proglens[1] < proglens[0]) {
        progs[1][proglens[1]] = CMD_NOP;
        progMods[1][proglens[1]] = CMD_NOP;
        progSpent[1][proglens[1]] = 0;
        proglens[1]++;
    }
    
    /* now bootstrap the processes ... */
    procs[0] = 1;
    procdef[0] = 0;
    procpptrs[0] = 0;
    procdptrs[0] = 0;
    procs[1] = 2;
    procdef[1] = 0;
    procpptrs[1] = 0;
    procdptrs[1] = 0;
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
                fprintf(stderr, "%s\n\t", argv[proc + 1]);
                for (inc = 0; inc < proglens[0]; inc++) {
                    fprintf(stderr, "%c", intToCmd(progs[proc][inc]));
                }
                fprintf(stderr, "\n");
                
                for (vproc = 0; vproc < procc; vproc++) {
                    outll = 0;
                    memset(outline, ' ', 32256);
                    
                    if (procs[vproc] == proc + 1) {
                        outline[procpptrs[vproc]] = 'p';
                        if (procpptrs[vproc] >= outll)
                            outll = procpptrs[vproc] + 1;
                        
                        if (procdef[vproc]) {
                            if (outline[procdptrs[vproc]] == 'p') {
                                outline[procdptrs[vproc]] = 'b';
                            } else {
                                outline[procdptrs[vproc]] = 'd';
                            }
                            if (procdptrs[vproc] >= outll)
                                outll = procdptrs[vproc] + 1;
                        }
                    } else if (procs[vproc] != 0 && !procdef[vproc]) {
                        outline[procdptrs[vproc]] = 'd';
                        if (procdptrs[vproc] >= outll)
                            outll = procdptrs[vproc] + 1;
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
            if (procs[proc]) {
                if ((winner = execcmd(proc, procs[proc], procpptrs + proc, procdptrs + proc))) {
                    if (outt) fprintf(stderr, "\n");
                    printf("%s wins!\n", argv[winner]);
                    return 0;
                }
            }
        }
        
        // see if anybody accidentally quit themselves
        alive[0] = 0;
        alive[1] = 0;
        for (proc = 0; proc < procc; proc++) {
            if (procs[proc]) {
                alive[procs[proc]-1] = 1;
            }
        }
        if (!alive[0]) {
            if (outt) fprintf(stderr, "\n");
            printf("%s wins!\n", argv[2]);
            return 0;
        }
        if (!alive[1]) {
            if (outt) fprintf(stderr, "\n");
            printf("%s wins!\n", argv[1]);
            return 0;
        }
    }
    
    printf("It's a draw due to 10,000,000 clocks with no winner!\n");
    
    return 0;
}

int cmdToInt(char cmd)
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

int execcmd(int procnum, unsigned char procowner, int *pptr, int *dptr)
{
    int i, fnd, origpptr;
    int procedits;
    int depth;
    
    if (procdef[procnum]) {
        procedits = procowner - 1;
    } else {
        procedits = !(procowner - 1);
    }

    switch (progs[procowner-1][*pptr]) {
        case CMD_INC:
            progMods[procedits][*dptr]++;
            if (progMods[procedits][*dptr] >= CMD_CNT) progMods[procedits][*dptr] = 0;
            break;

        case CMD_LFT:
            (*dptr)--;
            if (*dptr < 0) *dptr = 0;
            break;
        case CMD_RHT:
            (*dptr)++;
            if (*dptr >= proglens[0]) *dptr -= proglens[0];
            break;
        
        case CMD_INP:
        case CMD_OUT:
            /* no implementation, but reserved! */
            break;
        
        case CMD_COM:
            progs[procedits][*dptr] = progMods[procedits][*dptr];
            // if you commit a defect, you defect!
            if (progs[procedits][*dptr] == CMD_DEF) {
                procdef[procnum] = !procdef[procnum];
                *dptr = *pptr;
            }
            /* perhaps make commit only work once? */
            break;
            
        case CMD_UNC:
            progMods[procedits][*dptr] = progs[procedits][*dptr];
            break;
        
        case CMD_L1O:
            if (!progMods[procedits][*dptr]) {
                /* find the counterpart */
                depth = 0;
                origpptr = *pptr;
                (*pptr)++;
                while (progs[procowner-1][*pptr] != CMD_L1C ||
                       depth != 0) {
                    if (progs[procowner-1][*pptr] == CMD_L1O) depth++;
                    if (progs[procowner-1][*pptr] == CMD_L1C) depth--;
                    (*pptr)++;
                    
                    if (*pptr >= proglens[0]) {
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
            if (*pptr < 0) *pptr = 0;
            while (progs[procowner-1][*pptr] != CMD_L1O ||
                   depth != 0) {
                if (progs[procowner-1][*pptr] == CMD_L1O) depth++;
                if (progs[procowner-1][*pptr] == CMD_L1C) depth--;
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
                if (procs[i] && procs[i] != procowner) {
                    if (procpptrs[i] == *dptr) {
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
                while (progs[procowner-1][*pptr] != CMD_L2C ||
                       depth != 0) {
                    if (progs[procowner-1][*pptr] == CMD_L2O) depth++;
                    if (progs[procowner-1][*pptr] == CMD_L2C) depth--;
                    (*pptr)++;

                    if (*pptr >= proglens[0]) {
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
            if (*pptr < 0) *pptr = 0;
            while (progs[procowner-1][*pptr] != CMD_L2O ||
                   depth != 0) {
                if (progs[procowner-1][*pptr] == CMD_L2O) depth++;
                if (progs[procowner-1][*pptr] == CMD_L2C) depth--;
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
            if (!progSpent[procnum][*pptr]) {
                procs[procc] = procowner;
                procpptrs[procc] = *pptr + 1;
                procdptrs[procc] = *dptr;
                procdef[procc] = procdef[procnum];
                
                procc++;
                
                progSpent[procnum][*pptr] = 1;
            }
            
            /* now this proc needs to skip to the end */
            /* find the counterpart */
            depth = 0;
            origpptr = *pptr;
            (*pptr)++;
            while (progs[procowner-1][*pptr] != CMD_L3C ||
                   depth != 0) {
                if (progs[procowner-1][*pptr] == CMD_L3O) depth++;
                if (progs[procowner-1][*pptr] == CMD_L3C) depth--;
                (*pptr)++;
                
                if (*pptr >= proglens[0]) {
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
            if (*pptr < 0) *pptr = 0;
            while (progs[procowner-1][*pptr] != CMD_L3O ||
                   depth != 0) {
                if (progs[procowner-1][*pptr] == CMD_L3O) depth++;
                if (progs[procowner-1][*pptr] == CMD_L3C) depth--;
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
                procs[procnum] = 0;
            } else {
                /* only bomb if it's the last command of a thread */
                if (progs[procowner][(*pptr)+1] == CMD_L3C) {
                    procs[procnum] = 0;
                }
            }
            break;
            
        case CMD_DEF:
            /* defect - edit your own program */
            procdef[procnum] = !procdef[procnum];
            *dptr = *pptr;
            break;
    }
    
    (*pptr)++;
    if (*pptr >= proglens[0]) *pptr = 0;
    
    return 0;
}
