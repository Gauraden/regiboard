#ifndef FB_TEST_H_INCLUDED
#define FB_TEST_H_INCLUDED

#include <linux/fb.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
//#include <linux/asm/ps3fb.h>

#define FB_DEV "/dev/fb0"

void fb_test(u_int32 type, u_int32 arg) {
    struct fb_var_screeninfo scrinfo;
    //struct ps3fb_ioctl_res fbinfo;
    int scr_fd, w, h, bpp;
    long int scr_sz, offs;
    unsigned char * buff;
    timer::TTimer tm;
    printf("\t* test: Frame buffer\n");
    printf("\t* scr info: ");
    // открываем файл-устройство "frame buffer"
    scr_fd = open(FB_DEV, O_RDWR);
    if(scr_fd == -1) {
        printf("can't open device file %s\n", FB_DEV);
        return;
    }
    if(ioctl(scr_fd, FBIOGET_VSCREENINFO, &scrinfo)==-1) {
        printf("can't get screen info\n");
        close(scr_fd);
        return;
    }
    // получаем инфу
    w   = scrinfo.xres_virtual;
    h   = scrinfo.yres_virtual;
    bpp = scrinfo.bits_per_pixel;
    printf("%dx%d %d\n", w, h, bpp);
    // о фрэйм буфере
    /*
    printf("\t* FB info: ");
    if(ioctl(scr_fd, PS3FB_IOCTL_SCREENINFO, &fbinfo)==-1) {
        printf("can't get FB info\n");
        close(scr_fd);
        return;
    }
    */
    // проецируем файл в память
    scr_sz = w*h*(bpp/8);
    buff = (unsigned char *)mmap(0, scr_sz, PROT_WRITE, MAP_FILE|MAP_SHARED, scr_fd, 0);
    if(buff == MAP_FAILED) {
        close(scr_fd);
        printf("\t! can't map memory\n");
        return;
    }
    // выполняем тест
    printf("\t* writing time: ");
    tm.start();
    for(offs=0; offs<scr_sz; offs++) buff[offs] = 255;
    printf("%lf s\n", tm.cstop());
    if(munmap(buff, scr_sz) == -1)
        printf("\t! can't free maped memory\n");
    close(scr_fd);
}

#endif // FB_TEST_H_INCLUDED
