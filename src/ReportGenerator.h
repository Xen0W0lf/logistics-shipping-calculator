#pragma once
#include "Types.h"
#include "DataReader.h"
#include <string>
#include <vector>

class ReportGenerator {
public:
    static void generateTextReport(const std::string& filepath, const std::vector<CalculationResult>& results);
    static void generateHTMLMap(const std::string& filepath, const DataReader& data);
};
