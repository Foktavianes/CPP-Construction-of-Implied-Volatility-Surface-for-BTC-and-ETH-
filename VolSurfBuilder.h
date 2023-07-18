#ifndef QF633_CODE_VOLSURFBUILDER_H
#define QF633_CODE_VOLSURFBUILDER_H

#include <map>
#include "Msg.h"
#include "Date.h"

template<class Smile>
class VolSurfBuilder {
public:
    VolSurfBuilder() : first_snap(false) {} // Initialize the flag in the constructor
    void Process(const Msg& msg);  // process message
    void PrintInfo();
    std::map<datetime_t, std::pair<Smile, double>> FitSmiles();

  protected:
    // we want to keep the best level information for all instruments
    // here we use a map from contract name to BestLevelInfo, the key is contract name
    std::map<std::string, TickData> currentSurfaceRaw;
    bool first_snap; // The flag to check if the first snapshot has been encountered
};

template <class Smile>
void VolSurfBuilder<Smile>::Process(const Msg &msg)
{
    if (msg.isSnap)
    {
      // discard currently maintained market snapshot, and construct a
      // new copy based on the input Msg
      currentSurfaceRaw.clear();
      for (const auto &tick : msg.Updates)
      {
        currentSurfaceRaw.insert({tick.ContractName, tick});
      }
    }
    else
    {
      // update the currently maintained market snapshot
      for (const auto &tick : msg.Updates)
      {
        auto it = currentSurfaceRaw.find(tick.ContractName);
        if (it != currentSurfaceRaw.end())
        {
          it->second = tick;
        }
      }
    }
}

template <class Smile>
void VolSurfBuilder<Smile>::PrintInfo()
{
    // print out information about VolSurfBuilder's currentSnapshot to test
    std::cout << "Current surface contains " << currentSurfaceRaw.size() << " contracts:\n";
    for (const auto &pair : currentSurfaceRaw)
    {
      std::cout << "Contract: " << pair.first << ", LastPrice: " << pair.second.LastPrice << ", BestBidPrice: " << pair.second.BestBidPrice << ", BestAskPrice: " << pair.second.BestAskPrice << "\n";
    }
}

template <class Smile>
std::map<datetime_t, std::pair<Smile, double>> VolSurfBuilder<Smile>::FitSmiles()
{
    std::map<datetime_t, std::vector<TickData>> tickersByExpiry{};

    // Group the tickers in the current market snapshot by expiry date
    for (const auto& tick : currentSurfaceRaw)
    {
        datetime_t expiry = tick.ExpiryDate;
        tickersByExpiry[expiry].push_back(tick);
    }

    std::map<datetime_t, std::pair<Smile, double>> res{};

    // Fit smile for each expiry
    for (const auto& kvp : tickersByExpiry)
    {
        datetime_t expiry = kvp.first;
        const std::vector<TickData>& tickers = kvp.second;

        Smile sm = Smile::FitSmile(tickers);

        res[expiry] = std::make_pair(sm, 0.0); // Set fitting error to 0 for now. This one should keep the actual error, but we fail to do so.
    }

    return res;
}


#endif //QF633_CODE_VOLSURFBUILDER_H

