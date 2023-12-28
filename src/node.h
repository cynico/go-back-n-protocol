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

struct DelayedMessage {
    CustomMessage_Base* N = NULL;
    double simTime = 0;
    double realTime = 0;
    DelayedMessage* next = NULL;
    DelayedMessage* prev = NULL;
    bool lost = false;
    double extraDelay = 0;
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

    linkedList<DelayedMessage> delayedMessages;
};

struct ReceiverInfo {

    // The next expected frame sequence.
    int expectedFrameSequence = 0;

    linkedList<DelayedMessage> delayedMessages;
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
    virtual bool sendDataFrame(int lineNumber, int dataSequenceNumber, bool errorFree, float &realTime, bool timeout);

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

    virtual double insertIntoDelayed(linkedList<DelayedMessage> *l, CustomMessage_Base* msg, bool lost, double processingTime, double extraDelay=0);
    virtual double getRealTime(linkedList<DelayedMessage> *l);
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
