#include <linux/printk.h>

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

// compares strings by EOL ('\0') or ('\x0A').
// returns 0 if equal.
int string_compare(const char* str1, const char* str2) {
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

void print_string(const char* str) {
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
