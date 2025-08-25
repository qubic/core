#pragma once

#include <sstream>
#include <string>
#include <iomanip>
#include <fstream>
#include <filesystem>

namespace test_utils
{

static std::string byteToHex(const unsigned char* byteArray, size_t sizeInByte)
{
    std::ostringstream oss;
    for (size_t i = 0; i < sizeInByte; ++i)
    {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byteArray[i]);
    }
    return oss.str();

}
static m256i hexTo32Bytes(const std::string& hex, const int sizeInByte)
{
    if (hex.length() != sizeInByte * 2) {
        throw std::invalid_argument("Hex string length does not match the expected size");
    }

    m256i byteArray;
    for (size_t i = 0; i < sizeInByte; ++i)
    {
        byteArray.m256i_u8[i] = std::stoi(hex.substr(i * 2, 2), nullptr, 16);
    }

    return byteArray;
}

static void hexToByte(const std::string& hex, const int sizeInByte, unsigned char* out)
{
    if (hex.length() != sizeInByte * 2)
    {
        throw std::invalid_argument("Hex string length does not match the expected size");
    }

    for (size_t i = 0; i < sizeInByte; ++i)
    {
        out[i] = std::stoi(hex.substr(i * 2, 2), nullptr, 16);
    }
}

// Function to read and parse the CSV file
static std::vector<std::vector<std::string>> readCSV(const std::string& filename)
{
    std::vector<std::vector<std::string>> data;
    std::ifstream file(filename);
    std::string line;

    // Read each line from the file
    while (std::getline(file, line))
    {
        std::stringstream ss(line);
        std::string item;
        std::vector<std::string> parsedLine;

        // Parse each item separated by commas
        while (std::getline(ss, item, ','))
        {
            // Remove any spaces in the string
            item.erase(remove_if(item.begin(), item.end(), isspace), item.end());

            parsedLine.push_back(item);
        }
        data.push_back(parsedLine);
    }
    return data;
}

static m256i convertFromString(std::string& rStr)
{
    m256i value;
    std::stringstream ss(rStr);
    std::string item;
    int i = 0;
    while (std::getline(ss, item, '-'))
    {
        value.m256i_u64[i++] = std::stoull(item);
    }
    return value;
}

static std::vector<unsigned long long> convertULLFromString(std::string& rStr)
{
    std::vector<unsigned long long> values;
    std::stringstream ss(rStr);
    std::string item;
    int i = 0;
    while (std::getline(ss, item, '-'))
    {
        values.push_back(std::stoull(item));
    }
    return values;
}

} // test_utils
