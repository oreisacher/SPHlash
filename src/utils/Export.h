//
// Created by ruut on 7/15/26.
//

#ifndef SPHLASH_EXPORT_H
#define SPHLASH_EXPORT_H

#include <string>
#include <vector>

class Export {
public:
    static void exportCSV(const std::string& path, const std::vector<float>& data);
};

#endif //SPHLASH_EXPORT_H
