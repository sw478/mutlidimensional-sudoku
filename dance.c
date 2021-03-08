#include "dance.h"
#include "solTree.h"
#include "auxil.h"
#include "heuristic.h"
#define FOUND 1
#define NOT_FOUND 0

int algorithmX(Dance *d)
{
   Doubly *hcol, *candidateRow;
   /*
      res will be returned, so if there is at least one solution,
      res must be set to FOUND
      ret is just what algX returns from one candidate row, and is
      used for knowing whether the level below found at least one
      solution
   */
   int res = NOT_FOUND, ret;

   /* used to randomize row picking */
   int listSize, *hitList;

   /* storing information for the upper level */
   int solCreated = 0;
   SolTree *sol;

   /*
      matrix is empty, a solution has been found
      adds to the list of solutions
   */
   if(d->root == d->root->right)
   {
      d->csol = initTree();
      addLeaf(d, d->csol);
      return FOUND;
   }

   /* keeps track of non-trivial calls */
   d->numCalls++;

   /*
      chooses an hcol with a minimum number of
      elements beneath
   */
   hcol = USE_HEUR ? heuristic(d) : heuristic2(d);
//printHeur(d); /* used to show status of the heurs */

   /*
      there are no possible solution if there are still
      columns to fill, but no doubly left in the matrix
   */
   if(hcol == d->root)
      return NOT_FOUND;
   
   if(RANDOMIZE_ROWS)
   {
      listSize = hcol->drow - d->rmax;
      hitList = calloc(listSize, sizeof(int));
      candidateRow = nextRow(hcol, &listSize, &hitList);
   }
   else
      candidateRow = hcol->down;
   
   while(candidateRow != hcol)
   {
      /*
         calling the pairs of printMatrix before and after
         covering necessary doubly allows you to see the
         algorithm in work
      */
//printMatrix(d);
      selectCandidateRow(d, candidateRow);

      ret = algorithmX(d);
      res |= ret;

//printMatrix(d);
      unselectCandidateRow(d, candidateRow);

      /*
         sol will refer to the candidate row above this
         algX call, note that sol->row isn't assigned yet
      */
      if(ret == FOUND)
      {
         /*
            if a solution has been found below, d->csol
            will contain a solTree refering to that with
            everything but the row assigned, and it's at
            this level you assign the row to it
         */
         d->csol->row = candidateRow->hrow;
         /* malloc sol if haven't already */
         if(solCreated == 0)
         {
            solCreated = 1;
            sol = initTree();
         }
         /* add solTree from below (d->csol) as child to sol */
         addChild(sol, d->csol);
         d->csol = sol;
      }
      if(res == FOUND)
      {
         /* if you want to stop at the first solution found,
         break here */
         if(d->problem == SUDOKU && d->s->mode == 2) /* generating mode */
            break;
      }

      if(RANDOMIZE_ROWS)
         candidateRow = nextRow(hcol, &listSize, &hitList);
      else
         candidateRow = candidateRow->down;
   }

   if(RANDOMIZE_ROWS)
      free(hitList);
   return res;
}

/*
 * used to randomize order of candidate rows searched in column chosen
 * in AlgX, otherwise algX will just go down the column
 * 
 * keeps track of rows and records which have already been visited
 * hitList[i] = 0 if not visited, 1 if visited
 * 
 * set RANDOMIZE_ROWS to 1 to use
 */
Doubly *nextRow(Doubly *hcol, int *listSize, int **hitList)
{
   Doubly *row;
   int i, j, randInt;
   if(*listSize == 0)
      return hcol;

   randInt = rand() % *listSize;
   /* go to a random row */
   for(row = hcol->down, i = 0; i < randInt; i++, row = row->down);
   /* continue going down row until unvisited row is found */
   for(j = i; (*hitList)[j] == 1; j++, row = row->down)
   {
      /* skip if row is column header row */
      if(row == hcol)
         row = row->down;
   }

   (*listSize)--;
   (*hitList)[j] = 1;
   return row;
}

/*
   if candidateRow is row of a solution, we have to cover
   all doubly that conflict with it being chosen

   candidateRow is just one doubly in the candidate row,
   coverColRows() is called on all doubly in the row, including
   starting doubly

      0  1  2  3  4  5  6
      
   0  _  X  X  _  _  X  _

   1  _  X  _  X  _  _  X

   2  X  _  _  _  X  _  _

   ex. row 1 is the candidate row.
   row 0 cannot be part of the solution too because both have a doubly in col 1

   in the example, coverColRows() is called on (1, 1), (1, 3), and (1, 6)
*/
void selectCandidateRow(Dance *d, Doubly *candidateRow)
{
   Doubly *crow;

   /* crow loops through all elements in the candidate row */
   for(crow = candidateRow->right; crow != candidateRow; crow = crow->right)
      coverColRows(d, crow);
   coverColRows(d, candidateRow);
}

/*
   cover all rows in given column

   coverColRows() is called on (1, 1), so this searches for all other rows
   that have a doubly in column 1, and covers those rows

   it sees (0, 1), so coverRows() is called on (0, 1) and all doubly in
   row 0 are covered

   after call, resulting matrix will "look" like this:

      0  2  4  5

   2  X  _  X  _

   row 1 hidden since it was a chosen candidate row, and row 0 hidden
   since it conflicted with row 1

   columns 1, 3, 6 are covered because of the doubly in row 1
*/
void coverColRows(Dance *d, Doubly *crow)
{
   Doubly *doub, *hcol;

   /* cover crow's header column*/
   hcol = crow->hcol;
   hcol->right->left = hcol->left;
   hcol->left->right = hcol->right;
   d->root->dcol--; /* decrement hcol count */

   /*
      doub traverses down the hcol's column

      doub doesn't need to be covered since all the doubly in their rows
      are covered from the rest of the matrix
   */
   for(doub = hcol->down; doub != hcol; doub = doub->down)
   {
      doub->hcol->drow--;
      doub->hrow->dcol--;
      coverRows(d, doub);
   }
}

/*
   cover all doubly in doub's row vertically
*/
void coverRows(Dance *d, Doubly *doub)
{
   Doubly *xrow;

   for(xrow = doub->right; xrow != doub; xrow = xrow->right)
   {
      xrow->up->down = xrow->down;
      xrow->down->up = xrow->up;
      xrow->hcol->drow--;
      xrow->hrow->dcol--;
      
      /* checks for secondary columns */
      if(USE_HEUR && xrow->hcol->dcol < d->sec_hcol_index)
         decHeur(d, xrow->hcol->heur, 1);
   }
   if(USE_HEUR && xrow->hcol->dcol < d->sec_hcol_index)
      decHeur(d, doub->hcol->heur, 1);
}

void unselectCandidateRow(Dance *d, Doubly *candidateRow)
{
   Doubly *crow;

   uncoverColRows(d, candidateRow);
   for(crow = candidateRow->left; crow != candidateRow; crow = crow->left)
      uncoverColRows(d, crow);
}

void uncoverColRows(Dance *d, Doubly *crow)
{
   Doubly *doub, *hcol;

   hcol = crow->hcol;
   hcol->right->left = hcol;
   hcol->left->right = hcol;
   d->root->dcol++;

   for(doub = hcol->down; doub != hcol; doub = doub->down)
   {
      doub->hcol->drow++;
      doub->hrow->dcol++;
      uncoverRows(d, doub);
   }
}

void uncoverRows(Dance *d, Doubly *doub)
{
   Doubly *xrow;
   
   for(xrow = doub->right; xrow != doub; xrow = xrow->right)
   {
      xrow->up->down = xrow;
      xrow->down->up = xrow;
      xrow->hcol->drow++;
      xrow->hrow->dcol++;
      if(USE_HEUR && xrow->hcol->dcol < d->sec_hcol_index)
         incHeur(d, xrow->hcol->heur, 1);
   }
   if(USE_HEUR && xrow->hcol->dcol < d->sec_hcol_index)
      incHeur(d, doub->hcol->heur, 1);
}