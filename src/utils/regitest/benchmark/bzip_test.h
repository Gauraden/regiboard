#ifndef BZIP_TEST_H_INCLUDED
#define BZIP_TEST_H_INCLUDED

#include <bzlib.h>

void bzip_test(u_int32 type, u_int32 arg) {
    char * src;
    char * dst;
    timer::TTimer tm;
    unsigned int d_sz = arg; // могут быть проблемы, т.к. размер быть больше исходного.
    printf("\t* test: bzip\n");
    if((type >= 1)&&(type <= 9))
        printf("\t* level: %lu\n", type);
    else {
        printf("\t! unknown compression level [%lu]\n", type);
        return;
    }
    if(arg <= 0) {
        printf("\t! src size can't be 0 or less size\n");
        return;
    }
    printf("\t* src size: %lu\n", arg);
    src = (char *)malloc(arg);
    dst = (char *)malloc(d_sz);
    if((src == 0)||(dst == 0)) {
        printf("\t* not enough memory for test!\n");
        return;
    }
    printf("\t* generate data ...\n");
    gen_block((char *)src, arg); // from mem_test.h
    printf("\t* time: ");
    tm.start();
    switch(BZ2_bzBuffToBuffCompress(dst, &d_sz, src, arg, type, 0, 0)) {
        case BZ_OK          : printf("%lf s\n", tm.cstop());
                            if(type > 0) printf("\t* compress: %.1f%%\n", 100-((float)d_sz/(float)arg)*100);
                            break;
        case BZ_PARAM_ERROR : printf(" bzip init param error\n"); break;
        case BZ_MEM_ERROR   : printf(" insufficient memory is available\n"); break;
        case BZ_OUTBUFF_FULL: printf(" size of the compressed data exceeds\n"); break;
        default             : printf(" i don't know what happened :oP\n"); break;
    }
    free(src);
    free(dst);
}

#endif // BZIP_TEST_H_INCLUDED
