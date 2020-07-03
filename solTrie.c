#include "solTrie.h"

void addLeaf(SolTrie ***leaves, SolTrie *sol, long int *cap, long int *index)
{
   if(*index >= *cap)
   {
      *cap *= 2;
      *leaves = realloc(*leaves, (*cap)*sizeof(SolTrie));
   }
   (*leaves)[*index] = sol;
   (*index)++;
}

SolTrie* initTrie(void *row)
{
   SolTrie *new = malloc(sizeof(SolTrie));
   new->row = row;
   new->num = new->totalChildren = 0;
   new->cap = STARTING_CAP;
   new->child = malloc(new->cap*sizeof(SolTrie));
   new->parent = new;

   return new;
}

void addChild(SolTrie *sol, SolTrie *new)
{
   SolTrie *temp;

   if(sol->num >= sol->cap)
   {
      sol->cap *= 2;
      sol->child = realloc(sol->child, sol->cap*sizeof(SolTrie));
   }

   sol->child[sol->num] = new;
   sol->num++;

   new->parent = sol;
   for(temp = sol; temp->parent != temp; temp = temp->parent)
      temp->totalChildren++;
   temp->totalChildren++;
}

/* 
 * finds the child with the given row, removes and frees the solTrie,
 * and shifts all the solTries behind it forward
 * returns number of solTries found and deleted
 * (0 if not found, 1 if found, 2+ if duplicate was found)
 * if duplicates found, duplicates will also be freed and removed
 */
int deleteChild(SolTrie *sol, void *row)
{
   int i, found = 0;

   for(i = 0; i < sol->num; i++)
   {
      if(found > 0)
         sol->child[i-found] = sol->child[i];
      if(sol->child[i]->row == row)
      {
         found++;
         freeSol(sol->child[i]);
      }
   }
   sol->child[i] = NULL;
   sol->num -= found;

   return found;
}

int freeSol(SolTrie *sol)
{
   int i, total = 0;

   for(i = 0; i < sol->num; i++)
      total += freeSol(sol->child[i]);

   free(sol->child);
   free(sol);

   return total;
}