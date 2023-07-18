#ifndef QF633_CODE_CSVFEEDER_H
#define QF633_CODE_CSVFEEDER_H

#include <string>
#include <functional>
#include <chrono>
#include <fstream>

#include "Msg.h"


class CsvFeeder {
public:
    using FeedListener = std::function<void(const Msg& msg)>;
    using TimerListener = std::function<void(uint64_t ms_now)>;
    CsvFeeder(const std::string ticker_filename,
              FeedListener feed_listener,
              std::chrono::minutes interval, TimerListener timer_listener);
    ~CsvFeeder();
    bool Step();

  private:
    std::ifstream ticker_file_;
    FeedListener feed_listener_;
    const std::chrono::milliseconds interval_;
    TimerListener timer_listener_;

    uint64_t now_ms_{};
    Msg msg_;
    // your member variables and member functions below, if any
    Msg tempMsg_;
};

#endif //QF633_CODE_CSVFEEDER_H
