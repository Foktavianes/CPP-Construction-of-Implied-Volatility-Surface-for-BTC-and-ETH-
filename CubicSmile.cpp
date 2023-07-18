#include "CubicSmile.h"
#include "BSAnalytics.h"
#include <iostream>
#include <sstream>
#include <cmath>
#include <map>
#include <unordered_map>
#include "Date.h"
#include <Eigen/Core>
#include <LBFGSB.h>
#include <LBFGS.h>

// Rosenbrock
using Eigen::VectorXd;
using namespace LBFGSpp;

CubicSmile CubicSmile::FitSmile(const std::vector<TickData>& volTickerSnap)
{
    double fwd, T, atmvol, bf25, rr25, bf10, rr10;
    TickData latestTick;
    if (volTickerSnap.empty()) {
        throw std::runtime_error("No tick data available.");  
    }

    // Check if all tick data has the same expiry and underlying
    bool isSameExpiry = true;
    bool isSameUnderlying = true;

    const TickData& firstTick = volTickerSnap.front();
    uint64_t expiry = firstTick.LastUpdateTimeStamp;
    fwd = firstTick.UnderlyingPrice;

    for (const TickData& tick : volTickerSnap) {
        if (tick.LastUpdateTimeStamp != expiry)
            isSameExpiry = false;
        if (tick.UnderlyingPrice != fwd)
            isSameUnderlying = false;
    }

    if (!isSameExpiry || !isSameUnderlying) {
        throw std::runtime_error("Tick data does not have the same expiry or underlying.");
    }

    datetime_t latestTimeStamp = datetime_t(0);
    uint64_t latestTimeStampValue = 0;
    TickData LatestTick;
    for (const TickData& tick : volTickerSnap)
    {
        if (tick.LastUpdateTimeStamp > latestTimeStampValue)
        {
            latestTimeStampValue = tick.LastUpdateTimeStamp;
            latestTimeStamp = datetime_t(tick.LastUpdateTimeStamp);
            LatestTick = tick;
        }
    }
    
    // Get underlying price
    fwd = LatestTick.UnderlyingPrice;

    // Get expiry date from the contract name
    std::string contractName = LatestTick.ContractName;
    std::string expiryDate = contractName.substr(contractName.find_last_of('-') + 1, 6);

    // Parse the date manually
    int day = std::stoi(expiryDate.substr(0, 2));
    int month = std::stoi(expiryDate.substr(2, 2));
    int year = std::stoi(expiryDate.substr(4, 2));
    datetime_t expiry(year, month, day);

    // Calculate T by subtracting the expiry date from the last update time
    T = static_cast<double>(expiry - latestTimeStamp);

    // Extract the strike and option type from the contract name
    std::string optionTypeStr = contractName.substr(contractName.length() - 1, 1);
    OptionType optionType = (optionTypeStr == "C") ? Call : Put;
    double strike = std::stod(contractName.substr(contractName.find_last_of('-') + 1, contractName.length() - 2 - contractName.find_last_of('-')));

    // Fit the smile using optimizer and calculate the model error
    auto objective = [&](const VectorXd& x) -> double {
        double error = 0.0;

        for (const TickData& tick : volTickerSnap)
        {
            double observedVol = impliedVol(optionType, strike, fwd, T, tick.MarkPrice);

            double fittedVol = tick.MarkIV;
            double diff = fittedVol - observedVol;

            error += diff * diff;
        }

        return error;
    };

    // Initial parameter values
    VectorXd x(5);
    x[0] = 0.0;
    x[1] = 0.0;
    x[2] = 0.0;
    x[3] = 0.0;
    x[4] = 0.0;

    // Run the optimizer to minimize the objective function
    LBFGSpp::LBFGSParam<double> param;
    param.max_iterations = 100;
    LBFGSpp::LBFGSSolver<double> solver(param);
    double error;
    solver.minimize(objective, x, error);

    // Extract the fitted parameters
    atmvol = x[0];
    bf25 = x[1];
    rr25 = x[2];
    bf10 = x[3];
    rr10 = x[4];

    return CubicSmile(fwd, T, atmvol, bf25, rr25, bf10, rr10);
}

CubicSmile::CubicSmile( double underlyingPrice, double T, double atmvol, double bf25, double rr25, double bf10, double rr10) {
    // convert delta marks to strike vol marks, setup strikeMarks, then call BUildInterp
    double v_qd90 = atmvol + bf10 - rr10 / 2.0;
    double v_qd75 = atmvol + bf25 - rr25 / 2.0;
    double v_qd25 = atmvol + bf25 + rr25 / 2.0;
    double v_qd10 = atmvol + bf10 + rr10 / 2.0;

    // we use quick delta: qd = N(log(F/K / (atmvol) / sqrt(T))
    double stdev = atmvol * sqrt(T);
    double k_qd90 = quickDeltaToStrike(0.9, underlyingPrice, stdev);
    double k_qd75 = quickDeltaToStrike(0.75, underlyingPrice, stdev);
    double k_qd25 = quickDeltaToStrike(0.25, underlyingPrice, stdev);
    double k_qd10 = quickDeltaToStrike(0.10, underlyingPrice, stdev);

    strikeMarks.push_back(std::pair<double, double>(k_qd90, v_qd90));
    strikeMarks.push_back(std::pair<double, double>(k_qd75, v_qd75));
    strikeMarks.push_back(std::pair<double, double>(underlyingPrice, atmvol));
    strikeMarks.push_back(std::pair<double, double>(k_qd25, v_qd25));
    strikeMarks.push_back(std::pair<double, double>(k_qd10, v_qd10));
    BuildInterp();
}

void CubicSmile::BuildInterp()
{
  int n = strikeMarks.size();
  // end y' are zero, flat extrapolation
  double yp1 = 0;
  double ypn = 0;
  y2.resize(n);
  vector<double> u(n-1);

  y2[0] = -0.5;
  u[0]=(3.0/(strikeMarks[1].first-strikeMarks[0].first)) *
    ((strikeMarks[1].second-strikeMarks[0].second) / (strikeMarks[1].first-strikeMarks[0].first) - yp1);

  for(int i = 1; i < n-1; i++) {
    double sig=(strikeMarks[i].first-strikeMarks[i-1].first)/(strikeMarks[i+1].first-strikeMarks[i-1].first);
    double p=sig*y2[i-1]+2.0;
    y2[i]=(sig-1.0)/p;
    u[i]=(strikeMarks[i+1].second-strikeMarks[i].second)/(strikeMarks[i+1].first-strikeMarks[i].first)
      - (strikeMarks[i].second-strikeMarks[i-1].second)/(strikeMarks[i].first-strikeMarks[i-1].first);
    u[i]=(6.0*u[i]/(strikeMarks[i+1].first-strikeMarks[i-1].first)-sig*u[i-1])/p;
  }

  double qn=0.5;
  double un=(3.0/(strikeMarks[n-1].first-strikeMarks[n-2].first)) *
    (ypn-(strikeMarks[n-1].second-strikeMarks[n-2].second)/(strikeMarks[n-1].first-strikeMarks[n-2].first));

  y2[n-1]=(un-qn*u[n-2])/(qn*y2[n-2]+1.0);

//  std::cout << "y2[" << n-1 << "] = " << y2[n-1] << std::endl;
  for (int i=n-2;i>=0;i--) {
    y2[i]=y2[i]*y2[i+1]+u[i];
//    std::cout << "y2[" << i << "] = " << y2[i] << std::endl;
  }
}

double CubicSmile::Vol(double strike)
{
  unsigned i;
  // we use trivial search, but can consider binary search for better performance
  for (i = 0; i < strikeMarks.size(); i++ )
    if (strike < strikeMarks[i].first )
      break; // i stores the index of the right end of the bracket

  // extrapolation
  if (i == 0)
    return strikeMarks[i].second;
  if (i == strikeMarks.size() )
    return strikeMarks[i-1].second;

  // interpolate
  double h = strikeMarks[i].first - strikeMarks[i-1].first;
  double a = (strikeMarks[i].first - strike) / h;
  double b = 1 - a;
  double c = (a*a*a - a) * h * h / 6.0;
  double d = (b*b*b - b) * h * h / 6.0;
  return a*strikeMarks[i-1].second + b*strikeMarks[i].second + c*y2[i-1] + d*y2[i];
}
