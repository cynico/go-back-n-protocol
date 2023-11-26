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

int Info::windowSize = 0;
double Info::timeout = 0;
double Info::processingTime = 0;
double Info::transmissionDelay = 0;
double Info::errorDelay = 0;
double Info::duplicationDelay = 0;
double Info::ackLossProb = 0;
std::FILE* Info::log;


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

vecBitset8 ConvertStringToBits(std::string const &message, bool frame) {

    vecBitset8 bytes;

    if (frame) bytes.push_back(bitset8('$'));
    for (const char &c : message) {
        if (frame && (c == '$' || c == '/')) bytes.push_back(bitset8('/'));
        bytes.push_back(bitset8(c));
    }
    if (frame) bytes.push_back(bitset8('$'));

    return bytes;
}

std::string ConvertBitsToString(vecBitset8 const& bytes, bool deframe) {

    std::string message;
    message.reserve(bytes.size()+1);

    int start = deframe ? 1 : 0;
    int end = deframe ? bytes.size() - 1 : bytes.size();

    for (int i = start; i < end; i++) {
        if (deframe && (bytes[i] == '/')) {
            message += (char)bytes[i+1].to_ulong();
            i = i + 1;
        } else
            message += (char)bytes[i].to_ulong();
    }

    return message;
}


bool VerifyChecksum(vecBitset8 const& bytes, bitset8 const& checksum) {

    bitset8 calculatedChecksum = CalculateChecksum(bytes);
    if (bitset8(calculatedChecksum ^ checksum).count() == 0)
        return true;

    return false;
}


bitset8 CalculateChecksum(vecBitset8 const &bytes) {

    std::bitset<9> intermediate(0);
    bitset8 result(0);
    for (int i = 0; i < bytes.size(); i++) {

        intermediate = std::bitset<9>(result.to_ulong() + bytes[i].to_ulong());
        if (intermediate[8]) {
            intermediate.reset(8);
            intermediate = std::bitset<9>(intermediate.to_ulong() + 1);
            result = bitset8(intermediate.to_ulong());
        }
        else result = bitset8(intermediate.to_ulong());
    }

    result = ~result;
    return result;
}

