//
// Created by ruut on 7/15/26.
//

#include "utils/Export.h"
#include "utils/Logger.h"
#include <fstream>

void Export::exportCSV(const std::string& path, const std::vector<float>& data) {
    std::ofstream file(path);
    if (!file.is_open()) {
        LOG_ERROR("Could not open destination file.");
        return;
    }

    file << "Frame,Data\n";

    for (int i = 0 ; i < data.size() ; i++) {
        file << i << "," << data[i] << "\n";
    }
}
