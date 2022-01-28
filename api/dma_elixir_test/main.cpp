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

