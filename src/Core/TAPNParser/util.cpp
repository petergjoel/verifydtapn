#include "Core/TAPNParser/util.hpp"
#include <fstream>

namespace VerifyTAPN {
    std::string readFile(const std::string &filename) {
        std::ifstream file(filename.c_str());
        if (!file) {
            throw std::string("file not found.");
        }

        std::string storage((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        return storage;
    }
    std::string readFile(std::ifstream &file) {
        if (!file) {
            throw std::string("file not found.");
        }

        std::string storage((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        return storage;
    }
}
