#include "date-utils.h"
#include <ctime>
#include <cstdio>

std::string calculateAge(const std::optional<std::string>& birth_date) {
    if (!birth_date.has_value() || birth_date->empty()) return "-";

    int year, month, day;
    if (sscanf(birth_date->c_str(), "%d-%d-%d", &year, &month, &day) != 3) return "-";

    if (month < 1 || month > 12 || day < 1 || day > 31 || year < 1900 || year > 2100) {
        return "-";
    }

    time_t t = time(nullptr);
    tm now;
#ifdef _WIN32
    if (localtime_s(&now, &t) != 0) return "-";
#else
    if (localtime_r(&t, &now) == nullptr) return "-";
#endif

    int years = now.tm_year + 1900 - year;
    int months = (now.tm_mon + 1) - month;
    int days = now.tm_mday - day;

    if (days < 0) {
        months -= 1;
        int prevMonth = (now.tm_mon == 0) ? 12 : now.tm_mon;
        int prevYear = (now.tm_mon == 0) ? now.tm_year + 1900 - 1 : now.tm_year + 1900;
        static const int daysInMonth[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
        int dim = daysInMonth[prevMonth - 1];
        if (prevMonth == 2 && (prevYear % 4 == 0 && (prevYear % 100 != 0 || prevYear % 400 == 0))) dim = 29;
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
