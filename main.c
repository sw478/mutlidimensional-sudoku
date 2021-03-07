#include "main.h"
#include "auxil.h"
#include "setup.h"
#include "sew.h"
#include "heuristic.h"
#include "end.h"
#include "error.h"
#include "hide.h"
#include "shuffle.h"
#include "setMatrixDimensions.h"
#include "hrowCover.h"
#include "initHrowLayout.h"

/*
 * right now generate option generates full boards, will change to have
 * empty slot to be solvable
 *
 * a filled text file should only contain positive integers, 0 denoting an
 * unknown space
 *
 * If a textfile contains less characters than required, a message will
 * display. Any extra characters will be ignored
 *
 */
int main(int argc, char *argv[])
{
   Dance *d = malloc(sizeof(Dance));
   srand(time(NULL));
   parseArgs(d, argc, argv);

   switch(d->problem)
   {
      case 0: runSudoku(d, argc, argv); break;
      case 1: runSudoku2(d, argc, argv); break;
      case 2: runNQueens(d, argc, argv); break;
      default: break;
   }
}

int runSudoku(Dance *d, int argc, char *argv[])
{
   int mode;
   char *matrixFile = malloc(BUFSIZE*sizeof(char));

   mode = d->s->mode;

   if(mode == 1) /* if solving */
      printBoard(d, d->s->grid);

   //initMatrixFileSudoku(d); /* using matrixFileCreator.py probably easier */
   sprintf(matrixFile, "dance/ds1_%dx%d.txt", d->s->y, d->s->x);
   d->init = fopen(matrixFile, "r+");
   free(matrixFile);

   setMatrixDimensions_Sudoku(d);

   initDance(d); /* initialize dance struct */
   initMatrix(d); /* reads from d->init and creates the general matrix for the given dimensions */
   printf("finished matrix\n"); /*for larger boards everything prior takes a small but noticeable amount of time */
   
   printMatrix(d);

   initHeurList(d, d->rmax / d->s->xy); /* initializes the heuristic helper structure */
   printf("finished heur\n");

   if(mode == 1)
   {
      initHide_Sudoku(d);
      hideAllCells(d); /* if solving, hides the necessary rows in the matrix to define the puzzle, reading from sudoku file */
   }
   printf("finished hide\n");
   
   coverRowHeaders(d); /* cover all row headers, necessary for program to work */
   printf("finished cover\n");

   algorithmX(d);

   printf("number of calls: %d\n", d->numCalls);
   uncoverRowHeaders(d); /* handles memory allocated from coverRowHeaders */

   if(mode == 1)
      unhideAllCells(d); /* handles memory */
   printSolutions_Sudoku(d);
   saveSolution_Sudoku(d); /* translates solTrie matrix rows to sudoku solution */

   if(mode == 2)
   {
      initHide_Sudoku(d);
      printf("finished initHide\n");
      hideAllCells(d);

      saveGeneratedPuzzle(d);
      printBoard(d, d->s->grid);

      shuffle(d);
      printBoard(d, d->s->grid);
      unhideAllCells(d);
   }

   freeDance(d);

   return 0;
}

/* using layouts */
int runSudoku2(Dance *d, int argc, char *argv[])
{
   char *matrixFile = malloc(BUFSIZE*sizeof(char));

   if(d->s->mode == 2)
   {
      fprintf(stderr, "Can't generate puzzles with layout setup\n");
      error();
   }

   printBoard(d, d->s->grid);
 
   /* can set to custom matrixFile here */
   sprintf(matrixFile, "dance/ds2_%dx%d.txt", d->s->y, d->s->x);
   d->init = fopen(matrixFile, "r+");
   free(matrixFile);

   setMatrixDimensions_Sudoku2(d);

   initDance(d); /* initialize dance struct */
   initMatrix(d); /* reads from d->init and creates the general matrix for the given dimensions */
   printf("finished matrix\n"); /*for larger boards everything prior takes a small but noticeable amount of time */

   initHeurList(d, d->rmax / d->s->xy); /* initializes the heuristic helper structure */
   printf("finished heur\n");
   
   //printHeur(d);
   //printMatrix(d);
   initHrowLayout(d);
   //printHrowLayout(d);
   printf("finished hrow layout\n");

   hide_Sudoku2(d);
   printf("finished hide\n");

   //printMatrix(d);
   
   coverRowHeaders(d); /* cover all row headers, necessary for program to work */
   printf("finished cover\n");

   algorithmX(d);

   printf("number of calls: %d\n", d->numCalls);
   uncoverRowHeaders(d); /* handles memory allocated from coverRowHeaders */
   unhide_Sudoku2(d);

   printSolutions_Sudoku2(d);
   saveSolution_Sudoku2(d);

   freeDance(d);

   return 0;
}

int runNQueens(Dance *d, int argc, char *argv[])
{
   char *matrixFile = malloc(BUFSIZE*sizeof(char));
 
   /* can set to custom matrixFile here */
   sprintf(matrixFile, "dance/dq_%d.txt", d->nq);
   d->init = fopen(matrixFile, "r+");
   free(matrixFile);

   setMatrixDimensions_NQueens(d);

   initDance(d); /* initialize dance struct */
   initMatrix(d); /* reads from d->init and creates the general matrix for the given dimensions */
   printf("finished matrix\n"); /*for larger boards everything prior takes a small but noticeable amount of time */

   stitch_secondary(d, d->nq*2);
   printf("finished stitching secondary\n");

   initHeurList(d, d->rmax / d->s->xy); /* initializes the heuristic helper structure */
   printf("finished heur\n");
   
   coverRowHeaders(d); /* cover all row headers, necessary for program to work */
   printf("finished cover\n");

   algorithmX(d);

   printf("number of calls: %d\n", d->numCalls);
   uncoverRowHeaders(d); /* handles memory allocated from coverRowHeaders */

   unstitch_secondary(d);

   printSolutions_NQueens(d);

   freeDance(d);

   return 0;
}

/*
 * argument format: a.out [problem (s/s2/q)] ...
 * 
 * sudoku/sudoku2:
 * a.out [s/s2] [mode: s for solve, g for gen] [file: empty to be
 * filled with only dimensions if generating, filled if solving]
 * 
 * n queens:
 * a.out [q] [n]
 */
void parseArgs(Dance *d, int argc, char *argv[])
{
   int i, c, test;
   Sudoku *s;
   char *buf = malloc(BUFSIZE*sizeof(char)), temp;

   memset(buf, 0, BUFSIZE*sizeof(char));

   if(argc < 1)
      numArgError();
   
   if(!strcmp(argv[1], "s"))
      d->problem = 0;
   else if(!strcmp(argv[1], "s2"))
      d->problem = 1;
   else if(!strcmp(argv[1], "q"))
      d->problem = 2;

   if(d->problem == 0 || d->problem == 1)
   {
      s = malloc(sizeof(Sudoku));
      if(argc != 4)
         numArgError();
      if(!strcmp(argv[2], "s")) /* solve */
         s->mode = 1;
      else if(!strcmp(argv[2], "g")) /* generate */
         s->mode = 2;
      else
         usage();
      if(s->mode == 1)
      {
         s->in = fopen(argv[3], "r+");
         if(!s->in)
            fileError(argv[3]);
      }
      if(s->mode == 2)
         s->in = fopen(argv[3], "w+");

      assert(3 == sscanf(argv[3], "tests/%c/%dx%d.in", &temp, &s->x, &s->y));
      s->xy = s->x*s->y;
      s->gridSize = s->xy*s->xy;
      s->grid = calloc(s->gridSize, sizeof(int));

      assert(fseek(s->in, 0, SEEK_SET) == 0);
      if(s->mode == 2)
      {
         sprintf(buf, "%d %d\n", s->x, s->y);
         fwrite(buf, 1, BUFSIZE*sizeof(char), s->in);
         free(buf);
         return;
      }

      fgets(buf, BUFSIZE*sizeof(char), s->in);
      for(i = 0; i < s->gridSize; i++)
      {
         fgets(buf, BUFSIZE, s->in);
         test = sscanf(buf, "%d", &c);
         if(test < 1 || c < 0 || c > s->xy)
            invalidInput();
         s->grid[i] = c;
      }
      d->s = s;
   }
   else if(d->problem == 2)
   {
      if(argc != 3){
         printf("argc: %d\n", argc);
         numArgError();}
      if(1 != sscanf(argv[2], "%d", &d->nq))
         invalidN();
   }
   
   free(buf);
}