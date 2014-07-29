#ifndef GZIP_TEST_H_INCLUDED
#define GZIP_TEST_H_INCLUDED

#include <zlib.h>

void gzip_test(u_int32 type, u_int32 arg) {
    // Bytef - 1 byte
    Bytef * src;
    Bytef * dst;
    timer::TTimer tm;
    u_int32 d_sz = arg+arg*0.1+12; // !!! 0.1% larger than sourceLen plus 12 bytes
    printf("\t* test: gzip \n");
    if((type >= 0)&&(type <= 9))
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
    src = (Bytef *)malloc(arg);
    dst = (Bytef *)malloc(d_sz);
    if((src == 0)||(dst == 0)) {
        printf("\t* not enough memory for test!\n");
        // afunix: не удаляется память!
        if (src != 0)
            free(src);
        if (dst != 0)
            free(dst);
        return;
    }
    printf("\t* generate data ...\n");
    gen_block((char *)src, arg); // from mem_test.h
    printf("\t* time: ");
    tm.start();
    switch(compress2(dst, &d_sz, src, arg, type)) {
        case Z_OK       : printf("%lf s\n", tm.cstop());
                        if(type > 0) printf("\t* compress: %.1f%%\n", 100-((float)d_sz/(float)arg)*100);
                        break;
        case Z_BUF_ERROR: printf(" not enough room in the output\n"); break;
        default         : printf(" i don't know what happened :oP\n"); break;
    }
    free(src);
    free(dst);
}

#endif // GZIP_TEST_H_INCLUDED
