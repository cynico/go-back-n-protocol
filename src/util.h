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
#include <vector>
#include <bitset>

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


std::vector<std::bitset<8>> ConvertStringToBits(std::string const& message);
std::string ConvertBitsToString(std::vector<std::bitset<8>> const& bytes);
bool VerifyChecksum(std::vector<std::bitset<8>> const &bytes, std::bitset<8> const& checksum);
std::bitset<8> CalculateChecksum(std::vector<std::bitset<8>> const &bytes);

#endif /* UTIL_H_ */
