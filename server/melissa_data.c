/**
 *
 * @file melissa_data.c
 * @brief Routines related to the stats_data structure.
 * @author Terraz Théophile
 * @date 2016-05-24
 *
 * @defgroup melissa_data Melissa data
 *
 **/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "melissa_data.h"
#include "melissa_utils.h"

static void melissa_alloc_data (melissa_data_t *data)
{
    int i, j;

    if (data->is_valid != 1)
    {
        fprintf (stderr, "ERROR: data structure not valid (malloc_data)\n");
        exit (1);
    }

    if (data->options->mean_op == 1 && data->options->variance_op == 0)
    {
        data->means = melissa_malloc (data->options->nb_time_steps * sizeof(mean_t));
        for (i=0; i<data->options->nb_time_steps; i++)
            init_mean (&(data->means[i]), data->vect_size);
    }

    if (data->options->variance_op == 1)
    {
        data->variances = melissa_malloc (data->options->nb_time_steps * sizeof(variance_t));
        for (i=0; i<data->options->nb_time_steps; i++)
            init_variance (&(data->variances[i]), data->vect_size);
    }

    if (data->options->min_and_max_op == 1)
    {
        data->min_max = melissa_malloc (data->options->nb_time_steps * sizeof(min_max_t));
        for (i=0; i<data->options->nb_time_steps; i++)
            init_min_max (&(data->min_max[i]), data->vect_size);
    }

    if (data->options->threshold_op == 1)
    {
        data->thresholds = melissa_malloc (data->options->nb_time_steps * sizeof(int*));
        for (i=0; i<data->options->nb_time_steps; i++)
            data->thresholds[i] = melissa_calloc (data->vect_size, sizeof(int));
    }

    if (data->options->sobol_op == 1)
    {
        data->sobol_indices = melissa_malloc (data->options->nb_time_steps * sizeof(sobol_array_t));
        for (i=0; i<data->options->nb_time_steps; i++)
        {
            data->sobol_indices[i].sobol_martinez = melissa_malloc (data->options->nb_parameters * sizeof(sobol_martinez_t));
            init_variance (&data->sobol_indices[i].variance_b, data->vect_size);
            init_variance (&data->sobol_indices[i].variance_a, data->vect_size);
            for (j=0; j<data->options->nb_parameters; j++)
            {
                init_sobol_martinez (&data->sobol_indices[i].sobol_martinez[j], data->vect_size);
            }
            data->sobol_indices[i].iteration = 0;
        }
    }

    data->computed = melissa_calloc (data->options->nb_time_steps, sizeof(int));
}

/**
 *******************************************************************************
 *
 * @ingroup melissa_data
 *
 * This function initializes the data structure
 *
 *******************************************************************************
 *
 * @param[out] *data
 * pointer to the structure containing global parameters
 *
 * @param[in] *options
 * pointer to the structure containing options parsed from command line
 *
 * @param[in] vect_size
 * sise of the local input vector
 *
 *******************************************************************************/

void melissa_init_data (melissa_data_t    *data,
                        melissa_options_t *options,
                        int                vect_size)
{
    data->vect_size       = vect_size;
    data->options         = options;
    data->is_valid        = 0;
    data->means           = NULL;
    data->variances       = NULL;
    data->min_max         = NULL;
    data->thresholds      = NULL;
//    data->cond_means      = NULL;
    data->sobol_indices   = NULL;
    data->computed        = NULL;
    melissa_check_data (data);
    melissa_alloc_data (data);
}

/**
 *******************************************************************************
 *
 * @ingroup stats_data
 *
 * This function checks the data structure. It terminates the program if one of
 * the mandatory option is missing or invalid, and correct mistakes for other options
 *
 *******************************************************************************
 *
 * @param[in,out] *data
 * pointer to the structure containing global parameters
 *
 *******************************************************************************/

void melissa_check_data (melissa_data_t *data)
{
    // check options
    melissa_check_options(data->options);

    // every parameter is now validated
    data->is_valid = 1;
}

/**
 *******************************************************************************
 *
 * @ingroup melissa_data
 *
 * This function frees the memory in the data structure
 *
 *******************************************************************************
 *
 * @param[in] *data
 * pointer to the structure containing global parameters
 *
 *******************************************************************************/

void melissa_free_data (melissa_data_t *data)
{
    int i, j;

    if (data->is_valid != 1)
    {
        fprintf (stderr, "ERROR: data structure not valid (free_data)\n");
        exit (1);
    }

    if (data->options->mean_op == 1 && data->options->variance_op == 0)
    {
        for (i=0; i<data->options->nb_time_steps; i++)
            free_mean (&(data->means[i]));
        melissa_free (data->means);
    }

    if (data->options->variance_op == 1)
    {
        for (i=0; i<data->options->nb_time_steps; i++)
            free_variance (&(data->variances[i]));
        melissa_free (data->variances);
    }

    if (data->options->min_and_max_op == 1)
    {
        for (i=0; i<data->options->nb_time_steps; i++)
            free_min_max (&(data->min_max[i]));
        melissa_free (data->min_max);
    }

    if (data->options->threshold_op == 1)
    {
        for (i=0; i<data->options->nb_time_steps; i++)
            melissa_free (data->thresholds[i]);
        melissa_free (data->thresholds);
    }

    if (data->options->sobol_op == 1)
    {
        for (i=0; i<data->options->nb_time_steps; i++)
        {
            free_variance (&data->sobol_indices[i].variance_a);
            free_variance (&data->sobol_indices[i].variance_b);
            for (j=0; j<data->options->nb_parameters; j++)
            {
                free_sobol_martinez (&(data->sobol_indices[i].sobol_martinez[j]));
            }
            melissa_free (data->sobol_indices[i].sobol_martinez);
        }
        melissa_free (data->sobol_indices);
    }

    melissa_free (data->computed);

    data->options = NULL;

    data->is_valid = 0;
}

/**
 *******************************************************************************
 *
 * @ingroup stats_data
 *
 * This function computes and displays the memory consumption of the library
 *
 *******************************************************************************
 *
 * @param[in] *options
 * pointer to the structure containing global options
 *
 *******************************************************************************/

long int mem_conso (melissa_options_t *options)
{
    long int memory = 0;
    long int temp_mem = 0;

    if (options->mean_op != 0)
    {
        temp_mem = options->global_vect_size * options->nb_time_steps * sizeof(double)/* + sizeof(int)*/ / 1000000;
        fprintf(stdout, " --- Mean memory usage:               %ld MB\n", temp_mem);
        memory += temp_mem;
    }
    if (options->variance_op != 0 && options->mean_op == 0)
    {
        temp_mem = 2 * options->global_vect_size * options->nb_time_steps * sizeof(double)/* + sizeof(int)*/ / 1000000;
        fprintf(stdout, " --- Variance memory usage:           %ld MB\n", temp_mem);
        memory += temp_mem;
    }
    if (options->variance_op != 0 && options->mean_op != 0)
    {
        temp_mem = options->global_vect_size * options->nb_time_steps * sizeof(double)/* + sizeof(int)*/ / 1000000;
        fprintf(stdout, " --- Variance memory usage:           %ld MB\n", temp_mem);
        memory += temp_mem;
    }
    if (options->min_and_max_op != 0)
    {
        temp_mem = 2 * options->global_vect_size * options->nb_time_steps * sizeof(double)/* + sizeof(int)*/ / 1000000;
        fprintf(stdout, " --- Min and max memory usage:        %ld MB\n", temp_mem);
        memory += temp_mem;
    }
    if (options->threshold_op != 0)
    {
        temp_mem = options->global_vect_size * options->nb_time_steps * sizeof(int) / 1000000;
        fprintf(stdout, " --- Threshold memory usage:          %ld MB\n", temp_mem);
        memory += temp_mem;
    }
    if (options->sobol_op != 0)
    {
        // sobol indices
        temp_mem  = options->nb_parameters * 2 * options->global_vect_size * options->nb_time_steps * sizeof(double) / 1000000;
        // variances
        temp_mem += options->nb_parameters * 2 * options->global_vect_size * options->nb_time_steps * sizeof(double) / 1000000;
        // covariances
        temp_mem += options->nb_parameters * 6 * options->global_vect_size * options->nb_time_steps * sizeof(double) / 1000000;
        // glob variances
        temp_mem += 2 * options->global_vect_size * options->nb_time_steps * sizeof(double) / 1000000;
        fprintf(stdout, " --- Sobol indices memory usage:      %ld MB\n", temp_mem);
        memory += temp_mem;
    }
//    fprintf(stdout, " --- Total memory usage:      %d MB\n", memory);
    return memory;
}