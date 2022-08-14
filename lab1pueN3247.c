#define _GNU_SOURCE         
#define _XOPEN_SOURCE 500
#define MAX_FILE_NAME_LENGTH 256

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ftw.h>
#include <getopt.h>
#include <dlfcn.h>
#include <dirent.h>
#include <unistd.h>
#include "plugin_api.h"

int walk_func(const char*, const struct stat*, int, struct FTW*);

const char* get_file_extention(const char*);

struct plugin_to_pass
{
    void* dl;
    char* name;
    struct plugin_option* sup_long_opts;
    int sup_opts_len;
    struct option* opts_to_pass;
    int opts_to_pass_len;
    int used;
};

struct binary_options
{
    unsigned int AND;
    unsigned int OR;
    unsigned int NOT;
};

struct plugin_to_pass* plugins;
struct binary_options rules = { 0, 0, 0 };
struct option* longopts;
char* pdir;
int lib_count = 0;
int ffound = 0;

int main(int argc, char* argv[])
{
    char* wdir = strdup(argv[argc - 1]); // Рабочая директория
    pdir = get_current_dir_name(); // Стандартная директория для поиска плагинов

    char* DEBUG = getenv("LAB1DEBUG");

    if (argc == 1)
    {
        fprintf(stderr, "Usage: %s [[list of options] catalog]\nFor help enter flag -h\n", argv[0]);
        goto END;
    }

    // Сначала находим самые простые опции
    int pfound = 0;
    for (int i = 0; i < argc; i++)
    {

        if (strcmp("-v", argv[i]) == 0)
        {
            printf("Author: Pavel Ephremov\nGroup: N3247\nVariant: 3\n");
            goto END;
        }

        if (strcmp("-h", argv[i]) == 0)
        {
            printf("-P dir\tКаталог с плагинами.\n");
            printf("-A\tОбъединение опций плагинов с помощью операции «И» (действует по умолчанию)..\n");
            printf("-O\tОбъединение опций плагинов с помощью операции «ИЛИ».\n");
            printf("-N\tИнвертирование условия поиска (после объединения опций плагинов с помощью -A или -O).\n");
            printf("-v\tВывод версии программы и информации о программе (ФИО исполнителя, номер группы, номер варианта лабораторной).\n");
            printf("-h\tВывод справки по опциям.\n");
            goto END;

        }

        if (strcmp(argv[i], "-P") == 0)
        {
            if (pfound)
            {
                fprintf(stderr, "ERROR: expected only one option -- 'P'\n");
                goto END;
            }
            if (i == argc - 1)
            {
                fprintf(stderr, "ERROR: Expected last parameter to be directory\n");
                goto END;
            }
            if (opendir(argv[i + 1]) == NULL)
            {
                fprintf(stderr, "ERROR: Can't open directory: %s\n", argv[i + 1]);
                goto END;
            }
            else
            {
                pdir = strdup(argv[i + 1]);
                pfound = 1;
            }
        }
    }

    if (DEBUG)
    {
        fprintf(stderr, "LAB1DEBUG: Directory with plugins: %s\n", pdir);
    }

    DIR* d;
    struct dirent* dir;
    d = opendir(pdir);

    // Считаем количество обыкновенных файлов для выделения достаточного количества памяти под библиотеки
    if (d != NULL)
    {
        while ((dir = readdir(d)) != NULL)
        {
            if ((dir->d_type) == DT_REG)
            {
                lib_count++;
            }
        }
        closedir(d);
    }
    else
    {
        fprintf(stderr, "ERROR: opendir '%s'", pdir);
        goto END;
    }

    plugins = (struct plugin_to_pass*)malloc(lib_count * sizeof(struct plugin_to_pass));
    if (!plugins)
    {
        fprintf(stderr, "ERROR: malloc");
        goto END;
    }
    d = opendir(pdir);
    lib_count = 0;

    // Теперь находим все файлы с расширением so и пытаемся получить информацию о плагинах
    if (d != NULL)
    {
        while ((dir = readdir(d)) != NULL)
        {
            if ((dir->d_type) == DT_REG)
            {
                if (strcmp(get_file_extention(dir->d_name), "so") == 0)
                {
                    char lib_name[MAX_FILE_NAME_LENGTH+1];
                    snprintf(lib_name, sizeof lib_name, "%s/%s", pdir, dir->d_name);

                    void* dl = dlopen(lib_name, RTLD_LAZY);
                    if (!dl)
                    {
                        fprintf(stderr, "ERROR: dlopen() failed: %s\n", dlerror());
                        continue;
                    }
                    void* func = dlsym(dl, "plugin_get_info");
                    if (!func)
                    {
                        fprintf(stderr, "ERROR: dlsym() failed: %s\n", dlerror());
                    }
                    struct plugin_info pi = { 0 };
                    typedef int (*pgi_func_t)(struct plugin_info*);
                    pgi_func_t pgi_func = (pgi_func_t)func;

                    int ret = pgi_func(&pi);
                    if (ret < 0)
                    {
                        fprintf(stderr, "ERROR: plugin_get_info() failed: '%s'\n", dir->d_name);
                    }

                    plugins[lib_count].sup_opts_len = pi.sup_opts_len;
                    plugins[lib_count].name = dir->d_name;
                    plugins[lib_count].sup_long_opts = pi.sup_opts;
                    plugins[lib_count].dl = dl;
                    plugins[lib_count].opts_to_pass = calloc(pi.sup_opts_len, sizeof(struct option));
                    plugins[lib_count].opts_to_pass_len = 0;
                    plugins[lib_count].used = 0;

                    // Plugin info
                    fprintf(stdout, "\n///////////////////////////////////////////////////////////////\n");
                    fprintf(stdout, "Found plugin:\t\t%s\n", dir->d_name);
                    fprintf(stdout, "Plugin purpose:\t\t%s\n", pi.plugin_purpose);
                    fprintf(stdout, "Plugin author:\t\t%s\n", pi.plugin_author);
                    fprintf(stdout, "Supported options: ");
                    if (pi.sup_opts_len > 0) {
                        fprintf(stdout, "\n");
                        for (size_t i = 0; i < pi.sup_opts_len; i++) {
                            fprintf(stdout, "\t--%s\t\t%s\n", pi.sup_opts[i].opt.name, pi.sup_opts[i].opt_descr);
                        }
                    }
                    else {
                        fprintf(stdout, "none (!?)\n");
                    }
                    fprintf(stdout, "\n");
                    fprintf(stdout, "///////////////////////////////////////////////////////////////\n\n");

                    lib_count++;
                }
            }
        }
        closedir(d);
    }

    if (!lib_count)
    {
        fprintf(stderr, "ERROR: Can't find any library at '%s'\n", pdir);
        goto END;
    }

    // Считаем количество поддерживаемых длинных опций для выделения памяти
    int opt_count = 0;
    for (int i = 0; i < lib_count; i++)
    {
        opt_count += plugins[i].sup_opts_len;
    }


    // Теперь готовим массив с опциями, которые могут поступить на вход
    longopts = (struct option*)malloc(opt_count * sizeof(struct option));
    if (!longopts)
    {
        fprintf(stderr, "ERROR: malloc");
        goto END;
    }

    opt_count = 0;
    for (int i = 0; i < lib_count; i++)
    {
        for (int j = 0; j < plugins[i].sup_opts_len; j++)
        {
            longopts[opt_count] = plugins[i].sup_long_opts[j].opt;
            opt_count++;
        }
    }

    // Считываем длинные опции и оставшиеся короткие опции
    int res;
    while (1)
    {
        int opt_ind = -1;
        res = getopt_long(argc, argv, "P:AON", longopts, &opt_ind);
        if (res == -1)
            break;
        switch (res)
        {
        case 'A':
            if (!rules.AND)
            {
                if (!rules.OR)
                {
                    rules.AND = 1;
                }
                else
                {
                    fprintf(stderr, "ERROR: Didn't expect -O and -A at the sametime \n");
                    goto END;
                }
            }
            else
            {
                fprintf(stderr, "ERROR: expected only one option: -A\n");
                goto END;
            }
            break;
        case 'O':
            if (!rules.OR)
            {
                if (!rules.AND)
                {
                    rules.OR = 1;
                }
                else
                {
                    fprintf(stderr, "ERROR: Didn't expect -O and -A at the sametime \n");
                    goto END;
                }
            }
            else
            {
                fprintf(stderr, "ERROR: expected only one option: -O\n");
                goto END;
            }
            break;
        case 'N':
            if (!rules.NOT)
            {
                rules.NOT = 1;
            }
            else
            {
                fprintf(stderr, "ERROR: expected only one option: 'N'\n");
                goto END;
            }
            break;
        case ':':
            goto END;
        case '?':
            goto END;
        default:
            if (opt_ind != -1)
            {

                // Присоединяем опцию к нужной библиотеке
                for (int i = 0; i < lib_count; i++)
                {
                    for (int j = 0; j < plugins[i].sup_opts_len; j++)
                    {
                        if (strcmp(longopts[opt_ind].name, plugins[i].sup_long_opts[j].opt.name) == 0)
                        {
                            memcpy(plugins[i].opts_to_pass + plugins[i].opts_to_pass_len, longopts + opt_ind, sizeof(struct option));
                            if ((longopts + opt_ind)->has_arg) {
                                (plugins[i].opts_to_pass + plugins[i].opts_to_pass_len)->flag = (int*)strdup(optarg);
                            }
                            plugins[i].opts_to_pass_len++;
                            plugins[i].used = 1;
                        }
                    }
                }

            }
        }
    }

    if (!rules.OR && !rules.AND)
    {
        rules.AND = 1;
    }

    if (DEBUG)
    {
        fprintf(stderr, "AND: %d\nOR: %d\nNOT: %d\n", rules.AND, rules.OR, rules.NOT);
    }

    if (DEBUG)
    {
        fprintf(stderr, "Searching in directory: %s\n", wdir);
    }

    res = nftw(wdir, walk_func, 10, FTW_PHYS);
    if (res < 0)
    {
        fprintf(stderr, "ERROR: nftw\n");
        goto END;
    }

    if (!ffound)
    {
        fprintf(stdout, "Nothing found.\n");
    }



END:
    // Чистим паиять
    for (int i = 0; i < lib_count; i++)
    {
        for (int j = 0; j < plugins[i].opts_to_pass_len; j++)
        {
            if ((plugins[i].opts_to_pass + j)->flag) free((plugins[i].opts_to_pass + j)->flag);
        }
        if (plugins[i].opts_to_pass) free(plugins[i].opts_to_pass);
        if (plugins[i].dl) dlclose(plugins[i].dl);
    }

    if (plugins) free(plugins);
    if (longopts) free(longopts);
    if (pdir) free(pdir);
    if (wdir) free(wdir);

    return 0;

}

typedef int (*ppf_func_t)(const char*, struct option*, size_t);

int walk_func(const char* fpath, const struct stat* sb,
    int typeflag, struct FTW* ftwbuf)
{
    sb = sb;
    typeflag = typeflag;
    ftwbuf = ftwbuf;

    // Проверяем только файлы
    if (typeflag == FTW_F) {

        int isfit = rules.NOT ^ rules.AND;
        for (int i = 0; i < lib_count; i++)
        {
            if (plugins[i].used)
            {

                void* func = dlsym(plugins[i].dl, "plugin_process_file");

                ppf_func_t ppf_func = (ppf_func_t)func;

                int res = ppf_func(fpath, plugins[i].opts_to_pass, plugins[i].opts_to_pass_len);

                if (res < 0)
                {
                    fprintf(stderr, "ERROR: plugin_process_file() returned -1\n");
                    return 1;
                }

                // XORим результат и флаг NOT
                res ^= rules.NOT;

                // 2 варианта: a&b OR not(a||b) = not(a) & not(b) (см. законы Моргана)
                if (rules.NOT ^ rules.AND)
                {
                    if (!(isfit &= res))
                        break;
                }

                // a||b OR not(a&b) = not(a) || not(b)
                else
                {
                    if ((isfit |= res))
                        break;
                }
            }
            

        }
        if (isfit)
        {
            ffound = 1;
            printf("File fits: %s\n", fpath);
        }
        else
        {
            if (getenv("LAB1DEBUG"))
            {
                fprintf(stderr, "'%s' does not fit\n", fpath);
            }
        }
    }
    return 0;
}


const char* get_file_extention(const char* filename)
{
    const char* dot = strrchr(filename, '.');
    if (!dot || dot == filename) return "";
    return dot + 1;
}

