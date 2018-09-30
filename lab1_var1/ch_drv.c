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

static dev_t first;
static struct cdev c_dev;
static struct class *cl;
static struct file *work_file;
static int counter;

// compares strings by EOL ('\0') or ('\x0A').
// returns 0 if equal.
static int string_compare(const char* str1, const char* str2) {
    while(*str1 == *str2) {
        if(*str1 == '\0' || *str2 == '\0' || *str1 == '\x0A' || *str2 == '\x0A')
            break;
        str1++;
        str2++;
    }
    if ((*str1 == '\0' && *str2 == '\0') || (*str1 == '\x0A' && *str2 == '\x0A'))
        return 0;
    else
        return -1;
}

static void print_string(const char* str) {
    int code = *str;
    int i = 0;

    printk(KERN_DEBUG "%s\n", str);
    while (code != 0 && code != 10) {
        printk(KERN_DEBUG "[%d] = %d", i, code);
        str++;
        code = *str;
        i++;
    }
}

struct file *file_open(const char *path, int flags, int rights) {
    struct file *filp = NULL;
    mm_segment_t oldfs;
    int err = 0;

    oldfs = get_fs();
    set_fs(get_ds());
    filp = filp_open(path, flags, rights);
    set_fs(oldfs);
    // If we've not opened file.
    if (IS_ERR(filp)) {
        printk(KERN_DEBUG "file_open(): file is not opened. [%d]\n", __LINE__);
        err = PTR_ERR(filp);
        return NULL;
    }
    printk(KERN_DEBUG "file_open(): file opened. [%d]\n", __LINE__);
    return filp;
}

void file_close(struct file *file) {
    filp_close(file, NULL);
}

int file_read(struct file *file, unsigned long long offset,
    unsigned char *data, unsigned int size) {
    mm_segment_t oldfs;
    int ret;

    oldfs = get_fs();
    set_fs(get_ds());

    ret = vfs_read(file, data, size, &offset);

    set_fs(oldfs);
    return ret;
}

int file_write(struct file *file, unsigned long long offset,
    unsigned char *data, unsigned int size) {
    mm_segment_t oldfs;
    int ret;

    oldfs = get_fs();
    set_fs(get_ds());

    ret = vfs_write(file, data, size, &offset);

    set_fs(oldfs);
    return ret;
}

int file_sync(struct file *file) {
    vfs_fsync(file, 0);
    return 0;
}

// the function to convert int to string. Used in order to pass in file_write().
// returns number of characters.
int int2str(char* str, int counter) {
    int len = 0;
    int i;

    // write in backward order.
    while (counter > 0) {
        str[len++] = '0' + counter % 10;
        counter /= 10;
    }
    // reverse chars in array.
    for (i = 0; i < len / 2; i++) {
        char w = str[i];
        str[i] = str[len - 1 - i];
        str[len - 1 - i] = w;
    }
    return len;
}

static int my_open(struct inode *i, struct file *f) {
    return 0;
}

static int my_close(struct inode *i, struct file *f) {
    return 0;
}

// "ssize" stands for "signed size"
// TODO: write my_read func.
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
