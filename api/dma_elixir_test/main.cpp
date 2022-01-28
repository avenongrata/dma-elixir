#include <iostream>
#include <thread>
#include <cstring>
#include <dma_elixir.h>
#include <string>
#include <unistd.h>
#include <stdio.h>

using namespace std;

int main(int argc, char ** argv)
{
//    create_test_file();
//    exit(0);
    std::string file_name = "/dev/dma_elixir_o_v1";
    int fd;
    int ret;
    int act = 0;
    int i;
    char write_buf[BUF_SIZE];
    char read_buf[BUF_SIZE];
    struct pollfd fds;

    for (i = 0; i < BUF_SIZE; i++)
        write_buf[i] = (char)i;

    if (argc < 2)
    {
        std::cout << "Need to choose: read {1} / write {2}\n";
        exit(0);
    }

    /* get the count of transactions */
    act = atoi(argv[1]);

    /* open device file */
    fd = open_file(file_name, O_RDWR);
    if (fd < 0)
        exit(EXIT_FAILURE);

//    int mask = POLLOUT | POLLIN;
//    fds.fd = fd;
//    fds.events = mask;


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
//            if (fds.revents & POLLOUT)
//            {
//                ret = write(fds.fd, write_buf, BUF_SIZE);
////                if (ret != (BUF_SIZE))
////                {
////                    std::cout << "Writed less than needed\n";
////                    //global_rd_err++;
////                    return -1;
////                }

//                //fds.revents = 0;
//                //sleep(1);
//            }

//            if (fds.revents & POLLIN)
//            {
//                ret = read(fds.fd, read_buf, BUF_SIZE);
////                if (ret != (BUF_SIZE))
////                {
////                    std::cout << "Writed less than needed\n";
////                    //global_rd_err++;
////                    return -1;
////                }

//                //fds.revents = 0;
//                //sleep(1);
//            }

//            fds.revents = 0;
//        }
//    }

//    for (int i = 0; i < ITERATION_COUNT; i++)
//    {

//        char write_buf[BUF_SIZE];
//        char read_buf[BUF_SIZE];

//        for (int j = 0; j < BUF_SIZE; j++)
//            write_buf[j] = (char)j;

//        ret = write(fd, write_buf, BUF_SIZE);
//        if (ret != BUF_SIZE)
//        {
//            std::cout << "Write error" << std::endl;
//            exit(1);
//        }
//        sleep(2);

//        auto t1 = high_resolution_clock::now();
//        ret = read(fd, read_buf, BUF_SIZE);
//        auto t2 = high_resolution_clock::now();

//        if (ret != BUF_SIZE)
//        {
//            std::cout << "Read error" << std::endl;
//            exit(1);
//        }
//        if (check_data(read_buf, BUF_SIZE, 'k'))
//        {
//            std::cout << "Compare error" << std::endl;
//            exit(1);
//        }

//        /* count time */
//        duration<double, std::milli> ms_double = t2 - t1;
//        compl_time += ms_double.count();
//     }

//    double it_count = ITERATION_COUNT * BUF_SIZE;
//    double m_bits =  it_count / 125000;

//    std::cout << "Speed: " << m_bits << "/"
//              << compl_time / 1000 << std::endl
//              << m_bits / (compl_time / 1000) << " Mbits/sec\n";



    switch(act)
    {
    case 1:
        /* read from device file */
        ret = read_data(fd);
        if (ret)
            std::cout << "There is " << ret << " errors in dma read occurred\n";
        else
            std::cout << "There is no errors by transactions" << std::endl;
        break;

    case 2:
        ret = write_data(fd);
        if (ret < 0)
            std::cout << "There is an error in write function\n";
        break;

    default:
        std::cout << "There is no such action\n";
        exit(EXIT_FAILURE);
    }

    /* count time */

    return 0;
}

