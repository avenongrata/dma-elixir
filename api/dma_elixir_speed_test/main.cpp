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
    std::string file_name = "/dev/dma_elixir_o_v1";
    int fd;
    int ret;
    int act = 0;
    int i;
    struct pollfd fds;

    /* open device file */
    fd = open_file(file_name, O_RDWR);
    if (fd < 0)
        exit(EXIT_FAILURE);

    for (int i = 0; i < ITERATION_COUNT; i++)
    {
        char write_buf[BUF_SIZE];
        char read_buf[BUF_SIZE];

        for (int j = 0; j < BUF_SIZE; j++)
            write_buf[j] = (char)j;

        ret = write(fd, write_buf, BUF_SIZE);
        if (ret != BUF_SIZE)
        {
            std::cout << "Write error" << std::endl;
            exit(1);
        }

        auto t1 = high_resolution_clock::now();
        ret = read(fd, read_buf, BUF_SIZE);
        auto t2 = high_resolution_clock::now();

        if (ret != BUF_SIZE)
        {
            std::cout << "Read error" << std::endl;
            exit(1);
        }
        if (check_data(read_buf, BUF_SIZE, 'k'))
        {
            std::cout << "Compare error" << std::endl;
            exit(1);
        }

        /* count time */
        duration<double, std::milli> ms_double = t2 - t1;
        compl_time += ms_double.count();
     }

    double it_count = ITERATION_COUNT * BUF_SIZE;
    double m_bits =  it_count / 131072; // 125000 was ??

    std::cout << "Speed: " << m_bits << "/"
              << compl_time / 1000 << std::endl
              << m_bits / (compl_time / 1000) << " Mbits/sec\n";

    return 0;
}

