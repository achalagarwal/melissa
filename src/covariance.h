/**
 *
 * @file covariance.h
 * @author Terraz Théophile
 * @date 2017-15-01
 *
 **/

#ifndef COVARIANCE_H
#define COVARIANCE_H

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * @struct covariance_s
 *
 * Structure containing an array of covariances and the corresponding mean structures
 *
 *******************************************************************************/

struct covariance_s
{
    double *covariance; /**< covariance[vect_size] */
    mean_t  mean1;      /**< corresponding mean    */
    mean_t  mean2;      /**< corresponding mean    */
    int     increment;  /**< increment             */
};

typedef struct covariance_s covariance_t; /**< type corresponding to covariance_s */

void init_covariance(covariance_t *covariance,
                     const int     vect_size);

void increment_covariance (double        in_vect1[],
                           double        in_vect2[],
                           covariance_t *partial_covariance,
                           const int     vect_size);

void update_covariance (covariance_t *covariance1,
                        covariance_t *covariance2,
                        covariance_t *updated_covariance,
                        const int     vect_size);

void free_covariance (covariance_t *covariance);

#endif // COVARIANCE_H