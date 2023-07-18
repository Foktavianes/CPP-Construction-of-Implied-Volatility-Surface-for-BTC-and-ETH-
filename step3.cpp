#include <iostream>

#include "CsvFeeder.h"
#include "Msg.h"
#include "VolSurfBuilder.h"
#include "CubicSmile.h"

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: "
                  << argv[0] << " tick_data.csv" << " outputFile.csv" << std::endl;
        return 1;
    }
    const char* ticker_filename = argv[1];

    VolSurfBuilder<CubicSmile> volBuilder;
    auto feeder_listener = [&volBuilder] (const Msg& msg) {
        if (msg.isSet) {
            volBuilder.Process(msg);
        }
    };

    auto timer_listener = [&volBuilder](uint64_t now_ms) {
        // Fit smile
        auto smiles = volBuilder.FitSmiles();

        // Stream the smiles and their fitting error to outputFile.csv
        std::ofstream outputFile("outputFile.csv");
        if (outputFile.is_open())
        {
            // Write header
            outputFile << "TIME,EXPIRY,FUT_PRICE,ATM,BF25,RR25,BF10,RR10" << std::endl;

            // Write data
            for (const auto& kvp : smiles)
            {
                datetime_t expiry = kvp.first;
                const CubicSmile& smile = kvp.second.first;
                double fittingError = kvp.second.second;

                outputFile << now_ms << ",";
                outputFile << expiry.year << "-" << expiry.month << "-" << expiry.day << ",";
                outputFile << smile.fwd << ",";
                outputFile << smile.T << ",";
                outputFile << smile.atmvol << ",";
                outputFile << smile.bf25 << ",";
                outputFile << smile.rr25 << ",";
                outputFile << smile.bf10 << ",";
                outputFile << smile.rr10 << std::endl;
                }

            // Close the file
            outputFile.close();
        }
        else
        {
            std::cerr << "Unable to open outputFile.csv" << std::endl;
        }
    };


    const auto interval = std::chrono::minutes(1);  // we call timer_listener at 1 minute interval
    CsvFeeder csv_feeder(ticker_filename,
                         feeder_listener,
                         interval,
                         timer_listener);
    while (csv_feeder.Step()) {
    }
    return 0;
}
