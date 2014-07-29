#include <stdio.h>
#include "timer.h"
#include "macroses.h"
#include "mem_test.h"
#include "nand_test.h"
#include "fb_test.h"
#include "gzip_test.h"
//#include "bzip_test.h"

#define DIV "\t ------------------------------\n"
#define Kb 1024
#define Mb 1048576

using namespace std;

macroses::TMacrosList mlist;

long unsigned int parse_arg(char * src) {
    int num, mul=1;
    short int len;
    if(src == 0)
        return 0;
    len = strlen(src)-1;
    // получ множитель
    switch (src[len]) {
        case 'm': mul = Mb; break;
        case 'k': mul = Kb; break;
    }
    if(((int)src[len] < 48)||((int)src[len] > 57)) src[len] = 0;
    // получаем число
    num = atoi(src);
    return num*mul;
}

bool is_key(char * str) {
    if(str == 0)
        return false;
    if((int)str[0] == 45)
        return true;
    return false;
}

int main(int argc, char** argv)
{
    int argid;
    char * key;
    char * type;
    char * arg;
    // инициализация макросов
    mlist.add("mem", mem_test);
    mlist.add("nand", nand_test);
    mlist.add("gzip", gzip_test);
    //mlist.add("bzip2", bzip_test);
    mlist.add("fb", fb_test);
    // запуск тестов
    printf("Benchmark started...\n");
    printf(DIV);
    for (argid=1; argid<argc; argid++) {
        if(!is_key(argv[argid])) continue;
        // получаем название макроса и его тип
        key = strtok (&argv[argid][1], "_");
        if(key == 0)
                key = &argv[argid][1];
        type = strtok(0, "_");
        // пробуем получить аргумент макроса
        arg = 0;
        if((argid+1 < argc)&&(!is_key(argv[argid+1]))) arg = argv[argid+1];
        // запускаем подпрограмму теста
        if(mlist.run(key, parse_arg(type), parse_arg(arg)) < 0)
            printf("\t* macros \"%s\" not found\n", key);
        printf(DIV);
    }
    printf("Benchmark stoped...\n");
    return 0;
}
