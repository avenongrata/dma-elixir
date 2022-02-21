#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <elixir_test.h>

/*---------------------------------------------------------------------------*/
/* use global varibales to change behavior of tests */

int BUF_LEN;
int BUF_SIZE;

/*---------------------------------------------------------------------------*/

int main(int argc, char ** argv)
{
    int desc;
    char * device_file_name = "/dev/dma_elixir_o_v1";
       
    /* open device file */
    desc = open(device_file_name, O_RDWR);
    if (desc < 0)
    {
        printf("Can't open device file name\n");
        exit(0);
    }
    

    return 0;
}
