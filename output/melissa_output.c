/******************************************************************
*                            Melissa                              *
*-----------------------------------------------------------------*
*   COPYRIGHT (C) 2017  by INRIA and EDF. ALL RIGHTS RESERVED.    *
*                                                                 *
* This source is covered by the BSD 3-Clause License.             *
* Refer to the  LICENCE file for further information.             *
*                                                                 *
*-----------------------------------------------------------------*
*  Original Contributors:                                         *
*    Theophile Terraz,                                            *
*    Bruno Raffin,                                                *
*    Alejandro Ribes,                                             *
*    Bertrand Iooss,                                              *
******************************************************************/

/**
 *
 * @file melissa_output.c
 * @brief Writes stats on disc.
 * @author Terraz Théophile
 * @date 2017-19-01
 *
 * @defgroup melissa_output output statistics on disc
 *
 **/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
//#include "hdf5.h"
#include "melissa_output.h"

void write_output_d_txt(const char   *file_name,
                        const char   *field,
                        const int     t,
                        const size_t  vec_size,
                        const double  vec[])
{
    int   i;
    FILE* f;

    f = fopen(file_name, "w");
    for (i=0; i<vec_size; i++)
    {
        fprintf (f, "%g\n", vec[i]);
    }
    fclose(f);
}

void write_output_i_txt(const char   *file_name,
                        const char   *field,
                        const int     t,
                        const size_t  vec_size,
                        const int     vec[])
{
    int   i;
    FILE* f;

    f = fopen(file_name, "w");
    for (i=0; i<vec_size; i++)
    {
        fprintf (f, "%d\n", vec[i]);
    }
    fclose(f);
}


void write_output_d_ensight(const char   *file_name,
                            const char   *field,
                            const int     t,
                            const size_t  vect_size,
                            const double  vec[])
{
    int     i;
    FILE*   f;
    double  time_value;
    char    c_buffer[81], c_buffer2[81];
    float  *s_buffer;
    int32_t     n = 1;

    time_value = 0.0012 * t;
    s_buffer = melissa_malloc (vect_size * sizeof(float));

    for (i=0; i<vect_size; i++)
    {
        s_buffer[i] = (float)vec[i];
    }

    f = fopen(file_name, "w");
    sprintf(c_buffer, "%s (time values: %d, %g)", file_name, (int)t, time_value);
    strncpy(c_buffer2, c_buffer, 80);
    for (i=strlen(c_buffer); i < 80; i++)
        c_buffer2[i] = ' ';
    c_buffer2[80] = '\0';
    fwrite (c_buffer2, sizeof(char), 80, f);
    sprintf(c_buffer, "part");
    strncpy(c_buffer2, c_buffer, 80);
    for (i=strlen(c_buffer); i < 80; i++)
        c_buffer2[i] = ' ';
    c_buffer2[80] = '\0';
    fwrite (c_buffer2, sizeof(char), 80, f);
    fwrite (&n, sizeof(int32_t), 1, f);
    sprintf(c_buffer, "hexa8");
    strncpy(c_buffer2, c_buffer, 80);
    for (i=strlen(c_buffer); i < 80; i++)
        c_buffer2[i] = ' ';
    c_buffer2[80] = '\0';
    fwrite (c_buffer2, sizeof(char), 80, f);
    fwrite (s_buffer, sizeof(float), vect_size, f);

    fclose(f);
    melissa_free (s_buffer);
}


void write_output_i_ensight(const char   *file_name,
                            const char   *field,
                            const int     t,
                            const size_t  vect_size,
                            const int     vec[])
{
    int     i;
    FILE*   f;
    double  time_value;
    char    c_buffer[81], c_buffer2[81];
    float  *s_buffer;
    int32_t     n = 1;

    time_value = 0.0012 * t;
    s_buffer = melissa_malloc (vect_size * sizeof(float));

    for (i=0; i<vect_size; i++)
    {
        s_buffer[i] = (float)vec[i];
    }

    f = fopen(file_name, "w");
    sprintf(c_buffer, "%s (time values: %d, %g)", file_name, (int)t, time_value);
    strncpy(c_buffer2, c_buffer, 80);
    for (i=strlen(c_buffer); i < 80; i++)
        c_buffer2[i] = ' ';
    c_buffer2[80] = '\0';
    fwrite (c_buffer2, sizeof(char), 80, f);
    sprintf(c_buffer, "part");
    strncpy(c_buffer2, c_buffer, 80);
    for (i=strlen(c_buffer); i < 80; i++)
        c_buffer2[i] = ' ';
    c_buffer2[80] = '\0';
    fwrite (c_buffer2, sizeof(char), 80, f);
    fwrite (&n, sizeof(int32_t), 1, f);
    sprintf(c_buffer, "hexa8");
    strncpy(c_buffer2, c_buffer, 80);
    for (i=strlen(c_buffer); i < 80; i++)
        c_buffer2[i] = ' ';
    c_buffer2[80] = '\0';
    fwrite (c_buffer2, sizeof(char), 80, f);
    fwrite (s_buffer, sizeof(float), vect_size, f);

    fclose(f);
    melissa_free (s_buffer);
}

static inline void dgather_data(comm_data_t *comm_data,
                               int *local_vect_sizes,
                               double *d_buffer)
{
    int temp_offset = 0;
    int j;
#ifdef BUILD_WITH_MPI
    MPI_Status  status;
    if (comm_data->rank == 0)
    {
        for (j=1; j<comm_data->comm_size; j++)
        {
            temp_offset += local_vect_sizes[j-1];
            if (local_vect_sizes[j] > 0)
            {
                MPI_Recv (&d_buffer[temp_offset], local_vect_sizes[j], MPI_DOUBLE, j, j+121, comm_data->comm, &status);
            }
        }
        temp_offset = 0;
    }
    else
    {
        if (local_vect_sizes[comm_data->rank] > 0)
        {
            MPI_Send(d_buffer, local_vect_sizes[comm_data->rank], MPI_DOUBLE, 0, comm_data->rank+121, comm_data->comm);
        }
    }
#endif // BUILD_WITH_MPI
}

static inline void igather_data(comm_data_t *comm_data,
                               int *local_vect_sizes,
                               int *i_buffer)
{
    int temp_offset = 0;
    int j;
#ifdef BUILD_WITH_MPI
    MPI_Status  status;
    if (comm_data->rank == 0)
    {
        for (j=1; j<comm_data->comm_size; j++)
        {
            temp_offset += local_vect_sizes[j-1];
            if (local_vect_sizes[j] > 0)
            {
                MPI_Recv (&i_buffer[temp_offset], local_vect_sizes[j], MPI_INT, j, j+121, comm_data->comm, &status);
            }
        }
        temp_offset = 0;
    }
    else
    {
        if (local_vect_sizes[comm_data->rank] > 0)
        {
            MPI_Send(i_buffer, local_vect_sizes[comm_data->rank], MPI_INT, 0, comm_data->rank+121, comm_data->comm);
        }
    }
#endif // BUILD_WITH_MPI
}

/**
 *******************************************************************************
 *
 * @ingroup melissa_output
 *
 * This function writes the computed statistics on files
 *
 *******************************************************************************
 *
 * @param[in] **data
 * pointer to the array of structures containing statistics data
 *
 * @param[in] *options
 * Melissa option structure
 *
 * @param[in] comm_data
 * structure containing communications parameters
 *
 * @param[in] *field
 * name of the statistic field to write
 *
 *******************************************************************************/

void melissa_write_stats_seq(melissa_data_t    **data,
                             melissa_options_t  *options,
                             comm_data_t        *comm_data,
                             char               *field)
{
    long int    temp_offset=0;
    void (*write_output_d)(const char *, const char *, const int, const size_t, const double[]) = &write_output_d_txt;
    void (*write_output_i)(const char *, const char *, const int, const size_t, const int[]) = &write_output_i_txt;
#ifdef BUILD_WITH_MPI
    MPI_Status  status;
#endif // BUILD_WITH_MPI
    long int    i, j;
    int        *offsets;
    int         t, p;
    char        file_name[256];
    int         max_size_time;
    int         vect_size = 0;
    int        *local_vect_sizes;
    int         global_vect_size = 0;
    int        *i_buffer;
    double     *d_buffer;
//    double temp1, temp2;

    max_size_time=floor(log10(options->nb_time_steps))+1;

    // ================= first test =============== //
    //                                              //
    // Communicate local vect size to every process //
    //                                              //
    // ============================================ //

    local_vect_sizes = melissa_calloc (comm_data->comm_size, sizeof(int));
    offsets = melissa_calloc (comm_data->comm_size, sizeof(int));
    for (i=0; i<comm_data->client_comm_size; i++)
    {
        vect_size += (*data)[i].vect_size;
    }
#ifdef BUILD_WITH_MPI
    MPI_Allgather (&vect_size, 1, MPI_INT, local_vect_sizes, 1, MPI_INT, comm_data->comm);
#endif // BUILD_WITH_MPI

    for (i=0; i<comm_data->comm_size-1; i++)
    {
        offsets[i+1] = offsets[i] + local_vect_sizes[i];
        global_vect_size += local_vect_sizes[i];
    }
    global_vect_size += local_vect_sizes[comm_data->comm_size-1];

    // ============================================ //

    d_buffer = melissa_malloc (global_vect_size * sizeof(double));

    if (options->mean_op == 1)
    {
        for (t=0; t<options->nb_time_steps; t++)
        {
            sprintf(file_name, "results.%s_mean.%.*d", field, max_size_time, (int)t+1);
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if ((*data)[i].vect_size > 0)
                {
                    memcpy(&d_buffer[temp_offset], (*data)[i].moments[t].m1, (*data)[i].vect_size*sizeof(double));
                    temp_offset += (*data)[i].vect_size;
                }
            }
            temp_offset = 0;
            dgather_data (comm_data, local_vect_sizes, d_buffer);
            if (comm_data->rank == 0)
            {
                (*write_output_d)(file_name,
                                  field,
                                  t,
                                  global_vect_size,
                                  d_buffer);
            }
        }
    }

    if (options->variance_op == 1)
    {
        for (t=0; t<options->nb_time_steps; t++)
        {
            sprintf(file_name, "results.%s_variance.%.*d", field, max_size_time, (int)t+1);
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if ((*data)[i].vect_size > 0)
                {
                    memcpy(&d_buffer[temp_offset], (*data)[i].moments[t].theta2, (*data)[i].vect_size*sizeof(double));
                    temp_offset += (*data)[i].vect_size;
                }
            }
            temp_offset = 0;
            dgather_data (comm_data, local_vect_sizes, d_buffer);
            if (comm_data->rank == 0)
            {
                (*write_output_d)(file_name,
                                  field,
                                  t,
                                  global_vect_size,
                                  d_buffer);
            }
        }
    }

    if (options->skewness_op == 1)
    {
        for (t=0; t<options->nb_time_steps; t++)
        {
            sprintf(file_name, "results.%s_skewness.%.*d", field, max_size_time, (int)t+1);
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if ((*data)[i].vect_size > 0)
                {
                    compute_skewness (&(*data)[i].moments[t], &d_buffer[temp_offset], (*data)[i].vect_size);
                    temp_offset += (*data)[i].vect_size;
                }
            }
            temp_offset = 0;
            dgather_data (comm_data, local_vect_sizes, d_buffer);
            if (comm_data->rank == 0)
            {
                (*write_output_d)(file_name,
                                  field,
                                  t,
                                  global_vect_size,
                                  d_buffer);
            }
        }
    }

    if (options->kurtosis_op == 1)
    {
        for (t=0; t<options->nb_time_steps; t++)
        {
            sprintf(file_name, "results.%s_kurtosis.%.*d", field, max_size_time, (int)t+1);
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if ((*data)[i].vect_size > 0)
                {
                    compute_kurtosis (&(*data)[i].moments[t], &d_buffer[temp_offset], (*data)[i].vect_size);
                    temp_offset += (*data)[i].vect_size;
                }
            }
            temp_offset = 0;
            dgather_data (comm_data, local_vect_sizes, d_buffer);
            if (comm_data->rank == 0)
            {
                (*write_output_d)(file_name,
                                  field,
                                  t,
                                  global_vect_size,
                                  d_buffer);
            }
        }
    }

    if (options->min_and_max_op == 1)
    {
        for (t=0; t<options->nb_time_steps; t++)
        {
            sprintf(file_name, "results.%s_min.%.*d", field, max_size_time, (int)t+1);
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if ((*data)[i].vect_size > 0)
                {
                    memcpy(&d_buffer[temp_offset], (*data)[i].min_max[t].min, (*data)[i].vect_size*sizeof(double));
                    temp_offset += (*data)[i].vect_size;
                }
            }
            temp_offset = 0;
            dgather_data (comm_data, local_vect_sizes, d_buffer);
            if (comm_data->rank == 0)
            {
                (*write_output_d)(file_name,
                                  field,
                                  t,
                                  global_vect_size,
                                  d_buffer);
            }
        }

        for (t=0; t<options->nb_time_steps; t++)
        {
            sprintf(file_name, "results.%s_max.%.*d", field, max_size_time, (int)t+1);
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if ((*data)[i].vect_size > 0)
                {
                    memcpy(&d_buffer[temp_offset], (*data)[i].min_max[t].max, (*data)[i].vect_size*sizeof(double));
                    temp_offset += (*data)[i].vect_size;
                }
            }
            temp_offset = 0;
            dgather_data (comm_data, local_vect_sizes, d_buffer);
            if (comm_data->rank == 0)
            {
                (*write_output_d)(file_name,
                                  field,
                                  t,
                                  global_vect_size,
                                  d_buffer);
            }
        }
    }

    if (options->threshold_op == 1)
    {
        i_buffer = (int*)d_buffer;
        for (t=0; t<options->nb_time_steps; t++)
        {
            sprintf(file_name, "results.%s_threshold.%.*d", field, max_size_time, (int)t+1);
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if ((*data)[i].vect_size > 0)
                {
                    memcpy(&i_buffer[temp_offset], (*data)[i].thresholds[t], (*data)[i].vect_size*sizeof(int));
                    temp_offset += (*data)[i].vect_size;
                }
            }
            temp_offset = 0;
            igather_data (comm_data, local_vect_sizes, i_buffer);
            if (comm_data->rank == 0)
            {
                (*write_output_i)(file_name,
                                  field,
                                  t,
                                  global_vect_size,
                                  i_buffer);
            }
        }
    }

    if (options->quantile_op == 1)
    {
        int value;
        for (value=0; value<options->nb_quantiles; value++)
        {
            for (t=0; t<options->nb_time_steps; t++)
            {
                sprintf(file_name, "results.%s_quantile%g.%.*d", field, options->quantile_order[value], max_size_time, (int)t+1);
                for (i=0; i<comm_data->client_comm_size; i++)
                {
                    if ((*data)[i].vect_size > 0)
                    {
                        memcpy(&d_buffer[temp_offset], (*data)[i].quantiles[t][value].quantile, (*data)[i].vect_size*sizeof(double));
                        temp_offset += (*data)[i].vect_size;
                    }
                }
                temp_offset = 0;
                dgather_data (comm_data, local_vect_sizes, d_buffer);
                if (comm_data->rank == 0)
                {
                    (*write_output_d)(file_name,
                                      field,
                                      t,
                                      global_vect_size,
                                      d_buffer);
                }
            }
        }
    }

    if (options->sobol_op == 1)
    {
        for (p=0; p<options->nb_parameters; p++)
        {
            for (t=0; t<options->nb_time_steps; t++)
            {
                sprintf(file_name, "results.%s_sobol%d.%.*d", field, p, max_size_time, (int)(t+1));
                for (i=0; i<comm_data->client_comm_size; i++)
                {
                    if ((*data)[i].vect_size > 0)
                    {
                        memcpy(&d_buffer[temp_offset], (*data)[i].sobol_indices[t].sobol_martinez[p].first_order_values, (*data)[i].vect_size*sizeof(double));
                        temp_offset += (*data)[i].vect_size;
                    }
                }
                temp_offset = 0;
                dgather_data (comm_data, local_vect_sizes, d_buffer);
                if (comm_data->rank == 0)
                {
                    (*write_output_d)(file_name,
                                      field,
                                      t,
                                      global_vect_size,
                                      d_buffer);
                }
            }
        }
        for (p=0; p<options->nb_parameters; p++)
        {
            for (t=0; t<options->nb_time_steps; t++)
            {
                sprintf(file_name, "results.%s_sobol_tot%d.%.*d", field, p, max_size_time, (int)(t+1));
                for (i=0; i<comm_data->client_comm_size; i++)
                {
                    if ((*data)[i].vect_size > 0)
                    {
                        memcpy(&d_buffer[temp_offset], (*data)[i].sobol_indices[t].sobol_martinez[p].total_order_values, (*data)[i].vect_size*sizeof(double));
                        temp_offset += (*data)[i].vect_size;
                    }
                }
                temp_offset = 0;
                dgather_data (comm_data, local_vect_sizes, d_buffer);
                if (comm_data->rank == 0)
                {
                    (*write_output_d)(file_name,
                                      field,
                                      t,
                                      global_vect_size,
                                      d_buffer);
                }
            }
        }
    }
    melissa_free (d_buffer);
    melissa_free (offsets);
    melissa_free (local_vect_sizes);

}

/**
 *******************************************************************************
 *
 * @ingroup melissa_output
 *
 * This function writes the computed statistics on files
 *
 *******************************************************************************
 *
 * @param[in] **data
 * pointer to the array of structures containing statistics data
 *
 * @param[in] *options
 * Melissa option structure
 *
 * @param[in] comm_data
 * structure containing communications parameters
 *
 * @param[in] *field
 * name of the statistic field to write
 *
 *******************************************************************************/

void write_stats_bin (melissa_data_t    **data,
                      melissa_options_t  *options,
                      comm_data_t        *comm_data,
                      char               *field
                      )
{
    long int   i, temp_offset = 0;
    int        t, offset = 0;
#ifdef BUILD_WITH_MPI
    MPI_File   f;
    MPI_Status status;
#else // BUILD_WITH_MPI
    FILE* f;
#endif // BUILD_WITH_MPI
    char       file_name[256];
    int        max_size_time;
    int        vect_size = 0;
    int       *local_vect_sizes;
    int        global_vect_size = 0;

//// https://support.hdfgroup.org/HDF5/Tutor/parallel.html
//    /*
//     * HDF5 APIs definitions
//     */
//    hid_t   file_id;         /* file and dataset identifiers */
//    hid_t	plist_id;        /* property list identifier( access template) */
//    herr_t	status2;

    max_size_time=floor(log10(options->nb_time_steps))+1;

    // ============================================ //
    //                                              //
    // Communicate local vect size to every process //
    //                                              //
    // ============================================ //

    local_vect_sizes = melissa_calloc (comm_data->comm_size, sizeof(int));
    for (i=0; i<comm_data->client_comm_size; i++)
    {
        vect_size += (*data)[i].vect_size;
    }
#ifdef BUILD_WITH_MPI
    MPI_Allgather (&vect_size, 1, MPI_INT, local_vect_sizes, 1, MPI_INT, comm_data->comm);
#endif // BUILD_WITH_MPI

    for (i=0; i<comm_data->comm_size; i++)
    {
        global_vect_size += local_vect_sizes[i];
    }

    // ============================================ //

    for (i=0; i<comm_data->rank; i++)
    {
        offset += local_vect_sizes[i];
    }

    if (options->mean_op == 1)
    {
        for (t=0; t<options->nb_time_steps; t++)
        {
            sprintf(file_name, "%s_mean_%.*d", field, max_size_time, (int)t+1);
//            /*
//             * Set up file access property list with parallel I/O access
//             */
//             plist_id = H5Pcreate(H5P_FILE_ACCESS);
//             H5Pset_fapl_mpio(plist_id, comm_data->comm, info);

//             /*
//              * Create a new file collectively.
//              */
//             file_id = H5Fcreate(file_name, H5F_ACC_TRUNC, H5P_DEFAULT, plist_id);
#ifdef BUILD_WITH_MPI
            MPI_File_open (comm_data->comm, file_name, MPI_MODE_CREATE|MPI_MODE_WRONLY, MPI_INFO_NULL, &f);
            temp_offset = 0;
#else // BUILD_WITH_MPI
            f = fopen(file_name, "wb");
#endif // BUILD_WITH_MPI
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if ((*data)[i].vect_size > 0)
                {
#ifdef BUILD_WITH_MPI
                    MPI_File_write_at (f, offset + temp_offset, (*data)[i].moments[t].m1, (*data)[i].vect_size, MPI_DOUBLE, &status);
                    temp_offset += (*data)[i].vect_size;
#else // BUILD_WITH_MPI
                    fwrite((*data)[i].moments[t].m1, sizeof(double), (*data)[i].vect_size, f);
#endif // BUILD_WITH_MPI
                }
            }
#ifdef BUILD_WITH_MPI
            MPI_File_close (&f);
#else // BUILD_WITH_MPI
            fclose(f);
#endif // BUILD_WITH_MPI

//        /*
//         * Close property list.
//         */
//        H5Pclose(plist_id);

//        /*
//         * Close the file.
//         */
//        H5Fclose(file_id);
        }
    }

    if (options->variance_op == 1)
    {
        for (t=0; t<options->nb_time_steps; t++)
        {
            sprintf(file_name, "%s_variance_%.*d", field, max_size_time, (int)t+1);
#ifdef BUILD_WITH_MPI
            MPI_File_open (comm_data->comm, file_name, MPI_MODE_CREATE|MPI_MODE_WRONLY, MPI_INFO_NULL, &f);
            temp_offset = 0;
#else // BUILD_WITH_MPI
            f = fopen(file_name, "wb");
#endif // BUILD_WITH_MPI
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if ((*data)[i].vect_size > 0)
                {
#ifdef BUILD_WITH_MPI
                    MPI_File_write_at (f, offset + temp_offset, (*data)[i].moments[t].theta2, (*data)[i].vect_size, MPI_DOUBLE, &status);
                    temp_offset += (*data)[i].vect_size;
#else // BUILD_WITH_MPI
                    fwrite((*data)[i].moments[t].theta2, sizeof(double), (*data)[i].vect_size, f);
#endif // BUILD_WITH_MPI
                }
            }
#ifdef BUILD_WITH_MPI
            MPI_File_close (&f);
#else // BUILD_WITH_MPI
            fclose(f);
#endif // BUILD_WITH_MPI
        }
    }

    if (options->min_and_max_op == 1)
    {
        for (t=0; t<options->nb_time_steps; t++)
        {
            sprintf(file_name, "%s_min_%.*d", field, max_size_time, (int)t+1);
#ifdef BUILD_WITH_MPI
            MPI_File_open (comm_data->comm, file_name, MPI_MODE_CREATE|MPI_MODE_WRONLY, MPI_INFO_NULL, &f);
            temp_offset = 0;
#else // BUILD_WITH_MPI
            f = fopen(file_name, "wb");
#endif // BUILD_WITH_MPI
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if ((*data)[i].vect_size > 0)
                {
#ifdef BUILD_WITH_MPI
                    MPI_File_write_at (f, offset + temp_offset, (*data)[i].min_max[t].min, (*data)[i].vect_size, MPI_DOUBLE, &status);
                    temp_offset += (*data)[i].vect_size;
#else // BUILD_WITH_MPI
                    fwrite((*data)[i].min_max[t].min, sizeof(double), (*data)[i].vect_size, f);
#endif // BUILD_WITH_MPI
                }
            }
#ifdef BUILD_WITH_MPI
            MPI_File_close (&f);
#else // BUILD_WITH_MPI
            fclose(f);
#endif // BUILD_WITH_MPI
        }

        for (t=0; t<options->nb_time_steps; t++)
        {
            sprintf(file_name, "%s_max_%.*d", field, max_size_time, (int)t+1);
#ifdef BUILD_WITH_MPI
            MPI_File_open (comm_data->comm, file_name, MPI_MODE_CREATE|MPI_MODE_WRONLY, MPI_INFO_NULL, &f);
            temp_offset = 0;
#else // BUILD_WITH_MPI
            f = fopen(file_name, "wb");
#endif // BUILD_WITH_MPI
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if ((*data)[i].vect_size > 0)
                {
#ifdef BUILD_WITH_MPI
                    MPI_File_write_at (f, offset + temp_offset, (*data)[i].min_max[t].max, (*data)[i].vect_size, MPI_DOUBLE, &status);
                    temp_offset += (*data)[i].vect_size;
#else // BUILD_WITH_MPI
                    fwrite((*data)[i].min_max[t].max, sizeof(double), (*data)[i].vect_size, f);
#endif // BUILD_WITH_MPI
                }
            }
#ifdef BUILD_WITH_MPI
            MPI_File_close (&f);
#else // BUILD_WITH_MPI
            fclose(f);
#endif // BUILD_WITH_MPI
        }
    }

    if (options->threshold_op == 1)
    {
        for (t=0; t<options->nb_time_steps; t++)
        {
            sprintf(file_name, "%s_threshold_exceedance_%.*d", field, max_size_time, (int)t+1);
#ifdef BUILD_WITH_MPI
            MPI_File_open (comm_data->comm, file_name, MPI_MODE_CREATE|MPI_MODE_WRONLY, MPI_INFO_NULL, &f);
            temp_offset = 0;
#else // BUILD_WITH_MPI
            f = fopen(file_name, "wb");
#endif // BUILD_WITH_MPI
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if ((*data)[i].vect_size > 0)
                {
#ifdef BUILD_WITH_MPI
                    MPI_File_write_at (f, offset + temp_offset, (*data)[i].thresholds[t], (*data)[i].vect_size, MPI_INT, &status);
                    temp_offset += (*data)[i].vect_size;
#else // BUILD_WITH_MPI
                    fwrite((*data)[i].thresholds[t], sizeof(int), (*data)[i].vect_size, f);
#endif // BUILD_WITH_MPI
                }
            }
#ifdef BUILD_WITH_MPI
            MPI_File_close (&f);
#else // BUILD_WITH_MPI
            fclose(f);
#endif // BUILD_WITH_MPI
        }
    }

    if (options->quantile_op == 1)
    {
        int value;
        for (value=0; value<options->nb_quantiles; value++)
        {
            for (t=0; t<options->nb_time_steps; t++)
            {
                sprintf(file_name, "%s_quantile%g_%.*d", field, options->quantile_order[value], max_size_time, (int)t+1);
#ifdef BUILD_WITH_MPI
                MPI_File_open (comm_data->comm, file_name, MPI_MODE_CREATE|MPI_MODE_WRONLY, MPI_INFO_NULL, &f);
                temp_offset = 0;
#else // BUILD_WITH_MPI
                f = fopen(file_name, "wb");
#endif // BUILD_WITH_MPI
                for (i=0; i<comm_data->client_comm_size; i++)
                {
                    if ((*data)[i].vect_size > 0)
                    {
#ifdef BUILD_WITH_MPI
                        MPI_File_write_at (f, offset + temp_offset, (*data)[i].quantiles[t][value].quantile, (*data)[i].vect_size, MPI_DOUBLE, &status);
                        temp_offset += (*data)[i].vect_size;
#else // BUILD_WITH_MPI
                        fwrite((*data)[i].quantiles[t][value].quantile, sizeof(double), (*data)[i].vect_size, f);
#endif // BUILD_WITH_MPI
                    }
                }
#ifdef BUILD_WITH_MPI
                MPI_File_close (&f);
#else // BUILD_WITH_MPI
                fclose(f);
#endif // BUILD_WITH_MPI
            }
        }
    }

    if (options->sobol_op == 1)
    {
        int p;
        for (t=0; t<options->nb_time_steps; t++)
        {
            for (p=0; p<options->nb_parameters; p++)
            {
                sprintf(file_name, "%s_sobol_indices_%.*d.%d",field,  max_size_time, (int)t+1, p);
#ifdef BUILD_WITH_MPI
                MPI_File_open (comm_data->comm, file_name, MPI_MODE_CREATE|MPI_MODE_WRONLY, MPI_INFO_NULL, &f);
                temp_offset = 0;
#else // BUILD_WITH_MPI
                f = fopen(file_name, "wb");
#endif // BUILD_WITH_MPI
                for (i=0; i<comm_data->client_comm_size; i++)
                {
                    if ((*data)[i].vect_size > 0)
                    {
#ifdef BUILD_WITH_MPI
                        MPI_File_write_at (f, offset + temp_offset, (*data)[i].sobol_indices[t].sobol_martinez[p].first_order_values, (*data)[i].vect_size, MPI_INT, &status);
                        temp_offset += (*data)[i].vect_size;
#else // BUILD_WITH_MPI
                        fwrite((*data)[i].sobol_indices[t].sobol_martinez[p].first_order_values, sizeof(double), (*data)[i].vect_size, f);
#endif // BUILD_WITH_MPI
                    }
                }
#ifdef BUILD_WITH_MPI
                MPI_File_close (&f);
#else // BUILD_WITH_MPI
                fclose(f);
#endif // BUILD_WITH_MPI
            }
        }
        // total order
        for (t=0; t<options->nb_time_steps; t++)
        {
            for (p=0; p<options->nb_parameters; p++)
            {
                sprintf(file_name, "%s_sobol_total_indices_%.*d.%d",field,  max_size_time, (int)t+1, p);
#ifdef BUILD_WITH_MPI
                MPI_File_open (comm_data->comm, file_name, MPI_MODE_CREATE|MPI_MODE_WRONLY, MPI_INFO_NULL, &f);
                temp_offset = 0;
#else // BUILD_WITH_MPI
                f = fopen(file_name, "wb");
#endif // BUILD_WITH_MPI
                for (i=0; i<comm_data->client_comm_size; i++)
                {
                    if ((*data)[i].vect_size > 0)
                    {
#ifdef BUILD_WITH_MPI
                        MPI_File_write_at (f, offset + temp_offset, (*data)[i].sobol_indices[t].sobol_martinez[p].total_order_values, (*data)[i].vect_size, MPI_INT, &status);
                        temp_offset += (*data)[i].vect_size;
#else // BUILD_WITH_MPI
                        fwrite((*data)[i].sobol_indices[t].sobol_martinez[p].total_order_values, sizeof(double), (*data)[i].vect_size, f);
#endif // BUILD_WITH_MPI
                    }
                }
#ifdef BUILD_WITH_MPI
                MPI_File_close (&f);
#else // BUILD_WITH_MPI
                fclose(f);
#endif // BUILD_WITH_MPI
            }
        }
    }
    melissa_free (local_vect_sizes);
}

#ifdef TOTO
///**
// *******************************************************************************
// *
// * @ingroup melissa_output
// *
// * This function reads data from Ensight files
// *
// *******************************************************************************
// *
// * @param[in] *options
// * option structure
// *
// * @param[in] *comm_data
// * structure containing communications parameters
// *
// * @param[in] *in_vect
// * input vector
// *
// * @param[in] *local_vect_sizes
// * all local vector sises
// *
// * @param[in] *file_name
// * name of the file
// *
// *******************************************************************************/

//void read_ensight (melissa_options_t  *options,
//                   comm_data_t      *comm_data,
//                   double           *in_vect,
//                   int              *local_vect_sizes,
//                   char             *file_name)
//{
//#ifdef BUILD_WITH_MPI
//    long int    i, j, offset=0;
//    MPI_Status  status;
//#endif // BUILD_WITH_MPI
//    FILE*       f;
//    float      *r_buffer;
//    char        c_buffer[81];
//    int32_t     n;

//    if (comm_data->rank == 0)
//    {
//        r_buffer = malloc (global_vect_size * sizeof(float));
//    }
//    else
//    {
//        r_buffer = malloc (local_vect_sizes[comm_data->rank] * sizeof(float));
//    }

//    if (comm_data->rank == 0)
//    {
//        f = fopen(file_name, "r");
//        if (f == NULL) printf ("f = NULL\n");
//        fread (c_buffer, sizeof(char), 80, f);
//        fread (c_buffer, sizeof(char), 80, f);
//        fread (&n, sizeof(int32_t), 1, f);
//        fread (c_buffer, sizeof(char), 80, f);
//        fread (r_buffer, sizeof(float), global_vect_size, f);
//        fclose(f);
//    }
//#ifdef BUILD_WITH_MPI
//    if (comm_data->rank == 0)
//    {
//        offset = 0;
//        for (j=1; j<comm_data->comm_size; j++)
//        {
//            offset += local_vect_sizes[j];
//            MPI_Send (&r_buffer[offset], local_vect_sizes[j], MPI_FLOAT, j, j+121, comm_data->comm);
//        }
//    }
//    else
//    {
//        MPI_Recv(r_buffer, local_vect_sizes[comm_data->rank], MPI_FLOAT, 0, comm_data->rank+121, comm_data->comm, &status);
//    }

//    for (i=0; i<local_vect_sizes[comm_data->rank]; i++)
//    {
//        in_vect[i] = (double)r_buffer[i];
//    }
//    melissa_free (r_buffer);
//#endif // BUILD_WITH_MPI
//}
#endif // TOTO