/**
 *
 * @file test_base.c
 * @brief Basic tests to try basic statistic computations.
 * @author Terraz Théophile
 * @date 2017-15-01
 *
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "mean.h"
#include "variance.h"
#include "covariance.h"
#include "melissa_utils.h"

int main(int argc, char **argv)
{
    double       *tableau = NULL;
    double       *ref_mean = NULL;
    mean_t        my_mean;
    int           n = 1000; // n expériences
    int           vect_size = 100000; // size points de l'espace
    int           i, j;
    int           ret = 0;
    double        start_time = 0;
    double        end_time = 0;

    init_mean (&my_mean, vect_size);
    tableau = calloc (n * vect_size, sizeof(double));
    ref_mean = calloc (vect_size, sizeof(double));

    for (j=0; j<vect_size * n; j++)
    {
        tableau[j] = rand() / (double)RAND_MAX * (1000);
    }
    for (i=0; i<vect_size; i++)
    {
        for (j=0; j<n; j++)
        {
            ref_mean[i] += tableau[i + j*vect_size];
        }
        ref_mean[i] /= (double)n;
    }
    start_time = melissa_get_time();
    for (j=0; j<n; j++)
    {
        increment_mean (&my_mean, &tableau[j * vect_size], vect_size);
    }
    end_time = melissa_get_time();
    fprintf (stdout, "simple mean time: %g\n", end_time - start_time);
    for (i=0; i<vect_size; i++)
    {
        if (fabs((my_mean.mean[i] - ref_mean[i])/ref_mean[i]) > 10E-12)
        {
            fprintf (stdout, "mean failed (iterative mean = %g, ref mean = %g, i=%d)\n", my_mean.mean[i], ref_mean[i], i);
            ret = 1;
        }
    }

    melissa_free(tableau);
    melissa_free(ref_mean);
    free_mean(&my_mean);

    return ret;
}
