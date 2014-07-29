#ifndef TIMER_H_INCLUDED

#include <sys/time.h>

#define TIMER_H_INCLUDED
namespace timer {
    class TTimer {
        struct timeval first;
        bool cleared;
        public:
            TTimer() { cleared = true; };
            ~TTimer() {};
            void start();
            double stop();
            void clear();
            double cstop();

    };
    // начало отсчёта времени
    void TTimer::start() {
        gettimeofday(&first, 0);
        cleared = false;
    }
    // завершение отсчёта
    // выход: (сек).(милисек)
    double TTimer::stop() {
        if(cleared) return 0.0;
        struct timeval last;
        gettimeofday(&last, 0);
        // получаем разницу
        return (double)(last.tv_usec - first.tv_usec)/1000000 + (last.tv_sec - first.tv_sec);
    }
    // сбрасываем отсчёт времени
    void TTimer::clear() {
        cleared = true;
    }
    // завершение отсчёта с последующим сбросом
    double TTimer::cstop() {
        double res = stop();
        clear();
        return res;
    }
}
#endif // TIMER_H_INCLUDED
