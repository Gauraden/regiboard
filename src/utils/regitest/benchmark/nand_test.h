#ifndef NAND_TEST_H_INCLUDED
#define NAND_TEST_H_INCLUDED

#include <iostream>
#include <fstream>

#define BIN_FILE_NAME "test.bin"
//#define BLOCK_SIZE 2048 // 2Kb
#define BLOCK_SIZE 549

using namespace std;

void nand_test_w(u_int32 arg, char * block) {
    timer::TTimer tm;
    u_int32 offs;
    ofstream fhdl;
    printf("\t* writing file: ");

    //fhdl.open (BIN_FILE_NAME, ios::binary);
    fhdl.open (BIN_FILE_NAME, ios::out | ios::binary);
    if (!fhdl.is_open()) {
        printf(" can't open file!\n");
        return;
    }

    tm.start();
    for (offs=0; offs<arg; offs+=BLOCK_SIZE) {
    	if (not fhdl.good())
		std::cout << "\n\t\t> stream is not good!";
	fhdl.write(block, BLOCK_SIZE);
    }
    printf("%lf s\n", tm.cstop());
    fhdl.close();
}

void nand_test_r(u_int32 arg, char * block) {
    timer::TTimer tm;
    u_int32 offs;
    ifstream fhdl;
    printf("\t* reading file: ");
    fhdl.open (BIN_FILE_NAME, ios::binary);
    if (!fhdl.is_open()) {
        printf(" can't open file!\n");
        return;
    }
    tm.start();
    for(offs=0; offs<arg; offs+=BLOCK_SIZE) fhdl.read(block, BLOCK_SIZE);
    printf("%lf s\n", tm.cstop());
    fhdl.close();
}

void nand_test(u_int32 type, u_int32 arg) {
    char * block = (char *)malloc(BLOCK_SIZE);
    printf("\t* test: Nand\n");
    if(block == 0) {
        printf("\t* not enough memory for test!\n");
        return;
    }
    if(arg != 0)
        printf("\t* size: %lu\n", arg);
    printf("\t* block: %d\n", BLOCK_SIZE);
    nand_test_w(arg, block);
    nand_test_r(arg, block);
    printf("\t* remove file:");
    if(remove(BIN_FILE_NAME) != 0)
        printf(" error!\n");
    else
        printf(" ok\n");
    free(block);
}

#endif // NAND_TEST_H_INCLUDED
