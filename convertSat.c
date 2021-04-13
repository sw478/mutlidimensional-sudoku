#include "convertSat.h"
#include "error.h"
#include "math.h"

/*
    convert cell position and cell value to a SAT variable
*/
int cellToVar(Dance *d, int iSudoku, int val)
{
    return (iSudoku * d->s->containerSize) + val + 1;
}

void writeToDimacs(Dance *d)
{
    int sudokuSize = d->s->sudokuSize, containerSize = d->s->containerSize;
    int numVars, numClauses;
    FILE *dimacsFile = fopen("dimacs.txt", "w+");

    if(!dimacsFile)
        fileError("dimacs.txt");

    numVars = sudokuSize * containerSize;
    numClauses = getNumClausesMinimal(d) + getNumClausesExtended(d);

    dimacsHeader(dimacsFile, numVars, numClauses);
    dimacsMinimal(d, dimacsFile);
    dimacsExtended(d, dimacsFile);

    fclose(dimacsFile);
}

int getNumClausesMinimal(Dance *d)
{
    int sudokuSize = d->s->sudokuSize, containerSize = d->s->containerSize;
    int n = d->s->n;

    return sudokuSize +
        (n + 1) * (sudokuSize * (containerSize*(containerSize-1)/2));
}

int getNumClausesExtended(Dance *d)
{
    int sudokuSize = d->s->sudokuSize, containerSize = d->s->containerSize;
    int n = d->s->n;

    return sudokuSize * (containerSize*(containerSize-1)/2) + 
        (n + 1) * sudokuSize;
}

void dimacsMinimal(Dance *d, FILE *dimacsFile)
{
    int idim, n = d->s->n;

    dimacsAtLeastOneValuePerCell(d, dimacsFile);
    for(idim = 0; idim < n; idim++)
        dimacsAtMostOneValuePerSpan(d, dimacsFile, idim);
    dimacsAtMostOneValuePerContainer(d, dimacsFile);
}

void dimacsExtended(Dance *d, FILE *dimacsFile)
{
    int idim, n = d->s->n;

    dimacsAtMostOneValuePerCell(d, dimacsFile);
    for(idim = 0; idim < n; idim++)
        dimacsAtLeastOneValuePerSpan(d, dimacsFile, idim);
    dimacsAtLeastOneValuePerContainer(d, dimacsFile);
}

void dimacsHeader(FILE *dimacsFile, int numVars, int numClauses)
{
    fprintf(dimacsFile, "p cnf %d %d\n", numVars, numClauses);
}

void dimacsAtLeastOneValuePerCell(Dance *d, FILE *dimacsFile)
{
    int sudokuSize = d->s->sudokuSize, containerSize = d->s->containerSize;
    int iSudoku, val;

    for(iSudoku = 0; iSudoku < sudokuSize; iSudoku++)
    {
        for(val = 0; val < containerSize; val++)
            fprintf(dimacsFile, "%d ", cellToVar(d, iSudoku, val));
        fprintf(dimacsFile, "0\n");
    }
}

void dimacsAtMostOneValuePerSpan(Dance *d, FILE *dimacsFile, int idim)
{
    int sudokuSize = d->s->sudokuSize, containerSize = d->s->containerSize;
    int val, n = d->s->n, partial;

    /*
        a & b are the span indices of cells in the same span
        aSudoku & bSudoku are the sudoku indices of a & b
        aVar & bVar are the SAT vars of a & b
    */
    int a, b, aSudoku, bSudoku, aVar, bVar;

    /* number of span per sudoku, and iterator */
    int iSpan, numSpan = sudokuSize / containerSize;

    /* space between span cells */
    int cellSpace = (int)pow(containerSize, idim);

    /* loop through all span of a dimension */
    for(iSpan = 0; iSpan < numSpan; iSpan++)
    {
        /* starting iSudoku index of span */
        partial = iSpanToiSudoku(iSpan, idim, n, containerSize);

        /* loop through all values */
        for(val = 0; val < containerSize; val++)
        {
            /* loop through all pairs of indices within span */
            for(a = 0; a < containerSize-1; a++)
            {
                for(b = a+1; b < containerSize; b++)
                {
                    aSudoku = a * cellSpace + partial;
                    bSudoku = b * cellSpace + partial;
                    aVar = -1*cellToVar(d, aSudoku, val);
                    bVar = -1*cellToVar(d, bSudoku, val);
                    fprintf(dimacsFile, "%d %d 0\n", aVar, bVar);
                }
            }
        }
    }
}

int iSpanToiSudoku(int iSpan, int idim, int n, int containerSize)
{
    int i, mult = 1, partial, iSudoku = 0;
    
    for(i = 0; i < n; i++)
    {
        if(i != idim)
        {
            partial = iSpan % containerSize;
            iSpan /= containerSize;
            iSudoku += partial * mult;
        }
        mult *= containerSize;
    }

    return iSudoku;
}

void dimacsAtMostOneValuePerContainer(Dance *d, FILE *dimacsFile)
{
    int sudokuSize = d->s->sudokuSize, containerSize = d->s->containerSize;
    int *dim = d->s->dim, n = d->s->n, partial, val;
    int *invDim = inverseDim(d);

    /*
        a & b are the container indices of cells in the same container
        aSudoku & bSudoku are the sudoku indices of a & b
        aVar & bVar are the SAT vars of a & b
    */
    int a, b, aSudoku, bSudoku, aVar, bVar;

    /* number of span per sudoku, and iterator */
    int iContainer, numContainers = sudokuSize / containerSize;

    /* loop through all span of a dimension */
    for(iContainer = 0; iContainer < numContainers; iContainer++)
    {
        partial = iContainerToiSudoku(iContainer, containerSize, dim, invDim, n);

        /* loop through all values */
        for(val = 0; val < containerSize; val++)
        {
            /* loop through all pairs of indices within span */
            for(a = 0; a < containerSize-1; a++)
            {
                for(b = a+1; b < containerSize; b++)
                {
                    aSudoku = iContainerToiSudoku2(partial, a, containerSize, dim, n);
                    bSudoku = iContainerToiSudoku2(partial, b, containerSize, dim, n);
                    aVar = -1*cellToVar(d, aSudoku, val);
                    bVar = -1*cellToVar(d, bSudoku, val);
                    fprintf(dimacsFile, "%d %d 0\n", aVar, bVar);
                }
            }
        }
    }

    free(invDim);
}

int iContainerToiSudoku(int iContainer, int containerSize, int *dim, int *invDim, int n)
{
    int i, mult = 1, partial, iSudoku = 0;
    
    for(i = 0; i < n; i++)
    {
        partial = (iContainer % invDim[i]) * dim[i];
        iContainer /= invDim[i];
        iSudoku += partial * mult;
        mult *= containerSize;
    }

    return iSudoku;
}

int *inverseDim(Dance *d)
{
    int n = d->s->n, *dim = d->s->dim, containerSize = d->s->containerSize;
    int *invDim = malloc(n*sizeof(int));

    for(int i = 0; i < n; i++)
        invDim[i] = containerSize / dim[i];

    return invDim;
}

int iContainerToiSudoku2(int offset, int iCell, int containerSize, int *dim, int n)
{
    int i, mult = 1, partial, iSudoku = offset;
    
    for(i = 0; i < n; i++)
    {
        partial = iCell % dim[i];
        iCell /= dim[i];
        iSudoku += partial * mult;
        mult *= containerSize;
    }

    return iSudoku;
}

void dimacsAtMostOneValuePerCell(Dance *d, FILE *dimacsFile)
{
    int sudokuSize = d->s->sudokuSize, containerSize = d->s->containerSize;
    int iSudoku, a, b, aVar, bVar;

    for(iSudoku = 0; iSudoku < sudokuSize; iSudoku++)
    {
        for(a = 0; a < containerSize-1; a++)
        {
            for(b = a+1; b < containerSize; b++)
            {
                aVar = -1*cellToVar(d, iSudoku, a);
                bVar = -1*cellToVar(d, iSudoku, b);
                fprintf(dimacsFile, "%d %d 0\n", aVar, bVar);
            }
        }
    }
}

void dimacsAtLeastOneValuePerSpan(Dance *d, FILE *dimacsFile, int idim)
{
    int sudokuSize = d->s->sudokuSize, containerSize = d->s->containerSize;
    int val, n = d->s->n, partial, iSudoku, iCell;

    /* number of span per sudoku, and iterator */
    int iSpan, numSpan = sudokuSize / containerSize;

    /* space between span cells */
    int cellSpace = (int)pow(containerSize, idim);

    /* loop through all span of a dimension */
    for(iSpan = 0; iSpan < numSpan; iSpan++)
    {
        /* starting iSudoku index of span */
        partial = iSpanToiSudoku(iSpan, idim, n, containerSize);

        for(val = 0; val < containerSize; val++)
        {
            for(iCell = 0; iCell < containerSize; iCell++)
            {
                iSudoku = iCell * cellSpace + partial;
                fprintf(dimacsFile, "%d ", cellToVar(d, iSudoku, val));
            }
            fprintf(dimacsFile, "0\n");
        }
    }
}

void dimacsAtLeastOneValuePerContainer(Dance *d, FILE *dimacsFile)
{
    int sudokuSize = d->s->sudokuSize, containerSize = d->s->containerSize;
    int *dim = d->s->dim, n = d->s->n, partial, val;
    int *invDim = inverseDim(d), iSudoku, iCell;

    /* number of span per sudoku, and iterator */
    int iContainer, numContainers = sudokuSize / containerSize;

    /* loop through all span of a dimension */
    for(iContainer = 0; iContainer < numContainers; iContainer++)
    {
        partial = iContainerToiSudoku(iContainer, containerSize, dim, invDim, n);

        /* loop through all values */
        for(val = 0; val < containerSize; val++)
        {
            for(iCell = 0; iCell < containerSize; iCell++)
            {
                iSudoku = iContainerToiSudoku2(partial, iCell, containerSize, dim, n);
                fprintf(dimacsFile, "%d ", cellToVar(d, iSudoku, val));
            }
            fprintf(dimacsFile, "0\n");
        }
    }

    free(invDim);
}

void testConvertSat(Dance *d)
{
    int containerSize = d->s->containerSize, sudokuSize = d->s->sudokuSize;
    int n = d->s->n, *dim = d->s->dim, *invDim, offset;
    int iSudoku, iCell, idim, iSpan, iContainer;
    int invContainerSize = sudokuSize / containerSize;

    for(idim = 0; idim < n; idim++)
    {
        printf("\tidim: %d\n", idim);
        for(iSpan = 0; iSpan < sudokuSize / containerSize; iSpan++)
        {
            iSudoku = iSpanToiSudoku(iSpan, idim, n, containerSize);
            printf("%d ", iSudoku);
        }
        printf("\n");
    }

    invDim = inverseDim(d);
    printf("\tinv dim\n");
    for(idim = 0; idim < n; idim++)
        printf("%d ", invDim[idim]);
    printf("\n");

    printf("\tcontainers\n");
    for(iContainer = 0; iContainer < invContainerSize; iContainer++)
    {
        iSudoku = iContainerToiSudoku(iContainer, containerSize, dim, invDim, n);
        printf("%d ", iSudoku);
    }
    printf("\n");

    offset = 0;
    for(iCell = 0; iCell < containerSize; iCell++)
    {
        iSudoku = iContainerToiSudoku2(offset, iCell, containerSize, dim, n);
        printf("%d ", iSudoku);
    }

    free(invDim);
}