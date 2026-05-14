#pragma once
#include "DataReader.h"
#include "Types.h"
#include <vector>

class Calculator {
public:
    Calculator(const DataReader& data);
    std::vector<CalculationResult> calculateAll() const;
    CalculationResult calculateSingle(const ShippingRequest& req) const;
private:
    const DataReader& data;
    double calculateDistance(double lat1, double lon1, double lat2, double lon2) const;
};
