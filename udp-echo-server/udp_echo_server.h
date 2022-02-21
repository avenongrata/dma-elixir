#ifndef UDP_ECHO_SERVER_H
#define UDP_ECHO_SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#include "test_msg.h"

/*---------------------------------------------------------------------------*/

#define INPUT_ARGUMENTS_MAX 5

/*---------------------------------------------------------------------------*/

/* macros to avoid malloc fail */
#define MEM_FAILED(ptr) if (ptr == NULL) {      \
    fprintf(stderr, "Can't malloc memmory\n");  \
    exit(EXIT_FAILURE); }

/*---------------------------------------------------------------------------*/

/* 131072 */
#define DURATION(pkg, pkg_size, time) (time == 0) ? 0 : \
    (((pkg * pkg_size) / 125000) / time)

/*---------------------------------------------------------------------------*/

unsigned long pkg_sent = 0;
unsigned long pkg_recv = 0;
double duration = 0;

/*---------------------------------------------------------------------------*/

struct args
{
    char * dst_ip;
    int dst_port;
    int src_port;
    int timeout;
};

/*---------------------------------------------------------------------------*/

struct d_time
{
    long sec;  /* seconds */
    long ns;   /* nanoseconds */

    /*-----------------------------------------------------------------------*/

    double (*diff)(struct d_time *, struct d_time *);
};

/*---------------------------------------------------------------------------*/

struct addrs_info
{
    void (*print)(struct args *);
    struct args * args;
};

void print_net_info(struct args * args)
{
    printf("Server created on %s:%d\n", "127.0.0.1", args->src_port);
    printf("Resend to %s:%d\n", args->dst_ip, args->dst_port);
}

/*---------------------------------------------------------------------------*/

double get_difference(struct d_time * start, struct d_time * end)
{
    double diff = 0.0;

    diff = (double)((double)end->sec) * ((double)1000000000) +
            ((double)end->ns) - ((double)start->sec) * ((double)1000000000) -
            ((double)start->ns);

    diff /= 1000000000;

    return diff;
}

/*---------------------------------------------------------------------------*/

void get_time(struct d_time * dt)
{
    struct timespec ts;

    clock_gettime(CLOCK_REALTIME, &ts);
    dt->sec = ts.tv_sec;
    dt->ns = ts.tv_nsec;
}

/*---------------------------------------------------------------------------*/

void clear_dtime(struct d_time * dtime)
{
    dtime->ns   = 0;
    dtime->diff = get_difference;
}

/*---------------------------------------------------------------------------*/

int check(char * pkg)
{
    unsigned long i;

    for (i = 0; i < BUF_SIZE; i++)
    {
        if (pkg[i] != msg[i])
        {
            printf("Message is incorrect\n");
            return -1;
        }
    }

    return 0;
}

/*---------------------------------------------------------------------------*/

void clear_output()
{
    int c;

    for (c = 0; c < 3; c++)
    {
        fputs("\e[1;1H\e[2J", stdout);
        rewind(stdout);
    }
}

/*---------------------------------------------------------------------------*/

void * print_thread(void * vargp)
{
    unsigned long t_recv;
    struct addrs_info * addrs_info;
    addrs_info = (struct addrs_info *) vargp;

    t_recv = 0;
    while (1)
    {
        /* packet lost */
        if (pkg_recv == t_recv)
        {
            pkg_recv = pkg_sent = duration = 0;
        }
        else
        {
            t_recv = pkg_recv;
        }

        addrs_info->print(addrs_info->args);
        printf("Recv: %ld\nSent: %ld\nBitrate: %f\n", pkg_recv, pkg_sent,
               DURATION(pkg_recv, BUF_SIZE, duration));

        sleep(1);
        clear_output();
    }
}

/*---------------------------------------------------------------------------*/

void show_statistic(void * argp)
{
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, print_thread, argp);
}

/*---------------------------------------------------------------------------*/

void help(void)
{
    printf(
                " UDP echo server v. 1.1                            \n\n"
                " Arguments:                                        \n"
                "    -da        Destination address                 \n"
                "    -dp        Destination port                    \n"
                "    -sp        Source port                         \n"
                "    -t         Sleep in ns before send back        \n"
                "    -h         Show help message and quit          \n\n");
            exit(EXIT_SUCCESS);
}

/*---------------------------------------------------------------------------*/

int get_args(int argc, char ** argv)
{
    const char * input_arg[INPUT_ARGUMENTS_MAX] =
    {
        "-da", "-dp", "-sp" , "-t", "-h"
    };

    unsigned int arg_bit = 0;

    /*-----------------------------------------------------------------------*/

    for (int i = 1; i < argc; i++)
    {
        for (int j = 0; j < INPUT_ARGUMENTS_MAX; j++)
        {
            if (!strcmp(argv[i], input_arg[j]))
            {
                if (j == 0)         /* argument -da */
                {
                    arg_bit |= 1;
                }
                else if (j == 1)    /* argument -dp */
                {
                    arg_bit |= 2;
                }
                else if (j == 2)    /* argument -sp */
                {
                    arg_bit |= 4;
                }
                else if (j == 3)    /* argument -sp */
                {
                    arg_bit |= 8;
                }
                else if (j == 4)    /* argument -h */
                {
                    arg_bit = 425;
                    break;
                }
            }
        }
    }

    /*-----------------------------------------------------------------------*/

    if (arg_bit != 425)
    {
        if (arg_bit != 0b111 && arg_bit != 0b1111)
        {
            help();
            exit(1);
        }
    }

    return arg_bit;
}

/*---------------------------------------------------------------------------*/

int get_dest_addr(int argc, char ** argv)
{
    for (int i = 1; i < argc; i++)
    {
        for (int j = 0; j < INPUT_ARGUMENTS_MAX; j++)
        {
            if (!strcmp(argv[i], "-da"))
            {
                ++i;
                if (i == argc)
                {
                    fprintf(stderr, "No destination address is specified\n"
                                    "Use argument -h for full details\n\n");
                    exit(EXIT_FAILURE);
                }

                return i;
            }
        }
    }

    return -1;
}

/*---------------------------------------------------------------------------*/

int get_dest_port(int argc, char ** argv)
{
    int port = 0;

    for (int i = 1; i < argc; i++)
    {
        for (int j = 0; j < INPUT_ARGUMENTS_MAX; j++)
        {
            if (!strcmp(argv[i], "-dp"))
            {
                ++i;
                if (i == argc)
                {
                    fprintf(stderr, "No destination address is specified\n"
                                    "Use argument -h for full details\n\n");
                    exit(EXIT_FAILURE);
                }

                port = atoi(argv[i]);
                if (!port)
                {
                    fprintf(stderr, "No destination port is specified\n"
                                    "Use argument -h for full details\n\n");
                    exit(EXIT_FAILURE);
                }

                return port;
            }
        }
    }

    return -1;
}

/*---------------------------------------------------------------------------*/

int get_src_port(int argc, char ** argv)
{
    int port = 0;

    for (int i = 1; i < argc; i++)
    {
        for (int j = 0; j < INPUT_ARGUMENTS_MAX; j++)
        {
            if (!strcmp(argv[i], "-sp"))
            {
                ++i;
                if (i == argc)
                {
                    fprintf(stderr, "No destination address is specified\n"
                                    "Use argument -h for full details\n\n");
                    exit(EXIT_FAILURE);
                }

                port = atoi(argv[i]);
                if (!port)
                {
                    fprintf(stderr, "No destination port is specified\n"
                                    "Use argument -h for full details\n\n");
                    exit(EXIT_FAILURE);
                }

                return port;
            }
        }
    }

    return -1;
}

/*---------------------------------------------------------------------------*/

int get_timeout(int argc, char ** argv)
{
    int timeout = 0;

    for (int i = 1; i < argc; i++)
    {
        for (int j = 0; j < INPUT_ARGUMENTS_MAX; j++)
        {
            if (!strcmp(argv[i], "-t"))
            {
                ++i;
                if (i == argc)
                {
                    fprintf(stderr, "No timeout is specified. Set to "
                                    "default 0\n"
                                    "Use argument -h for full details\n\n");
                    exit(EXIT_FAILURE);
                }

                timeout = atoi(argv[i]);
                return timeout;
            }
        }
    }

    return -1;
}

/*---------------------------------------------------------------------------*/

void clear_args(struct args * args)
{
    args->dst_ip = NULL;
    args->dst_port = -1;
    args->src_port = -1;
    args->timeout = 0;
}


/*---------------------------------------------------------------------------*/

struct args parse_args(int argc, char ** argv, unsigned int arg_bit)
{
    struct args args;
    clear_args(&args);

    /*-----------------------------------------------------------------------*/

    // show help message and quit
    if (arg_bit == 425)
    {
        help();
        exit(0);
    }

    /*-----------------------------------------------------------------------*/

    // argument -da
    if (arg_bit & 1)
        args.dst_ip = argv[get_dest_addr(argc, argv)];

    /*-----------------------------------------------------------------------*/

    // argument -dp
    if (arg_bit & 2)
        args.dst_port = get_dest_port(argc, argv);


    /*-----------------------------------------------------------------------*/

    // argument -sp
    if (arg_bit & 4)
        args.src_port = get_src_port(argc, argv);

    /*-----------------------------------------------------------------------*/

    // argument -t
    if (arg_bit & 8)
        args.timeout = get_timeout(argc, argv);

    /*-----------------------------------------------------------------------*/

    return args;
}

/*---------------------------------------------------------------------------*/

#endif  // UDP_ECHO_SERVER_H
