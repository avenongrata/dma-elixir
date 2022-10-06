#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BUF_LEN 2046

int dma_write(int fd)
{
    int i;
    char write_buf[BUF_LEN];
    int ret;
    
    /* fill the buffer */
    for (i = 0; i < BUF_LEN; i++)
        write_buf[i] = 16;
    
    for (i = 0; i < BUF_LEN; i++)
        printf("Write_buf[%d] = %d\n", i, write_buf[i]);
    
    ret = write(fd, write_buf, BUF_LEN);
    if (ret != BUF_LEN)
    {
        printf("Returned less than needed - %d/%d\n", ret, BUF_LEN);
        exit(0);
    }
     
    return 0;    
}

int dma_read(int fd)
{
    char arr[BUF_LEN];
    int i;
    int ret;
    int test = 0;
    
    for (i = 0; i < BUF_LEN; i++)
        arr[i] = 1;
    
    for (i = 0; i < BUF_LEN; i++)
        printf("arr[%d] = %d\n", i, arr[i]);
    
    ret = read(fd, arr, BUF_LEN);

    for (i = 0; i < BUF_LEN; i++)
    {
        if (arr[i] != 16)
        {
            test++;
            printf("Index - %d, invalid value - %d\n", i, arr[i]);
        }
        else
            printf("%d\n", arr[i]);
    }
    
    printf("\n");
    
    if (test)
    {
        printf("There are %d errors in read function\n", test);
        exit(0);
    }
    else
        printf("DMA complited\n");
    
    return 0;
    
}

int main(int argc, char ** argv)
{
    int desc;
    char * device_file_name = "/dev/dma_elixir_o_v1";
    int operation = 0;
    
    if (argc < 2)
    {
        printf("Specify what you need to implement - read {1}/ write {2} operation\n");
        exit(0);
    }
    
    /* open device file */
    desc = open(device_file_name, O_RDWR);
    if (desc < 0)
    {
        printf("Can't open device file name\n");
        exit(0);
    }
    
    operation = atoi(argv[1]);
    printf("You choosed - %d\n", operation);
    
    switch(operation)
    {
    case 1:
        dma_read(desc);
        break;
    case 2:
        dma_write(desc);
        break;
    default:
        printf("Unknown key\n");
        exit(0);
        break;
            
    }

}
