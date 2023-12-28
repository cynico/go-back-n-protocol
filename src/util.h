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

typedef std::bitset<8> bitset8;
typedef std::vector<bitset8> vecBitset8;

// Singly-linked list node for buffered text lines.
struct TextLine {

    std::string line;

    // The number of the text line in the file.
    int N;

    TextLine* next = NULL;
    TextLine* prev = NULL;

};

// Result struct for TextFile::ReadNthLine
// moreLinesToRead => Whether there was more lines to read from the text file.
// readFromBuffer  => Whether the read line was read from the buffered lines, or from disk.
struct ReadLineResult {
    bool moreLinesToRead = true;
    bool readFromBuffer = false;
    ReadLineResult(bool moreLinesToRead, bool readFromBuffer) : moreLinesToRead(moreLinesToRead), readFromBuffer(readFromBuffer) {}
};

template <typename T>
struct linkedList {
    T* start = NULL;
    T* end = NULL;
    int size = -1;
    int availSpace = -1;
};

class Info {
public:
    Info() = delete;

    // All the parameters set in the Newtork.
    static int windowSize;
    static double timeout;
    static double processingTime;
    static double transmissionDelay;
    static double errorDelay;
    static double duplicationDelay;
    static double ackLossProb;

    // The log file shared between sender and receiver `output.txt`
    static std::ofstream log;
};

// Based on: https://stackoverflow.com/a/7273804
class TextFile {
    std::string fileName;
    std::ifstream file_stream;
    linkedList<TextLine> bufferedLines;
public:
    std::vector<std::ifstream::streampos> lineBeginnings;
    TextFile();
    TextFile(std::string fileName);
    TextFile(TextFile&& b);
    ReadLineResult ReadNthLine(int N, std::string &s);
    void SetBufferSize(int bufferSize);
    void OpenFile();
    virtual ~TextFile();
};


// Conversion from string to bitsets, and vice versa.
// Optional framing/deframing of messages.
vecBitset8 ConvertStringToBits(std::string const& message, bool frame = false);
std::string ConvertBitsToString(vecBitset8 const& bytes, bool deframe = false);

// Calculation and verification of checksums.
bool VerifyChecksum(vecBitset8 const &bytes, bitset8 const& checksum);
bitset8 CalculateChecksum(vecBitset8 const &bytes);

// Insertion, deletion, and retrieval of elements in a singly-linked list, optionally with size-constrained.
// Despite being generic, only currently used in TextFile.
// I also realize now I could've just used a ready one from stl
template<typename T, typename N> void DeleteElementFromLinkedList(linkedList<T>* l, N i) {
    for (T* it = l->start; it; it = it->next) {
        if (it->N == i) {

            if (it->next) it->next->prev = it->prev;
            if (it->prev) it->prev->next = it->next;

            if (it == l->start) l->start = l->start->next;
            if (it == l->end) l->end = (it->prev) ? it->prev : NULL;

            if (l->size != -1) l->availSpace += 1;

            delete it;
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
        T* oldEnd = l->end;
        l->end->next = new T();
        l->end = l->end->next;
        l->end->prev = oldEnd;
    }
}

template<typename T, typename N> T* GetElementFromLinkedList(linkedList<T>* l, N i) {

    for (T* it = l->start; it; it = it->next)
        if (it->N == i) return it;

    return NULL;
}


#endif /* UTIL_H_ */
