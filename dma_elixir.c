#include <linux/dmaengine.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>
#include <linux/of_dma.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/spinlock_types.h>

//------------------------------------------------------------------------------

static DECLARE_WAIT_QUEUE_HEAD(dma_write_wait);
spinlock_t write_queue_lock;
int write_flag = 0;

//------------------------------------------------------------------------------

#define IP_CORE_DEBUG
#ifdef IP_CORE_DEBUG
#define ipcore_debug(msg) {printk(KERN_INFO "(%s) - %s\n", __func__, msg);}
#else
#define ipcore_debug(msg) {}
#endif

//------------------------------------------------------------------------------

#define DMA_DEBUG
#ifdef DMA_DEBUG
#define dma_debug(msg, dir) \
{\
    printk(KERN_INFO "%s [%s] - %s\n", (dir == DMA_FROM_DEVICE) ? \
                "* * [rx]" : "# # {tx}",  __func__, msg);\
}
#else
#define dma_debug(msg, dir) {}
#endif

//------------------------------------------------------------------------------

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wimplicit-function-declaration"
#endif

//------------------------------------------------------------------------------

#define DRIVER_NAME         "dma_elixir"
#define DRIVER_NAME_LEN     128
#define DEVICE_COUNT        1

//------------------------------------------------------------------------------

static unsigned tx_timeout = 30000;
module_param(tx_timeout, int, 0444);
MODULE_PARM_DESC(tx_timeout, "set the DMA TX timeout");

static unsigned rx_timeout = 30000;
module_param(rx_timeout, int, 0444);
MODULE_PARM_DESC(rx_timeout, "set the DMA RX timeout");

//------------------------------------------------------------------------------

struct ip_core_dchan
{
    struct dma_chan * dchan;    /* dma support */
    struct completion cmp;      /* to hold the state of dma transaction */
    struct sg_table * sg;       /* scatter-gather support */
    struct page **pinned_pages; /* to pin user-land pages */

    dma_cookie_t cookie;        /* to check the progress of DMA engine activity */
    u32 dma_direction;          /* dma direction */

    int timeout;                /* wait-timeout for dma transaction */
};

//------------------------------------------------------------------------------

struct ip_core_dma
{
    struct ip_core_dchan * dma_tx;    /* tx channel for dma */
    struct ip_core_dchan * dma_rx;    /* rx channel for dma */

    struct device * dt_device;        /* device created from the device tree */
    struct device * dma_device;       /* device associated with char_device */
    struct cdev char_dev;             /* char device */
    dev_t devt;                       /* char device number */
};

//------------------------------------------------------------------------------

struct class * ip_core_dclass;

//------------------------------------------------------------------------------

/* set default values for main structure */
static void dma_clean(struct ip_core_dma * dma)
{
    ipcore_debug("start");
    dma->dma_tx        = NULL;
    dma->dma_rx        = NULL;
    dma->dt_device     = NULL;
    dma->dma_device    = NULL;
    dma->devt          = 0x0U;
    ipcore_debug("end");
}

//------------------------------------------------------------------------------

/* set default values for the dma channel */
static void chan_clean(struct ip_core_dchan * chan)
{
    ipcore_debug("start");
    chan->dchan         = NULL;
    chan->sg            = NULL;
    chan->pinned_pages  = NULL;
    chan->dma_direction = DMA_TRANS_NONE;
    chan->timeout       = 0x0U;
    ipcore_debug("end");
}

//------------------------------------------------------------------------------

static void sync_callback(void * completion)
{
    complete(completion);
}

//------------------------------------------------------------------------------

enum DMA_CLEAN
{
    FREE_PAGES      = 1,
    UNPIN_PAGES     = 2,
    FREE_SG         = 3,
    FREE_SG_TABLE   = 4,
    UNMAP_SG        = 5
};

void dma_cleanup(enum DMA_CLEAN idx, struct ip_core_dchan * dchan, ...)
{
    dma_debug("start", dchan->dma_direction);
    int i;
    va_list vargs;
    long pinned;

    switch (idx)
    {
    case UNMAP_SG:
        dma_unmap_sg(dchan->dchan->device->dev, dchan->sg->sgl,
                     dchan->sg->nents, dchan->dma_direction);
        __attribute__((fallthrough));

    case FREE_SG_TABLE:
        sg_free_table(dchan->sg);
        __attribute__((fallthrough));

    case FREE_SG:
        kfree(dchan->sg);
        __attribute__((fallthrough));

    case UNPIN_PAGES:
        va_start(vargs, dchan);
        pinned = va_arg(vargs, int);

        for (i = 0; i < pinned; i++)
            put_page(dchan->pinned_pages[i]);

        va_end(vargs);
        __attribute__((fallthrough));

    case FREE_PAGES:
        kfree(dchan->pinned_pages);
        break;

    default:
        printk(KERN_INFO "There is no such clean-index");
        break;
    }
    dma_debug("end", dchan->dma_direction);
}

//------------------------------------------------------------------------------

/**
 * wait_for_transfer - wait for DMA transfer
 * @chan:  contains info about dma channel
 *
 * Return zero on success, error otherwise.
*/
static long wait_for_transfer(struct ip_core_dchan * chan)
{
    dma_debug("start", chan->dma_direction);
    unsigned long ret;
    enum dma_status status;

    /* wait for the transaction to complete, or timeout, or get an error */
    ret = wait_for_completion_timeout(&chan->cmp,
                                      msecs_to_jiffies(chan->timeout));

    if (chan->dma_direction == DMA_TO_DEVICE)
    {
        spin_lock(&write_queue_lock);
        write_flag = 1;
        spin_unlock(&write_queue_lock);
    }
    else
    {
        spin_lock(&write_queue_lock);
        write_flag = 0;
        spin_unlock(&write_queue_lock);
    }

    /* wake_up_interruptible(); */
    wake_up(&dma_write_wait);

    if (ret == 0)
        dma_debug("DMA timed out", chan->dma_direction);

    status = dma_async_is_tx_complete(chan->dchan, chan->cookie, NULL, NULL);
    if (status != DMA_COMPLETE)
    {
        printk(KERN_ERR "DMA returned completion callback status of: %s\n",
               status == DMA_ERROR ? "error" : "in progress");
        return -ENOSYS;
    }

    dma_debug("end", chan->dma_direction);
    return 0;
}

//------------------------------------------------------------------------------

/**
 * prepare_sg - prepare sg table for DMA transaction
 * @chan:  contains info about dma channel
 * @offset:  offset in user-land page
 * @pinned:  number of user-land pinned pages
 * @len:  size of the user buffer
 *
 * Is success zero returned, error otherwise.
 */
struct dma_async_tx_descriptor * prepare_sg(struct ip_core_dchan * chan,
                                        long offset, long pinned, size_t len)
{
    dma_debug("start", chan->dma_direction);
    struct dma_chan * dchan = chan->dchan;
    struct dma_async_tx_descriptor * txd = NULL;
    struct device * dev = dchan->device->dev;
    enum dma_ctrl_flags flags = DMA_CTRL_ACK | DMA_PREP_INTERRUPT;
    enum dma_transfer_direction d_direction = DMA_TRANS_NONE;
    int err;

    chan->sg = kmalloc(sizeof(struct sg_table), GFP_ATOMIC);
    if(IS_ERR(chan->sg))
    {
        dev_err(dev, "Can't allocate memory for sg table\n");
        dma_cleanup(UNPIN_PAGES, chan, pinned);
        return NULL;
    }

    err = sg_alloc_table_from_pages(chan->sg, chan->pinned_pages, pinned,
                                    offset, len, GFP_ATOMIC);
    if(err < 0)
    {
        dev_err(dev, "Unable to create sg table\n");
        dma_cleanup(FREE_SG, chan, pinned);
        return NULL;
    }

    /* can be less than nents, why so ? */
    err = dma_map_sg(dev, chan->sg->sgl, chan->sg->nents, chan->dma_direction);
    if(err == 0)
    {
        dev_err(dev, "Unable to map user buffer to sg table\n");
        dma_cleanup(FREE_SG_TABLE, chan, pinned);
        return NULL;
    }

    d_direction = (chan->dma_direction == DMA_TO_DEVICE) ?
                      DMA_MEM_TO_DEV : DMA_DEV_TO_MEM;

    txd = dchan->device->device_prep_slave_sg(dchan, chan->sg->sgl,
                                              chan->sg->nents, d_direction,
                                              flags, NULL);
    if (IS_ERR(txd))
    {
        dev_err(dev, "Can't prepare slave sg\n");
        dma_cleanup(UNMAP_SG, chan, pinned);
        return NULL;
    }

    dma_debug("end", chan->dma_direction);
    return txd;
}

//------------------------------------------------------------------------------

/**
 * transfer - start DMA transaction
 * @chan:  contains info about dma channel
 *
 * Return zero on success, error otherwise.
 */
long transfer(struct ip_core_dchan * chan,
              struct dma_async_tx_descriptor * txd)
{
    dma_debug("start", chan->dma_direction);

    struct dma_chan * dchan = chan->dchan;
    struct device * dev = dchan->device->dev;

    init_completion(&chan->cmp);
    txd->callback = sync_callback;
    txd->callback_param = &chan->cmp;
    chan->cookie = dmaengine_submit(txd);

    if (dma_submit_error(chan->cookie))
    {
        dev_err(dev, "Can't submit transaction\n");
        return -ENOSYS;
    }

    /*
     * Start the DMA transaction which was previously queued up
     * in the DMA engine
     */
    dma_async_issue_pending(dchan);

    dma_debug("end", chan->dma_direction);
    return 0;
}

//------------------------------------------------------------------------------

/**
 * pin_pages: - pin user-land pages in physical memory
 * @chan:  contains info about dma channel
 * @buf:  user-land buffer
 * @len:  size of the user buffer
 *
 * If success pinned pages are returned, error otherwise.
 */
long pin_pages(struct ip_core_dchan * chan, const char __user * buf,
               size_t len)
{
    dma_debug("start", chan->dma_direction);

    struct dma_chan * dchan = chan->dchan;
    struct device * dev = dchan->device->dev;

    int pinned;
    int i;
    long alloc_pages;
    ulong first, last;

    size_t pg_mask  = (size_t) PAGE_MASK;
    size_t pg_shift = (size_t) PAGE_SHIFT;

    size_t buf_first = (size_t)buf;
    size_t buf_last  = (size_t)(buf + len - 1);

    first  = (buf_first & pg_mask) >> pg_shift;
    last   = (buf_last  & pg_mask) >> pg_shift;

    alloc_pages = (last - first) + 1;

    /* use just kmalloc ? */
    chan->pinned_pages = kzalloc((alloc_pages * (sizeof(struct page *))),
                               GFP_ATOMIC);
    if (IS_ERR(chan->pinned_pages))
    {
        dev_err(dev, "Unable to allocate memory for pages\n");
        return PTR_ERR(chan->pinned_pages);
    }

    /* try here get_user_pages / pin_user_pages */
    /* use 0 instead of 1 */
    pinned = get_user_pages_fast((ulong)buf, alloc_pages, 1,
                                 chan->pinned_pages);
    if (pinned < 0)
    {
        dev_err(dev, "Unable to pin user pages\n");
        dma_cleanup(FREE_PAGES, chan);
        return -ENOSYS;
    }
    else if (pinned < alloc_pages)
    {
        dev_err(dev, "Only pinned few user pages %d\n", pinned);
        /* for pin_user_pages need to use unpin_user_pages */
        dma_cleanup(UNPIN_PAGES, chan, pinned);
        return -ENOSYS;
    }

    dma_debug("end", chan->dma_direction);
    return pinned;
}

//------------------------------------------------------------------------------

long start_transfer(struct ip_core_dchan * chan, const char __user * buf,
                    size_t len)
{
    dma_debug("start", chan->dma_direction);
    long offset;
    long pinned;
    long ret;
    struct dma_async_tx_descriptor * txd;

    /* get offset in user-land page */
    offset = offset_in_page(buf);

    /* pin user-land pages in physical memory */
    pinned = pin_pages(chan, buf, len);
    if (IS_ERR_VALUE(offset))
        return offset;

    /* prepare sg for transaction */
    txd = prepare_sg(chan, offset, pinned, len);
    if (txd == NULL)
        return -ENOSYS;

    /* start DMA transaction */
    ret = transfer(chan, txd);
    if (IS_ERR_VALUE(ret))
        return ret;

    /* wait for DMA transfer */
    ret = wait_for_transfer(chan);
    if (IS_ERR_VALUE(ret))
        return ret;

    /* call dma cleanup */
    dma_cleanup(UNMAP_SG, chan, pinned);

    dma_debug("end", chan->dma_direction);
    return len;
}

//------------------------------------------------------------------------------

static int ip_core_dma_open(struct inode * inode, struct file * file)
{
    ipcore_debug("start");
    file->private_data = container_of(inode->i_cdev,
                                      struct ip_core_dma,
                                      char_dev);
    ipcore_debug("end");

    return 0;
}

//------------------------------------------------------------------------------

static int ip_core_dma_release(struct inode * inode, struct file * file)
{
    ipcore_debug("start");
    (void)inode;

    file->private_data = NULL;
    ipcore_debug("end");

    return 0;
}

//------------------------------------------------------------------------------

static ssize_t ip_core_dma_read(struct file * filp, char __user * buffer,
                            size_t length, loff_t * pos)
{
    ipcore_debug("start");
    (void) pos;
    struct ip_core_dma * dma = filp->private_data;
    long ret;

    ret = start_transfer(dma->dma_rx, buffer, length);
    if (IS_ERR_VALUE(ret))
    {
        printk(KERN_INFO "$$$ $$$ There is some error occured "
                "in read function: %ld\n", ret);
        return ret;
    }

    ipcore_debug("end");

    return length;
}

//------------------------------------------------------------------------------

static ssize_t ip_core_dma_write(struct file * filp, const char __user * buffer,
                            size_t length, loff_t * pos)
{
    ipcore_debug("start");
    (void) pos;
    struct ip_core_dma * dma = filp->private_data;
    long ret;

    ret = start_transfer(dma->dma_tx, buffer, length);
    if (IS_ERR_VALUE(ret))
    {
        printk(KERN_INFO "$$$ $$$ There is some error occured "
                    "in write function: %ld\n", ret);
        return ret;
    }

    ipcore_debug("end");

    return length;
}

//------------------------------------------------------------------------------

static unsigned int ip_core_dma_poll(struct file *filp, poll_table *wait)
{
    ipcore_debug("start");
    int mask = 0;

    poll_wait(filp, &dma_write_wait, wait);

    spin_lock(&write_queue_lock);
    mask |= (write_flag) ? POLLIN : POLLOUT;
    spin_unlock(&write_queue_lock);

    ipcore_debug("end");
    return mask;
}


//------------------------------------------------------------------------------

static struct file_operations dma_fops =
{
    .owner          = THIS_MODULE,
    .open           = ip_core_dma_open,
    .read           = ip_core_dma_read,
    .write          = ip_core_dma_write,
    .release        = ip_core_dma_release,
    .poll           = ip_core_dma_poll
};

//------------------------------------------------------------------------------

enum IP_CORE_CLEAN
{
    IP_CORE_DRVDATA       = 1,
    IP_CORE_REGION_CLEAN  = 2,
    IP_CORE_DEV_DESTROY   = 3,
    IP_CORE_DMADATA       = 4,
    IP_CORE_CDEV_DEL      = 5,
    IP_CORE_CHAN_DEL      = 6
};

void ip_core_cleanup(enum IP_CORE_CLEAN idx, struct ip_core_dma * dma)
{
    ipcore_debug("start");
    switch (idx)
    {
    case IP_CORE_CHAN_DEL:;
        if (dma->dma_tx)
        {
            dma->dma_tx->dchan->device->
                    device_terminate_all(dma->dma_tx->dchan);
            dma_release_channel(dma->dma_tx->dchan);
        }
        if (dma->dma_rx)
        {
            dma->dma_rx->dchan->device->
                    device_terminate_all(dma->dma_rx->dchan);
            dma_release_channel(dma->dma_rx->dchan);
        }
        __attribute__((fallthrough));

    case IP_CORE_CDEV_DEL:
        cdev_del(&dma->char_dev);
        __attribute__((fallthrough));

    case IP_CORE_DMADATA:
        dev_set_drvdata(dma->dma_device, NULL);
        __attribute__((fallthrough));

    case IP_CORE_DEV_DESTROY:
        device_destroy(ip_core_dclass, dma->devt);
        __attribute__((fallthrough));

    case IP_CORE_REGION_CLEAN:
        unregister_chrdev_region(dma->devt, DEVICE_COUNT);
        __attribute__((fallthrough));

    case IP_CORE_DRVDATA:
        dev_set_drvdata(dma->dt_device , NULL);
        break;

    default:
        dev_err(dma->dt_device, "There is no such clean-index");
        break;
    }
    ipcore_debug("end");
}

//------------------------------------------------------------------------------

/*
 * Create a DMA channel by getting a DMA channel from the DMA Engine
 * and then setting up the channel as a character device to allow
 * user space control.
 */
static int create_channel(struct device * dev, struct ip_core_dchan * chan,
                          char * name, u32 direction)
{
    ipcore_debug("start");
    /*
     * Request the DMA channel from the DMA engine and then use the device from
     * the channel for the proxy channel also.
     */
    chan->dchan = dma_request_slave_channel(dev, name);
    if (!chan->dchan)
    {
        dev_err(dev, "DMA channel request error\n");
        return PTR_ERR(chan->dchan);
    }

    /* set DMA direction for channel */
    chan->dma_direction = direction;
    ipcore_debug("end");

    return 0;
}

//------------------------------------------------------------------------------

static char * create_device_name(struct ip_core_dma * dma)
{
    char * device_name;
    char * tmp_device_name;
    ipcore_debug("start");

    /* create unique device name */
    device_name = devm_kmalloc(dma->dt_device, DRIVER_NAME_LEN, GFP_KERNEL);
    if (IS_ERR(device_name))
    {
        dev_err(dma->dt_device, "Can' allocate memory for device name");
        ip_core_cleanup(IP_CORE_DRVDATA, dma);
        return NULL;
    }

    //-------------------------------------------------------------------------

    tmp_device_name = kmalloc(DRIVER_NAME_LEN, GFP_KERNEL);
    if (IS_ERR(tmp_device_name))
    {
        dev_err(dma->dt_device, "Can' allocate memory for device name");
        ip_core_cleanup(IP_CORE_DRVDATA, dma);
        return NULL;
    }

    //-------------------------------------------------------------------------

    // when there is no address -> make another name (need to check ..->start ?)
    snprintf(tmp_device_name, DRIVER_NAME_LEN, "_io_v1");
    snprintf(device_name, DRIVER_NAME_LEN, "%s_%s",
             DRIVER_NAME, &tmp_device_name[2]);

    // delete memory
    kfree(tmp_device_name);
    dev_dbg(dma->dt_device, "device name [%s]\n", device_name);
    ipcore_debug("end");

    return device_name;
}

//------------------------------------------------------------------------------

static int init_char_device(struct ip_core_dma * dma, const char * device_name)
{
    int rc;
    ipcore_debug("start");
    /* allocate device number */
    rc = alloc_chrdev_region(&dma->devt, 0, DEVICE_COUNT, device_name);
    if (rc < 0)
    {
        dev_err(dma->dt_device, "Can't allocate region");
        ip_core_cleanup(IP_CORE_DRVDATA, dma);
        return rc;
    }
    else
    {
        dev_dbg(dma->dt_device, "allocated device number major %i minor %i\n",
                MAJOR(dma->devt), MINOR(dma->devt));
    }


    /* create driver file */
    /* can I use "%s", device_name, wtf ?? */
    dma->dma_device = device_create(ip_core_dclass, NULL,
                                    dma->devt, NULL, device_name);

    if (IS_ERR(dma->dma_device))
    {
        dev_err(dma->dt_device, "couldn't create driver file -> %s\n",
                device_name);
        ip_core_cleanup(IP_CORE_REGION_CLEAN, dma);
        return PTR_ERR(dma->dma_device);
    }
    else
    {
        dev_set_drvdata(dma->dma_device, dma);
    }

    /* create character device */
    cdev_init(&dma->char_dev, &dma_fops);
    rc = cdev_add(&dma->char_dev, dma->devt, 1);

    if (rc < 0)
    {
        dev_err(dma->dt_device, "couldn't create character device\n");
        ip_core_cleanup(IP_CORE_DMADATA, dma);
        return rc;
    }
    ipcore_debug("end");

    return 0;
}

//------------------------------------------------------------------------------

static int init_dma_channels(struct ip_core_dma * dma)
{
    int rc;
    ipcore_debug("start");

    /* allocate memory for the dma channels */
    dma->dma_rx = devm_kmalloc(dma->dt_device, sizeof(struct ip_core_dchan),
                               GFP_KERNEL);
    dma->dma_tx = devm_kmalloc(dma->dt_device, sizeof(struct ip_core_dchan),
                               GFP_KERNEL);

    if (!dma->dma_tx || !dma->dma_rx)
    {
        dev_err(dma->dt_device, "Can't allocate memory for the dma channels");
        ip_core_cleanup(IP_CORE_CDEV_DEL, dma);
        return -ENOMEM;
    }
    else
    {
        chan_clean(dma->dma_tx);
        chan_clean(dma->dma_rx);
    }

    //-------------------------------------------------------------------------

    /*
     * Create the channels in the dma. The direction does not matter
     * as the DMA channel has it inside it and uses it, other than this
     * will not work for cyclic mode.
     */

    /* TX channel */
    rc = create_channel(dma->dt_device, dma->dma_tx, "tx_channel",
                        DMA_TO_DEVICE);
    if (rc)
    {
        printk(KERN_INFO "Can't create tx channel");
        ip_core_cleanup(IP_CORE_CDEV_DEL, dma);
        return rc;
    }

    /* set timeout for DMA waiting function */
    dma->dma_tx->timeout = tx_timeout;


    //-------------------------------------------------------------------------

    /* RX channel */
    rc = create_channel(dma->dt_device, dma->dma_rx, "rx_channel",
                        DMA_FROM_DEVICE);
    if (rc)
    {
        printk(KERN_INFO "Can't create rx channel");
        ip_core_cleanup(IP_CORE_CHAN_DEL, dma);
        return rc;
    }

    /* set timeout for DMA waiting function */
    dma->dma_rx->timeout = rx_timeout;

    //-------------------------------------------------------------------------

    ipcore_debug("end");
    return 0;
}

//------------------------------------------------------------------------------

/* initialize the dma elixir device driver module */
static int ip_core_dma_probe(struct platform_device * pdev)
{
    int rc;
    struct ip_core_dma * dma;
    char * device_name;
    ipcore_debug("start");

    printk(KERN_INFO "dma_elixir module initialized\n");

    //-------------------------------------------------------------------------

    dma = devm_kmalloc(&pdev->dev, sizeof(struct ip_core_dma), GFP_KERNEL);
    if (IS_ERR(dma))
    {
        dev_err(&pdev->dev, "Cound not allocate memory for device\n");
        return PTR_ERR(dma);
    }
    else
    {
        dma_clean(dma);
        dev_set_drvdata(&pdev->dev, dma);
        dma->dt_device = &pdev->dev;
    }

    //-------------------------------------------------------------------------

    /* initialize global variables */
    write_flag = 0x0U;
    spin_lock_init(&write_queue_lock);

    //-------------------------------------------------------------------------

    /* create device name */
    device_name = create_device_name(dma);
    if (device_name == NULL)
        return -ENOMEM;

    //--------------------------------------------------------------------------

    /* create and init char device */
    rc = init_char_device(dma, device_name);
    if (rc < 0)
        return rc;

    //--------------------------------------------------------------------------

    /* create and init dma channels */
    rc = init_dma_channels(dma);
    if (rc < 0)
        return rc;

    //-------------------------------------------------------------------------

    ipcore_debug("end");
    return 0;
}

//------------------------------------------------------------------------------

/* exit the dma elixir driver module */
static int ip_core_dma_remove(struct platform_device *pdev)
{
    struct device * dev      = &pdev->dev;
    struct ip_core_dma * dma = dev_get_drvdata(dev);
    ipcore_debug("start");

    ip_core_cleanup(IP_CORE_CHAN_DEL, dma);
    printk(KERN_INFO "dma_elixir module exited\n");

    ipcore_debug("end");

    return 0;
}

//------------------------------------------------------------------------------

static const struct of_device_id ip_core_dma_of_ids[] =
{
    { .compatible = "xlnx,axi-dma-test-ngrt"},
    {}
};

//------------------------------------------------------------------------------

static struct platform_driver ip_core_dma_driver =
{
    .driver =
    {
        .name           = DRIVER_NAME,
        .owner          = THIS_MODULE,
        .of_match_table = ip_core_dma_of_ids,
    },
    .probe  = ip_core_dma_probe,
    .remove = ip_core_dma_remove,
};

//------------------------------------------------------------------------------

static int __init ip_core_dma_init(void)
{
    ipcore_debug("start");
    ip_core_dclass = class_create(THIS_MODULE, DRIVER_NAME);
    if (IS_ERR(ip_core_dclass))
    {
        printk(KERN_INFO "Unable to create class\n");
        return PTR_ERR(ip_core_dclass);
    }

    ipcore_debug("end");
    return platform_driver_register(&ip_core_dma_driver);
}

//------------------------------------------------------------------------------

static void __exit ip_core_dma_exit(void)
{
    ipcore_debug("start");
    platform_driver_unregister(&ip_core_dma_driver);
    class_destroy(ip_core_dclass);
    ipcore_debug("end");
}

//------------------------------------------------------------------------------

module_init(ip_core_dma_init)
module_exit(ip_core_dma_exit)

MODULE_AUTHOR("Yustitskii Kirill");
MODULE_DESCRIPTION("DMA elixir");
MODULE_LICENSE("GPL");
