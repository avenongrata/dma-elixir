#ifndef ELIXIR_TEST_H
#define ELIXIR_TEST_H

#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

extern int BUF_LEN;

/*---------------------------------------------------------------------------*/

int dma_write(int fd)
{
    int i;
    char write_buf[BUF_LEN];
    int ret;

    /*-----------------------------------------------------------------------*/

    /* fill the buffer */
    for (i = 0; i < BUF_LEN; i++)
    {
        write_buf[i] = (char) i;
    }

    /*-----------------------------------------------------------------------*/

#ifdef DEBUG_OUTPUT
    for (i = 0; i < BUF_LEN; i++)
        printf("Write_buf[%d] = %d\n", i, write_buf[i]);
#endif

    /*-----------------------------------------------------------------------*/

    /* write to the file */
    ret = write(fd, write_buf, BUF_LEN);
    if (ret != BUF_LEN)
    {
        printf("Returned less than needed - %d/%d\n", ret, BUF_LEN);
        exit(0);
    }

    /*-----------------------------------------------------------------------*/

    return 0;
}

/*---------------------------------------------------------------------------*/

int dma_read(int fd)
{
    char arr[BUF_LEN];
    int i;
    int ret;

    /*-----------------------------------------------------------------------*/

    /* free the buffer */
    memset(&arr, '\0', BUF_LEN);

    /*-----------------------------------------------------------------------*/

#ifdef DEBUG_OUTPUT
    for (i = 0; i < BUF_LEN; i++)
        printf("arr[%d] = %d\n", i, arr[i]);
#endif

    /*-----------------------------------------------------------------------*/

    /* read from the file */
    ret = read(fd, arr, BUF_LEN);
    if (ret != BUF_LEN)
    {
        printf("Returned less than needed - %d/%d\n", ret, BUF_LEN);
        exit(0);
    }

    /*-----------------------------------------------------------------------*/

    /* check for correctness */
    for (i = 0; i < BUF_LEN; i++)
    {
        assert(arr[i] != (char) i);
    }

    /*-----------------------------------------------------------------------*/

    return 0;
}

#endif // ELIXIR_TEST_H
