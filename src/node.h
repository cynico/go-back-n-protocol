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

#ifndef __PROJCET_NODE_H_
#define __PROJCET_NODE_H_

#include <omnetpp.h>
#include <fstream>
#include "CustomMessage_m.h"
#include "util.h"
using namespace omnetpp;

// Doubly-linked list node for the timers.
struct Timer {
    CustomMessage_Base* msg = NULL;
    Timer* next = NULL;
    Timer* prev = NULL;
};

struct SenderInfo {

    // Sequence number of the start of the window.
    int wStart = 0;

    // The next sequence number to be sent.
    int wCurrent = 0;

    // The line number in the text file corresponding to the
    // current wStart.
    int currentLineOffset = 0;

    // The starting time the sender should start sending frames.
    // Received from the coordinator.
    float startingTime = 0.0;

    // This is a vector of self-messages, acting as timeouts.
    Timer* timers = NULL;


    // Ad-hoc solution coming your way..

    // Used to calculate the right time to schedule messages.
    // Specifically, consider the following example, with the following parameters:
    // simTime() = 0, Info::windowSize = 3, Info::processingTime = 0.5

    // The sender sends:
    // First frame:
    // Sent at = simTime() + Info::processingTime = 0 + 0.5 = 0.5
    // Given that the sender doesn't either: busy-wait, or schedules a self-message,
    // it will proceed to the next frame

    // Second frame:
    // Sent at = simTime() + Info::processingTime = 0 + 0.5 = 0.5 => Not accurate

    // Which is the same as the time for the first frame, because it didn't
    // account for the time supposedly spent processing the frame.
    // We fix this by accumulating the processing times in offsetFromRealTime
    // so the way it goes:

    // First frame:
    // Sent at = simTime() + offsetFromRealTime + Info::processingTime = 0 + 0 + 0.5 = 0.5
    // offsetFromRealTime = offsetFromRealTime + Info::processingTime = 0 + 0.5 = 0.5

    // Second frame:
    // Sent at = simTime() + offsetFromRealTime + Info::processingTime = 0 + 0.5 + 0.5 = 1 => Accurate
    // offsetFromRealTime = offsetFromRealTime + Info::processingTime = 0.5 + 0.5 = 1

    // After sending all the frames our window can accomodate, we zero out this offsetFromRealTime.
    float offsetFromRealTime = 0;
};

struct ReceiverInfo {

    // The next expected frame sequence.
    int expectedFrameSequence = 0;

    // This serves a roughly similar role to senderInfo.offsetFromRealTime described above.
    // Specifically in the case when the senders send multiple frames at the same time t.
    // This happens when those frames are being resent, either due to a timeout or an NACK,
    // thus no processing time at the sender's end.

    // All the frames would thus be received at the same time t`. If we didn't handle it,
    // all the ack frames would be sent at (t` + 0.5), which is not logical.
    float lastAckSimTime = -1.0;
    float accumulatingProcessingTimes = 0.0;
};

class Node : public cSimpleModule
{
  protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);

    // Handles the initial message from the coordinator, and sets
    // the relevant data members accordingly.
    virtual void getInitializationInfo(CustomMessage_Base* msg);

    // Main function of the sender node.
    virtual void sender(CustomMessage_Base *msg);

    // Main function of the receiver node.
    virtual void receiver(CustomMessage_Base *msg);

    // Checks if the sender should terminate operation: when there is no more outstanding
    // frames. Called in sendDataFrame() below when there is no more lines to read from the file.
    virtual void checkTermination();

    // Sends a data frame with payload = the line with the given line number in the text
    // file, the sequence number of the frame to be sent, whether it should be errorFree,
    // calculates the delay and returns it in the parameter delay.
    // Returns true if there was more lines to read from the file, and a frame was successfully sent,
    // false otherwise.
    virtual bool sendDataFrame(int lineNumber, int dataSequenceNumber, bool errorFree, float &delay);

    // Sends an ACK/NACK, applying the ack loss probability if LP = true
    //
    virtual void sendAck(int ACK_TYPE, bool LP = true);

    // Performs mod operation that is valid for
    // negative numbers as well as positive numbers.
    virtual int modulus(int a);

    // Modifies a random bit in the payload in place, and returns the
    // position of modified bit.
    virtual int modifyPayload(vecBitset8 &payload);

    // Creates a timer with the given ackSequence number, inserts it into
    // the doubly-linked list of timers and returns it.
    virtual Timer* createTimer(int ackSequence);

    // Deletes a timer with the given ackSequence number, and removes it from
    // the doubly-linked list, and all timers after or before it.
    // Returns the head of the linked list after deletion of timers.
    virtual Timer* deleteTimers(int ackSequence, bool prev);

    // On receiving an ack/nack sequence at the sender, calculates and advances
    // the window and sends new frames.
    virtual void advanceWindowAndSendFrames(int ackSequence, bool prev);

    // Logs the given printf-style string and format to omnte++ simulation stdout,
    // and the log file `output.txt`
    template<typename... Args> void log(const char * f, Args... args);
  private:

    // The id of the node.
    int id;

    // The input text file of the node.
    TextFile *input;

    // Whether the node is the sender.
    bool isSender = false;

    struct SenderInfo senderInfo;
    struct ReceiverInfo receiverInfo;
};

#endif
