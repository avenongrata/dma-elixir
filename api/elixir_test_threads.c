#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>

#define BUF_LEN 2046

//------------------------------------------------------------------------------

void * _read_thread(void *);
void * _write_thread(void *);
int dma_write(int);
int dma_read(int);

//------------------------------------------------------------------------------

int main(int argc, char ** argv)
{
    int desc;
    char * device_file_name = "/dev/dma_elixir_o_v1";
    int trans_number;
    pthread_t tid;
    int write_arr[2];
    
    if (argc < 2)
    {
        printf("Specify number of transactions\n");
        exit(0);
    }

    /* get the count of transactions */
    trans_number = atoi(argv[1]);
    
    /* open device file */
    desc = open(device_file_name, O_RDWR);
    if (desc < 0)
    {
        printf("Can't open device file name\n");
        exit(0);
    }
    
    write_arr[0] = desc;
    write_arr[1] = trans_number;

    /* start the read thread */
    pthread_create(&tid, NULL, _read_thread, (void *)&desc);

    /* start the write thread */
    sleep(1);
    pthread_create(&tid, NULL, _write_thread, (void *)write_arr);
}

//------------------------------------------------------------------------------

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
    if (ret != BUF_LEN)
    {
     printf("Returned less than needed - %d/%d\n", ret, BUF_LEN);
     exit(0);
    }

    for (i = 0; i < BUF_LEN; i++)
    {
        if (16 != arr[i])
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

//------------------------------------------------------------------------------

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

//------------------------------------------------------------------------------

void * _read_thread(void * vargp)
{
    int *fd = (int *)vargp;

    while (1)
    {
        dma_read(*fd);
    }
}

//------------------------------------------------------------------------------

void * _write_thread(void * vargp)
{
    int * arr = (int *)vargp;
    int i;

    for (i = 0; i < 14663; i++)
    {
        dma_write(arr[0]);
        usleep(68);
    }

}
