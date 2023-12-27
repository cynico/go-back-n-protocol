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
std::ofstream Info::log;

TextFile::TextFile(std::string fileName) : fileName(fileName)
{
}

void TextFile::OpenFile()  {
    this->file_stream.open(this->fileName, std::ios::binary);
    if (!this->file_stream.good())
        throw std::runtime_error("Error opening the file: " + this->fileName);

    std::string s;
    lineBeginnings.push_back(file_stream.tellg());
    while (file_stream) {
        std::getline(file_stream, s);
        if (s.empty()) continue; // To handle files that has an empty last line
        lineBeginnings.push_back(file_stream.tellg());
    }
}
void TextFile::SetBufferSize(int bufferSize) {
    this->bufferedLines.availSpace = this->bufferedLines.size = bufferSize;
}
ReadLineResult TextFile::ReadNthLine(int N, std::string &s) {

    if (N >= lineBeginnings.size() - 1) return ReadLineResult{false, false};

    // Clear EOF and error flags
    this->file_stream.clear();
    this->file_stream.seekg(lineBeginnings[N]);

    bool isBuffered = false;
    if (this->bufferedLines.start)
        if (N >= this->bufferedLines.start->N && N <= this->bufferedLines.end->N)
           isBuffered = true;

    if (isBuffered) {
        TextLine *t = GetElementFromLinkedList<TextLine, int>(&this->bufferedLines, N);
        s = t->line;
    } else {

        InsertAtEndOfLinkedList<TextLine, int>(&this->bufferedLines);

        this->bufferedLines.end->N = N;
        std::getline(this->file_stream, this->bufferedLines.end->line);
        this->bufferedLines.end->line = this->bufferedLines.end->line.substr(0, this->bufferedLines.end->line.length() - 1); // Excluding new line
        s = this->bufferedLines.end->line;
    }

    return ReadLineResult{true, isBuffered};
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

template<typename T, typename N> void DeleteElementFromLinkedList(linkedList<T>* l, N i) {
    for (auto it = l->start; it; it = it->next) {
        if (it->N == i) {

            T* toDelete = it;

            if (it == l->start) l->start = l->start->next;
            if (it == l->end) l->end = l->end->next;
            delete toDelete;

            if (l->size != -1) l->availSpace += 1;
            return;
        }
    }
}

template<typename T, typename N> void InsertAtEndOfLinkedList(linkedList<T>* l) {

    if (l->size != -1) {
        // Deleting the oldest element in the list if there is no free space.
        if (l->availSpace == 0) DeleteElementFromLinkedList<T, N>(l, l->start->N);
        l->availSpace -= 1;
    }

    // Inserting the new element at the end.
    if (!l->start) l->start = l->end = new T();
    else {
        l->end->next = new T();
        l->end = l->end->next;
    }
}

template<typename T, typename N> T* GetElementFromLinkedList(linkedList<T>* l, N i) {

    for (auto it = l->start; it; it = it->next) {
        if (it->N == i)
            return it;
    }

    return NULL;
}

