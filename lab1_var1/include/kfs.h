#ifndef __KFS__H__
#define __KFS__H__

struct file *file_open(const char *path, int flags, int rights);
void file_close(struct file *file);
int file_read(struct file *file, unsigned long long offset,
    unsigned char *data, unsigned int size);
int file_write(struct file *file, unsigned long long offset,
    unsigned char *data, unsigned int size);
int file_sync(struct file *file);

#endif // __KFS__H__
