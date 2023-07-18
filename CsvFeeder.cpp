#include <iostream>
#include "CsvFeeder.h"
#include "date/date.h"
#include <sstream>
#include <string>

uint64_t TimeToUnixMS(std::string ts)
{
  std::istringstream in{ts};
  std::chrono::system_clock::time_point tp;
  in >> date::parse("%FT%T", tp);
  const auto timestamp = std::chrono::time_point_cast<std::chrono::milliseconds>(tp).time_since_epoch().count();
  return timestamp;
}

std::string parseLine(std::istringstream &lineStream, TickData &tick)
{
  std::string value;
  // Read ContractName
  std::getline(lineStream, value, ',');
  tick.ContractName = value;

  // Read and convert time to Unix timestamp
  std::getline(lineStream, value, ',');
  uint64_t timestamp = TimeToUnixMS(value);
  tick.LastUpdateTimeStamp = timestamp;

  // Assign snap field based on msgType
  std::getline(lineStream, value, ',');
  std::string snap = value;

  // Skip priceCcy
  std::getline(lineStream, value, ',');

  // Read BestBid (equivalent to BestBidPrice)
  std::getline(lineStream, value, ',');
  try
  {
    tick.BestBidPrice = std::stod(value);
  }
  catch (std::invalid_argument &e)
  {
    tick.BestBidPrice = std::numeric_limits<double>::quiet_NaN();
  }

  // Read BestBidAmount
  std::getline(lineStream, value, ',');
  try
  {
    tick.BestBidAmount = std::stod(value);
  }
  catch (std::invalid_argument &e)
  {
    tick.BestBidAmount = std::numeric_limits<double>::quiet_NaN();
  }

  // Read BestBidIV
  std::getline(lineStream, value, ',');
  try
  {
    tick.BestBidIV = std::stod(value);
  }
  catch (std::invalid_argument &e)
  {
    tick.BestBidIV = std::numeric_limits<double>::quiet_NaN();
  }

  // Read BestAsk (equivalent to BestAskPrice)
  std::getline(lineStream, value, ',');
  try
  {
    tick.BestAskPrice = std::stod(value);
  }
  catch (std::invalid_argument &e)
  {
    tick.BestAskPrice = std::numeric_limits<double>::quiet_NaN();
  }

  // Read BestAskAmount
  std::getline(lineStream, value, ',');
  try
  {
    tick.BestAskAmount = std::stod(value);
  }
  catch (std::invalid_argument &e)
  {
    tick.BestAskAmount = std::numeric_limits<double>::quiet_NaN();
  }

  // Read BestAskIV
  std::getline(lineStream, value, ',');
  try
  {
    tick.BestAskIV = std::stod(value);
  }
  catch (std::invalid_argument &e)
  {
    tick.BestAskIV = std::numeric_limits<double>::quiet_NaN();
  }

  // Read MarkPrice
  std::getline(lineStream, value, ',');
  try
  {
    tick.MarkPrice = std::stod(value);
  }
  catch (std::invalid_argument &e)
  {
    tick.MarkPrice = std::numeric_limits<double>::quiet_NaN();
  }

  // Read MarkIV
  std::getline(lineStream, value, ',');
  try
  {
    tick.MarkIV = std::stod(value);
  }
  catch (std::invalid_argument &e)
  {
    tick.MarkIV = std::numeric_limits<double>::quiet_NaN();
  }

  // Read UnderlyingIndex
  std::getline(lineStream, value, ',');
  tick.UnderlyingIndex = value;

  // Read UnderlyingPrice
  std::getline(lineStream, value, ',');
  try
  {
    tick.UnderlyingPrice = std::stod(value);
  }
  catch (std::invalid_argument &e)
  {
    tick.UnderlyingPrice = std::numeric_limits<double>::quiet_NaN();
  }

  // skip 'interest_rate'
  std::getline(lineStream, value, ',');

  // Read LastPrice with quiet_NaN() to handle missing values
  std::getline(lineStream, value, ',');
  try
  {
    tick.LastPrice = std::stod(value);
  }
  catch (std::invalid_argument &e)
  {
    tick.LastPrice = std::numeric_limits<double>::quiet_NaN();
  }

  // Read open_interest
  std::getline(lineStream, value, ',');
  try
  {
    tick.OpenInterest = std::stod(value);
  }
  catch (std::invalid_argument &e)
  {
    tick.OpenInterest = std::numeric_limits<double>::quiet_NaN();
  }

  // Skip 'vega', 'theta', 'rho', 'gamma', and 'delta'
  for (int i = 0; i < 5; i++)
  {
  std::getline(lineStream, value, ',');
  }
  return snap;
}

bool ReadNextMsg(std::ifstream &file, Msg &msg, Msg &tempMsg)
{
  if (!file.is_open())
  {
    std::cout << "Could not open the file" << std::endl;
    return false;
  }

  // Start with any message held over from the previous call
  if (tempMsg.isSet)
  {
    msg = tempMsg;
    tempMsg = Msg();
  }
  else
  {
    // Fetch the first line outside the loop
    std::string line;
    while (std::getline(file, line) && line.substr(0, 8) == "contract");
    if (line.empty())
    {
      return false;
    }

    // Parse the line and populate the first TickData and Msg
    std::istringstream lineStream(line);
    std::string value;
    TickData tick;

    // Assume the parseLine function correctly fills out the TickData and returns snap
    std::string snap = parseLine(lineStream, tick);

    msg.Updates.push_back(tick);
    msg.timestamp = tick.LastUpdateTimeStamp;
    msg.isSnap = (snap == "snap");
    msg.isSet = true;
  }

  std::string line;
  std::streampos oldPos; // Used to keep track of the stream position

  while (std::getline(file, line)) // Loop until a line with a different timestamp is found
  {
    // Skip header lines
    if (line.substr(0, 8) == "contract")
    {
      continue;
    }

    // Save the current position
    oldPos = file.tellg();

    std::istringstream lineStream(line);
    TickData tick;

    // Parse the line and populate tick
    std::string snap = parseLine(lineStream, tick);

    if (msg.isSet && (tick.LastUpdateTimeStamp != msg.timestamp || (snap == "snap") != msg.isSnap))
    {
      // Different timestamp or snap status found, put back line in file
      file.seekg(oldPos);

      // Save this message for the next call
      tempMsg.Updates.push_back(tick);
      tempMsg.timestamp = tick.LastUpdateTimeStamp;
      tempMsg.isSnap = (snap == "snap");
      tempMsg.isSet = true;

      return true;
    }
    else
    {
      // Add the tick to the current message
      msg.Updates.push_back(tick);
    }
  }

  // File ended, return true if at least one message was read
  return msg.isSet;
}

CsvFeeder::CsvFeeder(const std::string ticker_filename,
                     FeedListener feed_listener,
                     std::chrono::minutes interval,
                     TimerListener timer_listener)
        : ticker_file_(ticker_filename),
          feed_listener_(feed_listener),
          interval_(interval),
          timer_listener_(timer_listener) {
    // initialize member variables with input information, prepare for Step() processing

    ReadNextMsg(ticker_file_, msg_, tempMsg_);
    if (msg_.isSet) {
        // initialize interval timer now_ms_
        now_ms_ = msg_.timestamp;
    } else {
        throw std::invalid_argument("empty message at initialization");
    }
}

bool CsvFeeder::Step() {
    if (msg_.isSet) {
        // call feed_listener with the loaded Msg
        feed_listener_(msg_);

        // if current message's timestamp is crossing the given interval, call time_listener, change now_ms_ to the next interval cutoff
        if (now_ms_ < msg_.timestamp) {
            timer_listener_(now_ms_);
            now_ms_ += interval_.count();
        }
        // load tick data into Msg
        // if there is no more message from the csv file, return false, otherwise true
        return ReadNextMsg(ticker_file_, msg_, tempMsg_);
    }
    return false;
}

CsvFeeder::~CsvFeeder() {
    // release resource allocated in constructor, if any
}
