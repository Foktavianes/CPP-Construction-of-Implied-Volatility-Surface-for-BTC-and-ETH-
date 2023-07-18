#include <iostream>

int main() {
    // declare the function implemented in CsvFeeder.cpp
    uint64_t TimeToUnixMS(std::string ts);
    std::string ts("2022-05-06T00:00:00.139Z");
    auto r = TimeToUnixMS(ts);
    std::cout << "timestamp \"" << ts << "\"'s unix epoch (millisecond) is " << r << std::endl;
    return 0;
}
