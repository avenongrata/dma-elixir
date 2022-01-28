#pragma once
#include <stdio.h>
#include <iostream>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <chrono>

#define BUF_SIZE 128
#define BUF_KEY 'k'
#define ITERATION_COUNT 100000

int global_rd_err = 0;

using namespace std::chrono;
double compl_time = 0;

int open_file(std::string file_name, int act)
{
    int fd;

    fd = open(file_name.c_str(), act);
    if (fd < 0)
        std::cout << "Can't open file: " << file_name << std::endl;

    return fd;
}

int check_data(char *buf, int len, int key)
{
    for (int i = 0; i < len; i++)
        if (buf[i] != (char)i)
        {
            printf("## index %d, invalid value - %d\n", i, buf[i]);
            //return 1;
        }
//        else
//        {
//            printf("index %d, valid value - %d\n", i, buf[i]);
//        }

    //std::cout << "Dma complete\n";
    return 0;
}

int write_data(int fd)
{
    struct pollfd fds;
    char buf[BUF_SIZE];
    int ret;
    int i;

    printf("\n\n\n\n");
    /* fill the buffer */
    for (i = 0; i < BUF_SIZE; i++)
    {
        buf[i] = (char)i;
        //printf("buf[%d] = %d\n", i, buf[i]);
    }

//    for (i = 0; i < BUF_KEY; i++)
//        std::cout << buf[i] << std::endl;

    /* check for write-function */
    fds.fd = fd;
    fds.events = POLLOUT | POLLWRBAND;

    while (1)
    {
        ret = poll(&fds, 1, -1);
        if (ret < 0)
        {
            std::cout << "There is an error in poll function\n";
        }
        else if (ret == 0)
        {
            std::cout << "Timeout in poll function occurred\n";
        }
        else
        {
            if (fds.revents & (POLLOUT | POLLWRBAND))
            {
                ret = write(fds.fd, buf, BUF_SIZE);
                if (ret != BUF_SIZE)
                {
                    std::cout << "Writed less than needed\n";
                    //global_rd_err++;
                    return -1;
                }

                fds.revents = 0;
                //sleep(1);
            }
        }
    }

    return 0;
}

int read_data(int fd)
{
    char buf[BUF_SIZE];
    int ret;
    struct pollfd fds;

//    fds.fd = fd;
//    fds.events = POLLIN;

//    while (1)
//    {
//        ret = poll(&fds, 1, -1);
//        if (ret < 0)
//        {
//            std::cout << "There is an error in poll function\n";
//        }
//        else if (ret == 0)
//        {
//            std::cout << "Timeout in poll function occurred\n";
//        }
//        else
//        {
//            if (fds.revents & POLLIN)
//            {
//                ret = read(fds.fd, buf, BUF_SIZE);
//                if (ret != BUF_SIZE)
//                {
//                    std::cout << "Read less than needed" << ret << std::endl;
//                    //global_rd_err++;
//                    //return -1;
//                }

//                fds.revents = 0;
//            }
//        }
//    }

    for (int i = 0; i < ITERATION_COUNT; i++)
    {

        /* start timer */
        auto t1 = high_resolution_clock::now();
        ret = read(fd, buf, BUF_SIZE);
        /* end timer */
        auto t2 = high_resolution_clock::now();

        /* count time */
        duration<double, std::milli> ms_double = t2 - t1;
        compl_time += ms_double.count();

        if (ret != BUF_SIZE)
        {
            std::cout << "Returned less than needed\n";
            global_rd_err++;
            continue;
            //return -1;
        }
        /* check data for correctness */
        ret = check_data(buf, ret, i);
        if (ret)
            global_rd_err++;
    }

    double it_count = ITERATION_COUNT * BUF_SIZE;
    double m_bits =  it_count / 125000;

    std::cout << "Speed: " << m_bits << "/"
              << compl_time / 1000 << std::endl
              << m_bits / (compl_time / 1000) << " Mbits/sec\n";

    return global_rd_err;
}


int transfer(void)
{
    /* start timer */

    /* start read-thread */

    /* write to the file */

    /* wait for read-thread */

    /* stop timer */

    return 0;
}

void generate_file(int fd)
{
    long count;
    char sym;
    std::srand(std::time(nullptr));

    count = BUF_SIZE * ITERATION_COUNT;

    for (int i = 0; i < count; i++)
    {
        sym = (std::rand()%255);
        write(fd, &sym, 1);
    }

    close(fd);
}

int create_test_file()
{
    std::string file_name = "f2tt";
    int fd;

    fd = open_file(file_name, O_WRONLY);
    if (fd < 0)
    {
        std::cout << "Can't create file to test dma transaction\n";
        return -1;
    }

    generate_file(fd);

    return 0;

}


