/**
 *
 * @file min_max.c
 * @brief Min and max related functions.
 * @author Terraz Théophile
 * @date 2016-15-02
 *
 **/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "stats.h"

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * This function initializes a min and max structure.
 *
 *******************************************************************************
 *
 * @param[in,out] *min_max
 * the min and max structure to initialize
 *
 * @param[in] vect_size
 * size of the vectors
 *
 *******************************************************************************/

void init_min_max (min_max_t *min_max,
                   const int  vect_size)
{
    min_max->min = malloc (vect_size * sizeof(double));
    min_max->max = malloc (vect_size * sizeof(double));
    min_max->is_init = 0;
}

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * This function updates the min and the max values of min and max vectors
 * using the input vector.
 *
 *******************************************************************************
 *
 * @param[in] in_vect[]
 * input vector of double values
 *
 * @param[in,out] *min_max
 * the min and max structure
 *
 * @param[in] vect_size
 * size of the input vectors
 *
 *******************************************************************************/

void min_and_max (double     in_vect[],
                  min_max_t *min_max,
                  const int  vect_size)
{
    if (min_max->is_init == 0)
    {
        memcpy (min_max->min, in_vect, vect_size * sizeof(double));
        memcpy (min_max->max, in_vect, vect_size * sizeof(double));
        min_max->is_init = 1;
    }
    else
    {
        int     i;
#pragma omp parallel for
        for (i=0; i<vect_size; i++)
        {
            if (min_max->min[i] > in_vect[i])
                min_max->min[i] = in_vect[i];
            if (min_max->max[i] < in_vect[i])
                min_max->max[i] = in_vect[i];
        }
    }
}

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * This function frees a min and max structure.
 *
 *******************************************************************************
 *
 * @param[in] *min_max
 * the min and max structure to free
 *
 *******************************************************************************/

void free_min_max (min_max_t *min_max)
{
    free (min_max->min);
    free (min_max->max);
}
