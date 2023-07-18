# CPP-Construction-of-Implied-Volatility-Surface-for-BTC-and-ETH-

Step 1 involves reading the .csv file to make it such as we receiveing tick data and grouping data with same timestamp together

Step 2 checking whether the tick data is update or snapshot. If the tick data is update, we just change the value of certain member with utilizing the property of  std::map. If the tick data is snapshow, we will erase current snapshot and generate new snapshot

Step 3 builds the implied vol surface by using LBFGS optimization to obtain the implied vol for BF10, BF25, RR10, RR 25 at one time. Then, we will interpolate the cubicspline from different expiry to generate implied volatility surface 
