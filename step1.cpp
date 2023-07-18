#include <iostream>
#include "CsvFeeder.h"
#include "Msg.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: "
                  << argv[0] << " tick_data.csv" << std::endl;
        return 1;
    }
    const char* ticker_filename = argv[1];

    auto feeder_listener = [&] (const Msg& msg) {
        if (msg.isSet) {
            std::cout << msg.timestamp << ", isSnap = " << msg.isSnap << ", numUpdates = " << msg.Updates.size() << std::endl;
        }
    };

    auto timer_listener = [&] (uint64_t now_ms) {
        std::cout << "timer_listener called: " << now_ms << std::endl;
    };

    const auto interval = std::chrono::minutes(1);  // we call timer_listener at 1 minute interval
    CsvFeeder csv_feeder(ticker_filename,
                         feeder_listener,
                         interval,
                         timer_listener);

    // int count = 0; // set a counter to investigate the first few rows
    while (csv_feeder.Step())
    {
        // count++; // Increment the counter each time a line is read
        // if (count >= 100)
        // { // If 100 lines have been read, break the loop
        //   break;
        // }
    }
    return 0;
}
