#ifndef MEM_TEST_H_INCLUDED
#define MEM_TEST_H_INCLUDED

void gen_block(char * block, u_int32 size) {
    u_int32 offs;
    char val = 0;
    for(offs=0; offs<size; offs++) {
        block[offs] = val;
        val++;
        if(val == 255) val = 0;
    }
}

void mem_test_8(char * array, u_int32 size) {
    timer::TTimer tm;
    char tmp;
    u_int32 offs;
    // заполняем массив
    tm.start();
    for(offs=0; offs<size; offs++) array[offs] = 1;
    printf("\t* write time: %lf s\n", tm.cstop());
    // читаем из массива
    tm.start();
    for(offs=0; offs<size; offs++) tmp = array[offs];
    printf("\t* read time: %lf s\n", tm.cstop());
}

void mem_test_16(short int * array, u_int32 size) {
    timer::TTimer tm;
    short int tmp;
    u_int32 offs;
    // заполняем массив
    tm.start();
    for(offs=0; offs<size; offs++) array[offs] = 1;
    printf("\t* write time: %lf s\n", tm.cstop());
    // читаем из массива
    tm.start();
    for(offs=0; offs<size; offs++) tmp = array[offs];
    printf("\t* read time: %lf s\n", tm.cstop());
}

void mem_test_32(long int * array, u_int32 size) {
    timer::TTimer tm;
    long int tmp;
    u_int32 offs;
    // заполняем массив
    tm.start();
    for(offs=0; offs<size; offs++) array[offs] = 1;
    printf("\t* write time: %lf s\n", tm.cstop());
    // читаем из массива
    tm.start();
    for(offs=0; offs<size; offs++) tmp = array[offs];
    printf("\t* read time: %lf s\n", tm.cstop());
}

void mem_test(u_int32 type, u_int32 arg) {
    char * array = (char *)malloc(arg);
    printf("\t* test: Mem %lu\n", type);
    if(array == 0) {
        printf("\t* not enough memory for test!\n");
        return;
    }
    if(arg != 0)
        printf("\t* size: %lu\n", arg);
    switch(type) {
        case 8: mem_test_8(array, arg); break;
        case 16: mem_test_16((short int *)array, arg/2); break;
        case 32: mem_test_32((long int *)array, arg/4); break;
    }
    free(array);
}

#endif // MEM_TEST_H_INCLUDED
