
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <math.h>
#ifdef BUILD_WITH_ZMQ
#include <zmq.h>
#endif // BUILD_WITH_ZMQ
#include "stats.h"
#include "server.h"

#define ZEROCOPY

static volatile int end_signal = 0;

void sig_handler(int signo) {
    if (signo == SIGUSR1 || signo == SIGINT || signo == SIGUSR2)
        end_signal = signo;
}

int main (int argc, char **argv)
{
    stats_options_t     stats_options;
    stats_data_t       *data_ptr = NULL;
    field_ptr           field = NULL;
    comm_data_t         comm_data;
    int                 time_step;
    int                 recv_buff_size, i, j, ret, client_rank;
    char               *buffer, *buf_ptr;
    double            **buff_tab_ptr;
    int                 iteration = 0, nb_iterations = 0;
    int                 port_no;
    char                port_name[MPI_MAX_PROCESSOR_NAME] = {0};
    char               *node_names;
    int                 sinit_tab[3], rinit_tab[2];
    void               *context = zmq_ctx_new ();
    char                node_name[MPI_MAX_PROCESSOR_NAME];
    void               *connexion_responder = zmq_socket (context, ZMQ_REP);
    void               *init_responder = zmq_socket (context, ZMQ_REP);
    void               *data_puller = zmq_socket (context, ZMQ_PULL);
#ifdef BUILD_WITH_PY_ZMQ
    void               *python_requester = zmq_socket (context, ZMQ_REQ);
#endif // BUILD_WITH_PY_ZMQ
    int                 nb_fields = 0;
    int                 first_init = 1;
    int                 first_connect = 1;
    int                 get_next_message = 0;
    int                *client_vect_sizes, *local_vect_sizes;
    pull_data_t         pull_data;
    int                 nb_bufferized_messages = 10;
    char               *field_name_ptr;
    int                 simu_id, group_id;
    int                 nb_converged_fields = 0;
    double              interval1, interval_tot;
    zmq_msg_t           msg;

#ifdef BUILD_WITH_MPI
    // === init MPI === //

    MPI_Init (&argc, &argv);
    comm_data.comm = MPI_COMM_WORLD;
    MPI_Comm_size(comm_data.comm, &comm_data.comm_size);
    MPI_Comm_rank (comm_data.comm, &comm_data.rank);
#else
    comm_data.comm_size       = 1;
    comm_data.rank            = 0;
#endif // BUILD_WITH_MPI

    // === Get the node adress === //

    melissa_get_node_name (node_name);

#ifdef BUILD_WITH_PROBES
    start_time = melissa_get_time();
#endif // BUILD_WITH_PROBES

    // === Install signal handler === //

    if (signal(SIGINT, sig_handler) == SIG_ERR)
            printf("\ncan't catch SIGINT\n");
    if (signal(SIGUSR1, sig_handler) == SIG_ERR)
            printf("\ncan't catch SIGUSR1\n");
    if (signal(SIGUSR2, sig_handler) == SIG_ERR)
            printf("\ncan't catch SIGUSR2\n");

    // == Read options from command line === //

    stats_get_options (argc, argv, &stats_options);
    if (comm_data.rank == 0)
    {
        print_options (&stats_options);
        write_options (&stats_options);
    }
    nb_iterations = stats_options.nb_groups * stats_options.nb_time_steps ;

    // === Open data puller port === //

    port_no = 100 + comm_data.rank;
    sprintf (port_name, "tcp://*:11%d", port_no);
    zmq_setsockopt (data_puller, ZMQ_RCVHWM, &nb_bufferized_messages, sizeof(int));
    melissa_bind (data_puller, port_name);
#ifdef BUILD_WITH_PY_ZMQ
    if(stats_options.sobol_op == 1)
    {
        sprintf (port_name, "tcp://localhost:5555");
        melissa_connect (python_requester, port_name);
    }
#endif // BUILD_WITH_PY_ZMQ

    if (comm_data.rank == 0)
    {
        melissa_bind (init_responder, "tcp://*:2002");
        melissa_bind (connexion_responder, "tcp://*:2003");

        node_names = melissa_malloc (MPI_MAX_PROCESSOR_NAME * comm_data.comm_size);

        first_connect = 2;
        first_init = 2;
    }

    // === Restart initialisation === //

    if (stats_options.restart == 1)
    {
        first_connect = 1;
        first_init = 0;
        fprintf (stdout, "reading data files...");
        if (comm_data.rank == 0)
        {
            read_client_data(&comm_data.client_comm_size, &client_vect_sizes);
        }
        MPI_Bcast(&comm_data.client_comm_size, 1, MPI_INT, 0, comm_data.comm);
        if (comm_data.rank != 0)
        {
            client_vect_sizes = melissa_malloc (comm_data.client_comm_size * sizeof(int));
        }
        fprintf (stdout, " ok \n");

    }

    // === Gather node names on node 0 === //

#ifdef BUILD_WITH_MPI
    MPI_Gather(node_name, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, node_names, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, 0, comm_data.comm);
#else // BUILD_WITH_MPI
    memcpy (node_names, node_name, MPI_MAX_PROCESSOR_NAME);
#endif // BUILD_WITH_MPI

    sinit_tab[0] = comm_data.comm_size;
    sinit_tab[1] = stats_options.sobol_op;
    sinit_tab[2] = stats_options.nb_parameters;

    // === Main loop === //

    while (1)
    {
#ifdef BUILD_WITH_PROBES
        start_wait_time = melissa_get_time();
#endif // BUILD_WITH_PROBES
        zmq_pollitem_t items [] = {
            { connexion_responder, 0, ZMQ_POLLIN, 0 },
            { data_puller, 0, ZMQ_POLLIN, 0 }
        };
        zmq_poll (items, 2, 100);
#ifdef BUILD_WITH_PROBES
        end_wait_time = melissa_get_time();
        total_wait_time += end_wait_time - start_wait_time;
#endif // BUILD_WITH_PROBES

        // === If message on the connexion port === //

        if (items[0].revents & ZMQ_POLLIN)
        {
            if (comm_data.rank == 0)
            {
#ifdef BUILD_WITH_PROBES
                start_comm_time = melissa_get_time();
#endif // BUILD_WITH_PROBES
                // new simulation wants to connect
                zmq_recv (connexion_responder, rinit_tab, 2 * sizeof(int), 0);
                zmq_send (connexion_responder, sinit_tab, 3 * sizeof(int), 0);
                get_next_message = 1;
                if (first_init == 2)
                {
                    first_init = 1;
                }
#ifdef BUILD_WITH_PROBES
                end_comm_time = melissa_get_time();
                total_comm_time += end_comm_time - start_comm_time;
#endif // BUILD_WITH_PROBES
            }
        }

        // === Only after the first connexion, broadcast client info === //

        if (first_init == 1)
        {
#ifdef BUILD_WITH_MPI
            MPI_Bcast(rinit_tab, 2, MPI_INT, 0, comm_data.comm);
#endif // BUILD_WITH_MPI
            comm_data.client_comm_size = rinit_tab[0];
            client_vect_sizes = melissa_malloc (comm_data.client_comm_size * sizeof(int));
            first_init = 0;
        }

        // === Second part of the connexion message, only for rank 0 === //

        if (get_next_message == 1)
        {
#ifdef BUILD_WITH_PROBES
            start_comm_time = melissa_get_time();
#endif // BUILD_WITH_PROBES
            // new simulation wants to connect, step two

            zmq_recv (init_responder, client_vect_sizes, comm_data.client_comm_size * sizeof(int), 0);
            zmq_send (init_responder, node_names, comm_data.comm_size * MPI_MAX_PROCESSOR_NAME * sizeof(char), 0);

            get_next_message = 0;
            if (first_connect == 2)
            {
                first_connect = 1;
            }
#ifdef BUILD_WITH_PROBES
            end_comm_time = melissa_get_time();
            total_comm_time += end_comm_time - start_comm_time;
#endif // BUILD_WITH_PROBES
            write_client_data (&comm_data.client_comm_size, client_vect_sizes);
        }

        // === Melissa structures initialisation, after the second part of the first connexion === //

        if (first_connect == 1 && first_init == 0)
        {
#ifdef BUILD_WITH_MPI
            MPI_Bcast(client_vect_sizes, comm_data.client_comm_size, MPI_INT, 0, comm_data.comm);
#endif // BUILD_WITH_MPI
            stats_options.global_vect_size = 0;
            for (i=0; i< comm_data.client_comm_size; i++)
            {
                stats_options.global_vect_size += client_vect_sizes[i];
            }
            local_vect_sizes = melissa_malloc (comm_data.comm_size * sizeof(int));
            for (i=0; i<comm_data.comm_size; i++)
            {
                local_vect_sizes[i] = stats_options.global_vect_size / comm_data.comm_size;
                if (i < stats_options.global_vect_size % comm_data.comm_size)
                    local_vect_sizes[i] += 1;
            }

            comm_data.rcounts = melissa_calloc (comm_data.client_comm_size, sizeof(int));
            comm_data.rdispls = melissa_calloc (comm_data.client_comm_size, sizeof(int));

            comm_n_to_m_init (comm_data.rcounts,
                              comm_data.rdispls,
                              stats_options.global_vect_size,
                              local_vect_sizes,
                              client_vect_sizes,
                              comm_data.client_comm_size,
                              comm_data.rank,
                              &pull_data);
            nb_iterations *= pull_data.local_nb_messages;
            recv_buff_size = pull_data.buff_size * sizeof(double);
            if (stats_options.sobol_op == 1)
            {
                recv_buff_size *= stats_options.nb_parameters+2;
            }
            recv_buff_size += MAX_FIELD_NAME * sizeof(char) + 4 * sizeof(int);
            buffer = melissa_malloc (recv_buff_size);

            if (stats_options.sobol_op == 1)
            {
                buff_tab_ptr = melissa_malloc ((stats_options.nb_parameters + 2) * sizeof(double*));
            }
            else
            {
                buff_tab_ptr = melissa_malloc (sizeof(double*));
            }
            first_connect = 0;
        }

        // === Data reception and statistics computation === //

        if (items[1].revents & ZMQ_POLLIN)
        {
#ifdef BUILD_WITH_PROBES
            start_comm_time = melissa_get_time();
#endif // BUILD_WITH_PROBES
#ifdef ZEROCOPY
            zmq_msg_init (&msg);
            zmq_msg_recv (&msg, data_puller, 0);
            buf_ptr = zmq_msg_data (&msg);
#else // ZEROCOPY
            zmq_recv (data_puller, buffer, recv_buff_size, 0);
            buf_ptr = buffer;
#endif // ZEROCOPY
#ifdef BUILD_WITH_PROBES
            end_comm_time = melissa_get_time();
            total_comm_time += end_comm_time - start_comm_time;
#endif // BUILD_WITH_PROBES

            time_step = *buf_ptr;
            buf_ptr += sizeof(int);
            simu_id = *buf_ptr;
            buf_ptr += sizeof(int);
            group_id = *buf_ptr;
            buf_ptr += sizeof(int);
            client_rank = *buf_ptr;
            buf_ptr += sizeof(int);
            field_name_ptr = buf_ptr;
            if (field == NULL)
            {
                add_field(&field, field_name_ptr, comm_data.client_comm_size);
                data_ptr = get_data_ptr (field, field_name_ptr);
                nb_fields += 1;
            }
            else
            {
                data_ptr = get_data_ptr (field, field_name_ptr);
                if (data_ptr == NULL)
                {
                    add_field(&field, field_name_ptr, comm_data.client_comm_size);
                    data_ptr = get_data_ptr (field, field_name_ptr);
                    nb_fields += 1;
                }
            }
            if (data_ptr[client_rank].is_valid != 1)
            {
                init_data (&data_ptr[client_rank], &stats_options, comm_data.rcounts[client_rank]);
                if (stats_options.restart == 1)
                {
                    fprintf (stdout, "reading checkpoint files...");
                    read_saved_stats (data_ptr, &comm_data, field_name_ptr, client_rank);
                    fprintf (stdout, " ok\n");
                }
            }
            buf_ptr += MAX_FIELD_NAME * sizeof(char);
#ifdef BUILD_WITH_PROBES
#ifdef ZEROCOPY
            total_bytes_recv += zmq_msg_size (&msg);
#else // ZEROCOPY
            total_bytes_recv += recv_buff_size;
#endif // ZEROCOPY
            start_computation_time = melissa_get_time();
#endif // BUILD_WITH_PROBES

            if (stats_options.sobol_op != 1)
            {
                buff_tab_ptr[0] = (double*)buf_ptr;
                // === Compute classical statistics === //
                compute_stats (&data_ptr[client_rank], time_step-1, 1, buff_tab_ptr);
                iteration++;
            }
            else
            {
                for (i=0; i<stats_options.nb_parameters+2; i++)
                {
                    buff_tab_ptr[i] = (double*)buf_ptr;
                    buf_ptr += comm_data.rcounts[client_rank] * sizeof(double);
                }
                // === Compute classical statistics + Sobol indices === //
                compute_stats (&data_ptr[client_rank], time_step-1, stats_options.nb_parameters+2, buff_tab_ptr);
                iteration++;
                confidence_sobol_martinez (&(data_ptr[client_rank].sobol_indices[time_step-1]), stats_options.nb_parameters, data_ptr[client_rank].vect_size);
                nb_converged_fields += check_convergence_sobol_martinez(&(data_ptr[client_rank].sobol_indices),
                                                                        0.01,
                                                                        stats_options.nb_time_steps,
                                                                        stats_options.nb_parameters);
            }
#ifdef BUILD_WITH_PROBES
            end_computation_time = melissa_get_time();
            total_computation_time += end_computation_time - start_computation_time;
#endif // BUILD_WITH_PROBES
            if (comm_data.rank==0 && ((iteration % 10) == 0 || iteration < 10) )
            {
                fprintf(stdout, "iteration %d / %d  - field \"%s\"\n", iteration, nb_iterations*nb_fields, field_name_ptr);
            }
#ifdef BUILD_WITH_PY_ZMQ
            sprintf (port_name, "iteration %d", comm_data.rank);
            zmq_send(python_requester, port_name, strlen(port_name) * sizeof(char), 0);
            string_recv(python_requester, port_name);
#endif // BUILD_WITH_PY_ZMQ
#ifdef ZEROCOPY
            for (i=0; i<sizeof(buff_tab_ptr)/sizeof(double*); i++)
            {
                buff_tab_ptr[i] = NULL;
            }
            buf_ptr = NULL;
            zmq_msg_close (&msg);
#endif // ZEROCOPY
        }

#ifdef BUILD_WITH_PY_ZMQ
        // === Send a message to the Python master in case of Sobol indices convergence === //
        if (nb_converged_fields >= nb_fields * pull_data.local_nb_messages && stats_options.sobol_op == 1)
        {
            sprintf (port_name, "converged %d", comm_data.rank);
            zmq_send(python_requester, port_name, strlen(port_name) * sizeof(char), 0);
            string_recv(python_requester, port_name);
            if (strcmp ("stop", port_name))
            {
                break;
            }
        }
#endif // BUILD_WITH_PY_ZMQ

        // === Signal handling === //

        if (end_signal == SIGINT)
        {
            field_ptr fptr = field;
            if (comm_data.rank == 0)
                fprintf (stderr, "\nINTERUPTED at iteration %d \n", iteration);
            while (fptr != NULL)
            {
                save_stats (fptr->stats_data, &comm_data, fptr->name);
                fptr = field->next;
                if (comm_data.rank == 0)
                {
                    char dir[256];
                    getcwd(dir, 256*sizeof(char));
                    fprintf(stderr, "statistic fields saved in %s\n\n", dir);
                }
            }
            MPI_Finalize ();
            return 0;
            break;
        }
        else if (end_signal == SIGUSR1 || end_signal == SIGUSR2)
        {
            break;
        }

#ifdef BUILD_WITH_PY_ZMQ
        if (nb_fields > 0 && stats_options.sobol_op == 0)
#else  // BUILD_WITH_PY_ZMQ
        if (nb_fields > 0)
#endif // BUILD_WITH_PY_ZMQ
        {
            if (iteration >= nb_iterations*nb_fields)
            {
                break;
            }
        }
    }


    if (end_signal == 0)
    {

        // === Sockets deconnexion === //

        zmq_close (connexion_responder);
        zmq_close (init_responder);
        zmq_close (data_puller);
#ifdef BUILD_WITH_PY_ZMQ
        if (stats_options.sobol_op == 1)
        {
            sprintf (port_name, "finished %d", comm_data.rank);
            zmq_send(python_requester, port_name, strlen(port_name) * sizeof(char), 0);
            string_recv(python_requester, port_name);
            zmq_close (python_requester);
        }
#endif // BUILD_WITH_PY_ZMQ
        zmq_ctx_term (context);
    }
    melissa_free (buff_tab_ptr);

    if (comm_data.rank == 0)
    {
        melissa_free(node_names);
    }

    interval1 = 0;
    interval_tot = 0;
    if (stats_options.sobol_op == 1)
    {
        global_confidence_sobol_martinez (field, &comm_data, &interval1, &interval_tot);
    }

    if (end_signal == 0)
    {
        finalize_field_data (field, &comm_data, &pull_data, &stats_options, local_vect_sizes
#ifdef BUILD_WITH_PROBES
                            , &total_write_time
#endif // BUILD_WITH_PROBES
                            );
    }
    melissa_free (buffer);
    melissa_free (comm_data.rcounts);
    melissa_free (comm_data.rdispls);

#ifdef BUILD_WITH_PROBES
#ifdef BUILD_WITH_MPI
    double temp1;
    long int temp2;
    MPI_Reduce (&interval1, &temp1, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    interval1 = temp1;
    MPI_Reduce (&interval_tot, &temp1, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    interval_tot = temp1;
    MPI_Reduce (&total_comm_time, &temp1, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    total_comm_time = temp1 / comm_data.comm_size;
    MPI_Reduce (&total_bytes_recv, &temp2, 1, MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
    total_bytes_recv = temp2;
#endif // BUILD_WITH_MPI
    if (comm_data.rank==0)
    {
        fprintf (stdout, " --- Number of simulations:           %d\n", stats_options.nb_simu);
        fprintf (stdout, " --- Number of simulation cores:      %d\n", comm_data.client_comm_size);
        fprintf (stdout, " --- Number of analysis cores:        %d\n", comm_data.comm_size);
        fprintf (stdout, " --- Average communication time:      %g s\n", total_comm_time);
        fprintf (stdout, " --- Calcul time:                     %g s\n", total_computation_time);
        fprintf (stdout, " --- Waiting time:                    %g s\n", total_wait_time);
        fprintf (stdout, " --- Writing time:                    %g s\n", total_write_time);
        fprintf (stdout, " --- Total time:                      %g s\n", melissa_get_time() - start_time);
        fprintf (stdout, " --- Bytes recieved:                  %ld bytes\n",total_bytes_recv);
        fprintf (stdout, " --- Stats structures memory:         %ld bytes\n", mem_conso(&stats_options));
        fprintf (stdout, " --- Bytes written:                   %ld bytes\n", count_bytes_written(&stats_options)*nb_fields);
        if (stats_options.sobol_op == 1)
        {
            fprintf (stdout, " --- Worst Sobol confidence interval: %g (first order)\n", interval1);
            fprintf (stdout, " --- Worst Sobol confidence interval: %g (total order)\n", interval_tot);
        }
        fprintf (stdout, "\n");
    }
#endif // BUILD_WITH_PROBES

#ifdef BUILD_WITH_MPI
    MPI_Finalize ();
#endif // BUILD_WITH_MPI

    return 0;
}
