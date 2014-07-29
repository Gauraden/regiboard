#ifndef MACROSES_H_INCLUDED

#include <stdlib.h>
#include <string.h>

#define MACROSES_H_INCLUDED
#define MACROSES_NAME_LEN 10

typedef unsigned long int u_int32;

namespace macroses {
    struct __macros {
        char name[MACROSES_NAME_LEN];
        void (* func)(u_int32 type, u_int32 arg);
        //struct __macros * prev;
        struct __macros * next;
    };

    class TMacrosList {
        //int count;
        struct __macros * list;
        struct __macros * end;
        public:
            TMacrosList() {
                //count = 0;
                list = 0;
                end = 0;
            };
            ~TMacrosList() {
                clear();
            };
            void add(const char * name, void (* func)(u_int32 type, u_int32 arg));
            struct __macros * get(const char * name);
            int run(const char * name, u_int32 type, u_int32 arg);
            void clear();
    };

    void TMacrosList::add(const char * name, void (* func)(u_int32 type, u_int32 arg)) {
        struct __macros * tmp;
        tmp = (struct __macros *) calloc(1, sizeof(struct __macros));
        strcpy(tmp->name, name);
        tmp->func = func;
        if(end != 0) {
            end->next = tmp;
            end = tmp;
        }
        if(list == 0) {
            list = tmp;
            end = tmp;
        }
    }

    struct __macros * TMacrosList::get(const char * name) {
        struct __macros * cur = list;
        while(cur != 0) {
            if(strcmp(name, cur->name) == 0) break;
            cur = cur->next;
        }
        return cur;
    }

    int TMacrosList::run(const char * name, u_int32 type, u_int32 arg) {
        struct __macros * cur = get(name);
        if(cur != 0) {
            cur->func(type, arg);
            return 0;
        }
        return -1;
    }

    void TMacrosList::clear() {
        struct __macros * cur = list;
        struct __macros * tmp;
        while(cur != 0) {
            tmp = cur->next;
            free(cur);
            cur = tmp;
        }
    }
}

#endif // MACROSES_H_INCLUDED
