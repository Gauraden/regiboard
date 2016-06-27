/*
 * debug_backtrace.hpp
 *
 *  Created on: 11.11.2015
 *      Author: denis
 */

#ifndef DEBUG_BACKTRACE_HPP
#define DEBUG_BACKTRACE_HPP

#include <string>
#include <ostream>

namespace debug {

class Backtrace {
  public:
    Backtrace();
    ~Backtrace();
    /**
     * Метод чтения и сохранения стэка
     * @return true если удалось прочитать
     */
    bool KeepStack();
    /**
     * Метод вывода сохраненного стэка в поток выходных данных.
     * Вывод производится в текстовом режиме.
     * @param out указатель на поток для вывода
     */
    bool PrintStackTo(std::ostream *out);
    /**
     * Метод чтения стэка и передачи его в заданную строку.
     * @param out указатель на строку для записи прочитанного стэка.
     * @return true если удалось прочитать и записать результат
     */
    bool ReadStack(std::string *out);
    /// Метод удаления запомненного стэка
    void Clear();
  private:
    bool ReadStackTo(std::string *out);

    std::string _stack;
}; // class Backtrace

} // namespace debug

#endif /* DEBUG_BACKTRACE_HPP */
