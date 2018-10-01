#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/buffer_head.h>
#include <asm/segment.h>
#include <asm/uaccess.h>

#include "../include/kfs.h"
#include "../include/util.h"

static dev_t first;
static struct cdev c_dev;
static struct class *cl;
static struct file *work_file;
static int counter;

static int my_open(struct inode *i, struct file *f) {
    return 0;
}

static int my_close(struct inode *i, struct file *f) {
    return 0;
}

// "ssize" stands for "signed size"
static ssize_t my_read(struct file *f, char __user *buf, size_t len,
                       loff_t *off) {
    // if not opened, try to open and after reading close.
    // If doesn't exists, print err msg to ring buffer.
    if (!work_file) {
        work_file = file_open("/media/sf_D_DRIVE/work_file",  O_RDONLY, 0);
        if (work_file <= 0) {
            printk(KERN_ERR "work_file reading: work_file doesn't exist! [%d]\n", __LINE__);
        } else {
            // init block
            unsigned char buf[256];
            int i;
            for (i = 0; i < 256; i++)
                buf[i] = 0;

            // read and print
            file_read(work_file, 0, buf, 256);
            printk(KERN_INFO "work_file reading: %s [%d]\n", buf, __LINE__);

            // close resources and print message
            file_close(work_file);
            work_file = 0;
            printk(KERN_INFO "close file: file is closed. [%d]\n", __LINE__);
        }
    } else { // file is opened already --> print actual contents
        // init block
        unsigned char buf[256];
        int i;
        for (i = 0; i < 256; i++)
            buf[i] = 0;

        // read and print
        file_read(work_file, 0, buf, 256);
        printk(KERN_INFO "work_file reading: %s [%d]\n", buf, __LINE__);
    }
    return 0;
}

static ssize_t my_write(struct file *f, const char __user *buf,
                        size_t len, loff_t *off) {
    const char* str_open = "open work_file\x0A";
    const char* str_close = "close\x0A";

    // update counter. (-1) in the statement disclude trailing \x0A.
    counter += len - 1;

    if (string_compare(buf, str_open) == 0) {
        // create if not used earlier.
        // printk(KERN_DEBUG "open file: strings are equal. [%d]\n", __LINE__);
        if (!work_file)
            work_file = file_open("/media/sf_D_DRIVE/work_file", O_CREAT | O_RDWR, 0777);
        else
            printk(KERN_ERR "file_open: file is already opened! [%d]\n", __LINE__);
    } else if (string_compare(buf, str_close) == 0) {
        // printk(KERN_DEBUG "close file: strings are equal. [%d]\n", __LINE__);
        if (work_file <= 0)
            printk(KERN_ERR "close file: file is not opened! [%d]\n", __LINE__);
        else {
            char string_of_int[16];
            int len_of_int = int2str(string_of_int, counter);
            file_write(work_file, 0, string_of_int, len_of_int);
            file_close(work_file);
            work_file = 0;
            printk(KERN_INFO "close file: file is closed. [%d]\n", __LINE__);
        }
    } else
        printk(KERN_DEBUG "my_write(): String is not command. [%d]\n", __LINE__);
    return len;
}

static struct file_operations mychdev_fops =
{
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_close,
    .read = my_read,
    .write = my_write
};

static int __init ch_drv_init(void)
{
    printk(KERN_DEBUG "Hello! [%d]\n", __LINE__);
    // tell kernel which device numbers will be used for this module.
    if (alloc_chrdev_region(&first, 0, 1, "ch_dev") < 0)
    {
        return -1;
    }
    if ((cl = class_create(THIS_MODULE, "chardrv")) == NULL)
    {
        unregister_chrdev_region(first, 1);
        return -1;
    }
    if (device_create(cl, NULL, first, NULL, "var1") == NULL)
    {
        class_destroy(cl);
        unregister_chrdev_region(first, 1);
        return -1;
    }
    cdev_init(&c_dev, &mychdev_fops);
    if (cdev_add(&c_dev, first, 1) == -1)
    {
        device_destroy(cl, first);
        class_destroy(cl);
        unregister_chrdev_region(first, 1);
        return -1;
    }
    return 0;
}

static void __exit ch_drv_exit(void)
{
    cdev_del(&c_dev);
    device_destroy(cl, first);
    class_destroy(cl);
    unregister_chrdev_region(first, 1);
    printk(KERN_DEBUG "Bye!!! [%d]\n", __LINE__);
}

module_init(ch_drv_init);
module_exit(ch_drv_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Author");
MODULE_DESCRIPTION("The first kernel module");
