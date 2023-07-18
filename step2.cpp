#include <iostream>

#include "CsvFeeder.h"
#include "Msg.h"
#include "VolSurfBuilder.h"
#include "CubicSmile.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: "
                  << argv[0] << " tick_data.csv" << std::endl;
        return 1;
    }
    const char* ticker_filename = argv[1];

    VolSurfBuilder<CubicSmile> volBuilder;
    auto feeder_listener = [&volBuilder] (const Msg& msg) {
        if (msg.isSet) {
            volBuilder.Process(msg);
        }
    };

    auto timer_listener = [&volBuilder] (uint64_t now_ms) {
        // print information of vol builder - you need to implement the PrintInfo() function yourself
        volBuilder.PrintInfo();
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
      // if (count >= 10000)
      // { // If 100 lines have been read, break the loop
      //   break;
      // }
    }
    return 0;
}
