//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#include <string>
#include <iostream>
#include "util.h"

TextFile::TextFile(std::string fileName) : file_stream(fileName), fileName(fileName)
{
    if (!file_stream.good())
        throw std::runtime_error("Error opening the file: " + fileName);

    std::string s;
    s.reserve(256);

    while(file_stream) {
        lineBeginnings.push_back(file_stream.tellg());
        std::getline(file_stream, s);
    }
}

std::string TextFile::ReadNthLine(int N) {
    if (N >= lineBeginnings.size()-1)
        throw std::runtime_error("File " + this->fileName + " doesn't have that many lines.");

    std::string s;

    // Clear EOF and error flags
    file_stream.clear();
    file_stream.seekg(lineBeginnings[N]);
    std::getline(file_stream, s);
    return s;
}

TextFile::TextFile() {
}

TextFile::~TextFile() {
}

std::vector<std::bitset<8>> ConvertStringToBits(std::string const &message) {
    std::vector<std::bitset<8>> bytes;
    for (const char &c : message) {
        std::bitset<8> byte(c);
        bytes.push_back(byte);
    }
    return bytes;
}

std::string ConvertBitsToString(std::vector<std::bitset<8>> const& bytes) {
    std::string message;
    message.reserve(bytes.size()+1);

    for (auto &byte : bytes)
        message += (char)byte.to_ulong();

    return message;
}


bool VerifyChecksum(std::vector<std::bitset<8>> const& bytes, std::bitset<8> const& checksum) {

    std::bitset<8> calculatedChecksum = CalculateChecksum(bytes);
    if (std::bitset<8>(calculatedChecksum ^ checksum).count() == 0)
        return true;

    return false;
}


std::bitset<8> CalculateChecksum(std::vector<std::bitset<8>> const &bytes) {

    std::bitset<9> intermediate(0);
    std::bitset<8> result(0);
    for (int i = 0; i < bytes.size(); i++) {

        intermediate = std::bitset<9>(result.to_ulong() + bytes[i].to_ulong());
        if (intermediate[8]) {
            intermediate.reset(8);
            intermediate = std::bitset<9>(intermediate.to_ulong() + 1);
            result = std::bitset<8>(intermediate.to_ulong());
        }
        else result = std::bitset<8>(intermediate.to_ulong());
    }

    result = ~result;
    return result;
}

template<typename T, typename F> void PrintVector(std::vector<T> v, std::string(T::*func)()){
    for (auto &element: v)
        if (method) std::cout << (element.*f)() << " ";
        else std::cout << element << " ";

    std::cout << std::endl;
}
