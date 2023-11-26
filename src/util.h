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

#ifndef UTIL_H_
#define UTIL_H_

#include <fstream>
#include <stdio.h>
#include <vector>
#include <bitset>

typedef std::bitset<8> bitset8;
typedef std::vector<bitset8> vecBitset8;

class Info {
public:
    Info() = delete;
    static int windowSize;
    static double timeout;
    static double processingTime;
    static double transmissionDelay;
    static double errorDelay;
    static double duplicationDelay;
    static double ackLossProb;

    static std::FILE* log;
};

class TextFile {
    std::string fileName;
    std::ifstream file_stream;
    std::vector<std::ifstream::streampos> lineBeginnings;
public:
    TextFile();
    TextFile(std::string fileName);
    TextFile(TextFile&& b);
    std::string ReadNthLine(int N);
    virtual ~TextFile();
};


vecBitset8 ConvertStringToBits(std::string const& message, bool frame = false);
std::string ConvertBitsToString(vecBitset8 const& bytes, bool deframe = false);
bool VerifyChecksum(vecBitset8 const &bytes, bitset8 const& checksum);
bitset8 CalculateChecksum(vecBitset8 const &bytes);

#endif /* UTIL_H_ */
