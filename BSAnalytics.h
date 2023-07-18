#ifndef _BS_ANALYTICS
#define _BS_ANALYTICS

#include <cmath>
#include "Solver/RootSearcher.h"
#include "Msg.h"

enum OptionType
{
  Call,
  Put
};
double cnorm(double x)
{
    // constants
    double a1 =  0.254829592;
    double a2 = -0.284496736;
    double a3 =  1.421413741;
    double a4 = -1.453152027;
    double a5 =  1.061405429;
    double p  =  0.3275911;
    int sign = 1;
    if (x < 0)
        sign = -1;
    x = fabs(x)/sqrt(2.0);
    double t = 1.0/(1.0 + p*x);
    double y = 1.0 - (((((a5*t + a4)*t) + a3)*t + a2)*t + a1)*t*exp(-x*x);
    return 0.5*(1.0 + sign*y);
}

double invcnorm(double x) {
    assert(x > 0 && x < 1);
    auto f = [&x](double v){return cnorm(v) - x;};
    return rfbisect(f, -1e8, 1e8, 1e-6);
}

double bsUndisc(OptionType optType, double k, double fwd, double T, double sigma) {
    double sigmaSqrtT = sigma * std::sqrt(T);
    double d1 = std::log(fwd / k)/sigmaSqrtT + 0.5 * sigmaSqrtT;
    double d2 = d1 - sigmaSqrtT;
    double V_0;
    switch (optType)
    {
        case Call:
            V_0 = fwd * cnorm(d1) - k * cnorm(d2);
            break;
        case Put:
            V_0 = k * cnorm(-d2) - fwd * cnorm(-d1);
            break;
        default:
            throw "unsupported optionType";
    }
    return V_0;
}

double quickDelta(double fwd, double strike, double atmVol, double T){
    return cnorm(std::log(fwd/strike) / (atmVol * sqrt(T)));
}

double quickDelta(double fwd, double strike, double stdev){
    return cnorm(std::log(fwd/strike) / stdev);
}

// qd = N(log(F/K) / stdev), so K = F / exp((N^{-1}(qd) * stdev))
double quickDeltaToStrike(double qd, double fwd, double stdev) {
    double inv = invcnorm(qd);
    return fwd / std::exp(inv * stdev);
}

double quickDeltaToStrike(double qd, double fwd, double atmvol, double T) {
    double stdev = atmvol * sqrt(T);
    return quickDeltaToStrike(qd, fwd, stdev);
}

double impliedVol(OptionType optionType, double k, double fwd, double T, double undiscPrice) {
    auto f = [undiscPrice, optionType, k, fwd, T](double vol){return bsUndisc(optionType, k, fwd, T, vol) - undiscPrice;};
    return rfbrent(f, 1e-4, 10, 1e-6);
}

#endif
