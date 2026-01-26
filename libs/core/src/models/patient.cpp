#include "tracker/models/patient.h"
#include <ctime>
#include <sstream>
#include <cstdio>

std::string Patient::getAge() const {
    if (!birth_date.has_value() || birth_date->empty()) return "-";

    int year, month, day;
    if (sscanf(birth_date->c_str(), "%d-%d-%d", &year, &month, &day) != 3) return "-";

    time_t t = time(nullptr);
    tm* now = localtime(&t);

    int years = now->tm_year + 1900 - year;
    int months = (now->tm_mon + 1) - month;
    int days = now->tm_mday - day;

    if (days < 0) {
        months -= 1;
        // приближение дней в месяце
        int prevMonth = (now->tm_mon == 0) ? 12 : now->tm_mon;
        int prevYear = (now->tm_mon == 0) ? now->tm_year : now->tm_year;
        static const int daysInMonth[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
        int dim = daysInMonth[prevMonth - 1];
        // високосный год для февраля
        if (prevMonth == 2 && ((prevYear + 1900) % 4 == 0 && ((prevYear + 1900) % 100 != 0 || (prevYear + 1900) % 400 == 0))) dim = 29;
        days += dim;
    }

    if (months < 0) {
        years -= 1;
        months += 12;
    }

    if (years > 0) return std::to_string(years) + " years";
    else if (months > 0) return std::to_string(months) + " months";
    else return std::to_string(days) + " days";
}
