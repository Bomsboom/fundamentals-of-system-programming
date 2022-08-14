#define _GNU_SOURCE
#define MAX_IPV4_BUFF_SIZE 32

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <ftw.h>

#include "plugin_api.h"


static char* g_lib_name = "lib.so";

static char* g_plugin_purpose = "Find files which contain given IPv4 address";

static char* g_plugin_author = "Pavel Ephremov";

char little_end[64] = { 0 };
char big_end[64] = { 0 };
int found = 0;
char target[MAX_IPV4_BUFF_SIZE];

//Option name
#define OPT_IPV4_ADDR_STR "ipv4-addr"


static struct plugin_option g_po_arr[] = {
    /*
        struct plugin_option {
            struct option {
               const char *name;
               int         has_arg;
               int        *flag;
               int         val;
            } opt,
            char *opt_descr
        }
    */
        {
            {
                OPT_IPV4_ADDR_STR,
                required_argument,
                0, 0,
            },
            "Target IPv4 address"
        },
};

static int g_po_arr_len = sizeof(g_po_arr) / sizeof(g_po_arr[0]);

//
//  Private functions
//
int atob(char*, char*, char*);
int search_in_file(FILE*);

//
//  API functions
//
int plugin_get_info(struct plugin_info* ppi) {
    if (!ppi) {
        fprintf(stderr, "ERROR: invalid argument\n");
        return -1;
    }

    ppi->plugin_purpose = g_plugin_purpose;
    ppi->plugin_author = g_plugin_author;
    ppi->sup_opts_len = g_po_arr_len;
    ppi->sup_opts = g_po_arr;

    return 0;
}

int plugin_process_file(const char* fname,
    struct option in_opts[],
    size_t in_opts_len) {

    // Return error by default
    int ret = -1;

    // Pointer to file mapping
    unsigned char* ptr = NULL;

    char* DEBUG = getenv("LAB1DEBUG");

    if (!fname || !in_opts || !in_opts_len) {
        errno = EINVAL;
        return -1;
    }

    if (DEBUG)
    {
        fprintf(stderr, "\nSearcing in file:\t'%s'\n", fname);
        fprintf(stderr, "DEBUG: %s: Got option '%s' with arg '%s'\n",
            g_lib_name, in_opts[0].name, (char*)in_opts[0].flag);
    }


    strncpy(target, (char*)in_opts[0].flag, MAX_IPV4_BUFF_SIZE);

    if (DEBUG)
    {
        fprintf(stderr, "DEBUG %s: got adress '%s'\n", g_lib_name, target);
    }

    int saved_errno = 0;

    FILE* fp = fopen(fname, "r");;

    int fd = open(fname, O_RDONLY);
    if (fd < 0) {
        // errno is set by open()
        return -1;
    }

    struct stat st = { 0 };
    int res = fstat(fd, &st);
    if (res < 0) {
        saved_errno = errno;
        goto END;
    }

    // Check that size of file is > 0
    if (st.st_size == 0) {
        if (DEBUG) {
            fprintf(stderr, "DEBUG: %s: File size should be > 0\n",
                g_lib_name);
        }
        saved_errno = ERANGE;
        goto END;
    }



    ptr = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (ptr == MAP_FAILED) {
        saved_errno = errno;
        goto END;
    }

    res = atob(target, little_end, big_end);

    if (res)
    {
        fprintf(stderr, "%s: Invalid IP address: '%s'\n", g_lib_name, target);
        ret = -1;
        goto END;
    }

    if (DEBUG)
    {
        fprintf(stderr, "DEBUG: %s: Converted address:\n\tlittle-endian: %s\n\tbig endian: %s\n",
            g_lib_name, little_end, big_end);
    }

    res = search_in_file(fp);

    if (res)
    {
        ret = 1;
        if (DEBUG)
        {
            fprintf(stderr, "DEBUG: %s:\nSample found in line: %d\n", g_lib_name, res);
        }
    }
    else ret = 0;

END:
    fclose(fp);
    close(fd);
    if (ptr != MAP_FAILED && ptr != NULL) munmap(ptr, st.st_size);

    //Restore errno value
    errno = saved_errno;

    return ret;
}


// Перевод адреса в бинарный вид с проверкой валидности адреса
int atob(char* addr, char* buf1, char* buf2)
{
    int len = strlen(addr);

    if (len < 7 || len > 15)
        return 1;

    char tail[32];
    tail[0] = 0;

    unsigned int bytes[4];
    int res = sscanf(addr, "%u.%u.%u.%u%s", &bytes[0], &bytes[1], &bytes[2], &bytes[3], tail);

    if (res != 4 || tail[0])
    {
        return 1;
    }

    for (int i = 0; i < 4; i++)
    {
        if (bytes[i] > 255)
            return 1;
    }

    unsigned int lit_sum = bytes[0] * pow(2, 24) + bytes[1] * pow(2, 16) + bytes[2] * pow(2, 8) + bytes[3];
    unsigned int big_sum = bytes[3] * pow(2, 24) + bytes[2] * pow(2, 16) + bytes[1] * pow(2, 8) + bytes[0];

    int count = 0;
    for (int i = 0; i < (int)sizeof(int) * 8; i++)
    {

        if (lit_sum & (1 << i))
        {
            buf1[31 - count] = '1';
        }
        else
        {
            buf1[31 - count] = '0';
        }

        if (big_sum & (1 << i))
        {
            buf2[31 - count] = '1';
        }
        else
        {
            buf2[31 - count] = '0';
        }

        count++;
    }

    return 0;

}

int search_in_file(FILE* file)
{

    char* line = NULL;
    size_t len = 0;
    ssize_t read;
    int line_num = 1;

    while ((read = getline(&line, &len, file)) != -1)
    {
        if (strstr(line, little_end) || strstr(line, big_end) || strstr(line, target))
        {
            free(line);
            return line_num;
        }
        line_num++;
    }

    free(line);
    return 0;
}

