/**
 *
 * @file quantile.c
 * @brief Quantile related functions.
 * @author Terraz Théophile
 * @date 2017-18-05
 *
 **/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "quantile.h"
#include "melissa_utils.h"

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * This function initializes a quantile structure.
 *
 *******************************************************************************
 *
 * @param[in,out] *quantile
 * the quantile structure to initialize
 *
 * @param[in] vect_size
 * size of the quantile vector
 *
 * @param[in] alpha
 * alpha parameter of the algotithm
 *
 * @param[in] gamma
 * gamma initial value
 *
 *******************************************************************************/

void init_quantile (quantile_t   *quantile,
                    const int     vect_size,
                    const double  alpha)
{
    int i;

    quantile->quantile = melissa_calloc (vect_size, sizeof(double));
    for (i=0; i<vect_size; i++)
    {
        quantile->quantile[i] = alpha;
    }
    quantile->increment = 0;
    quantile->alpha = alpha;
}

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * This function updates the incremental quantile.
 *
 *******************************************************************************
 *
 * @param[in] in_vect[]
 * input vector of double values
 *
 * @param[in] nmax
 * maximum number of iterations
 *
 * @param[in,out] *partial_mean
 * input: previously computed partial mean,
 * output: updated partial mean
 *
 * @param[in] vect_size
 * size of the input vectors
 *
 *******************************************************************************/

void increment_quantile (quantile_t *quantile,
                         const int   nmax,
                         double      in_vect[],
                         const int   vect_size)
{
    int    i;
    double temp, gamma;

    quantile->increment += 1;

#ifdef BUILD_WITH_OPENMP
#pragma omp parallel for
#endif // BUILD_WITH_OPENMP
    if (quantile->increment > 1)
    {
    for (i=0; i<vect_size; i++)
    {
        gamma = quantile->increment*(0.9/(nmax-1)) - 0.9/(nmax-1) +0.1;
        if (quantile->quantile[i] >= in_vect[i])
        {
            temp = 1 - quantile->alpha;
        }
        else
        {
            temp = 0 - quantile->alpha;
        }
        quantile->quantile[i] -= temp/pow(quantile->increment, gamma);
    }
    }
    else
    {
        for (i=0; i<vect_size; i++)
        {
            quantile->quantile[i] = in_vect[i];
        }
    }
}

/**
 *******************************************************************************
 *
 * @ingroup save_stats
 *
 * This function writes an array of quantile structures on disc
 *
 *******************************************************************************
 *
 * @param[in] *quantile
 * quantile structures to save, size nb_time_steps
 *
 * @param[in] vect_size
 * size of double vectors
 *
 * @param[in] nb_time_steps
 * number of time_steps of the study
 *
 * @param[in] f
 * file descriptor
 *
 *******************************************************************************/

void save_quantile(quantile_t *quantiles,
                   int         vect_size,
                   int         nb_time_steps,
                   FILE*       f)
{
    int i;
    for (i=0; i<nb_time_steps; i++)
    {
        fwrite(quantiles[i].quantile, sizeof(double), vect_size, f);
        fwrite(&quantiles[i].increment, sizeof(int), 1, f);
        fwrite(&quantiles[i].alpha, sizeof(double), 1, f);
    }
}

/**
 *******************************************************************************
 *
 * @ingroup save_stats
 *
 * This function reads an array of quantile structures on disc
 *
 *******************************************************************************
 *
 * @param[in] *quantile
 * quantile structures to read, size nb_time_steps
 *
 * @param[in] vect_size
 * size of double vectors
 *
 * @param[in] nb_time_steps
 * number of time_steps of the study
 *
 * @param[in] f
 * file descriptor
 *
 *******************************************************************************/

void read_quantile(quantile_t *quantiles,
                   int         vect_size,
                   int         nb_time_steps,
                   FILE*       f)
{
    int i;
    for (i=0; i<nb_time_steps; i++)
    {
        fread(quantiles[i].quantile, sizeof(double), vect_size, f);
        fread(&quantiles[i].increment, sizeof(int), 1, f);
        fread(&quantiles[i].alpha, sizeof(double), 1, f);
    }
}

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * This function frees a quantile structure.
 *
 *******************************************************************************
 *
 * @param[in,out] *quantile
 * the quantile structure to free
 *
 *******************************************************************************/

void free_quantile (quantile_t *quantile)
{
    melissa_free (quantile->quantile);
}
