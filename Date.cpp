#include "Date.h"

double operator-(const datetime_t& d1, const datetime_t& d2)
{
    int yearDiff = d1.year - d2.year;
    int monthDiff = d1.month - d2.month;
    int dayDiff = d1.day - d2.day;
    int hourDiff = d1.hour - d2.hour;
    int minDiff = d1.min - d2.min;
    int secDiff = d1.sec - d2.sec;
    return yearDiff + monthDiff / 12.0 + dayDiff / 365.0 + hourDiff / 8760.0 + minDiff / 525600.0 + secDiff / 31536000.0;
}

bool operator<(const datetime_t& d1, const datetime_t& d2)
{
    return d2 - d2 > 0;
}

std::ostream& operator<<(std::ostream& os, const datetime_t& d)
{
  os << d.year << " " << d.month << " " << d.day << std::endl;
  return os;
}

std::istream& operator>>(std::istream& is, datetime_t& d)
{
  is >> d.year >> d.month >> d.day;
  return is;
}
