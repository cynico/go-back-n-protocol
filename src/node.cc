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

#include "node.h"
#include <stdio.h>

Define_Module(Node);

template<typename... Args> void Node::log(const char * f, Args... args) {

    char buffer[1024];
    if (snprintf(buffer, 1024, f, args...) < 0) std::cerr << "Error formatting buffer" << std::endl;

    Info::log << buffer << std::endl;
    Info::log.flush();

    EV << buffer << std::endl;
}

void Node::initialize()
{
    this->id = int(this->getName()[strlen(this->getName())-1] - '0');
    char fileName [32];
    int status = std::snprintf(fileName, 32, "..\\src\\input%d.txt", this->id);
    if (status < 0)
        throw std::runtime_error("Failed to format file name in node: " + std::to_string(this->id));

    input = new TextFile(std::string(fileName));
}

void Node::getInitializationInfo(CustomMessage_Base* msg) {
    std::istringstream iss(ConvertBitsToString(*msg->getPayload()));
    int startingNodeID;

    if ( !(iss >> startingNodeID >> this->senderInfo.startingTime) ) {
        std::cerr << "Invalid format of the coordinator message payload" << std::endl;
        exit(1);
    }

    if (startingNodeID == this->id) {
        isSender = true;

        // We only need to open the file if we are the sender.
        this->input->OpenFile();
        this->input->SetBufferSize(Info::windowSize);

        // Starting operation at the set starting time
        CustomMessage_Base* initialMsg = new CustomMessage_Base("");
        initialMsg->setAckSequence(-1);
        scheduleAt(this->senderInfo.startingTime, initialMsg);
    }
    return;
}

int Node::modulus(int a) {
    int b = Info::windowSize + 1;
    return ((a % b) + b) % b;
}

int Node::modifyPayload(vecBitset8 &payload) {
    int bit = intuniform(0,payload.size()*8);
    payload[int(bit/8)][bit%8] = (~payload[int(bit/8)][bit%8]);
    return bit;
}

bool Node::sendDataFrame(int lineNumber, int dataSequenceNumber, bool errorFree, float &delay) {

    std::string line;

    ReadLineResult r = this->input->ReadNthLine(lineNumber, line);
    if (!r.moreLinesToRead) return false;

    vecBitset8 payload = ConvertStringToBits(line.substr(5), true);

    CustomMessage_Base* msgToSend = new CustomMessage_Base();
    msgToSend->setFrameType(DATA_FRAME);
    msgToSend->setParity(CalculateChecksum(payload));
    msgToSend->setDataSequence(dataSequenceNumber);

    this->log("At time [%.3f], Node [%d], Introducing channel error with code = [%s]", simTime().dbl() + this->senderInfo.offsetFromRealTime, this->id, line.substr(0,4).c_str());

    bool lost = false, duplicated = false;
    int modifiedBit = -1;
    float errorDelay = 0;

    // Applying errors..
    if (!errorFree) {

        // Modification
        std::bitset<4> errorPrefix(line.substr(0, 4));
        if (errorPrefix.test(3)) modifiedBit = this->modifyPayload(payload);

        // Loss
        lost = errorPrefix.test(2);

        // Duplication
        duplicated = errorPrefix.test(1);

        // Delay
        if (errorPrefix.test(0)) errorDelay = Info::errorDelay;
    }

    float processingTime = (int)!r.readFromBuffer * Info::processingTime;
    this->senderInfo.offsetFromRealTime += processingTime;

    // Log output
    this->log("At time [%.3f], Node [%d] [sent] frame with seq_num = [%d] and payload = [%s] and trailer = [%s], Modified [%d], Lost [%s], Duplicate [%d], Delay [%.3f]",
            simTime().dbl() + this->senderInfo.offsetFromRealTime, this->id, dataSequenceNumber, ConvertBitsToString(payload).c_str(),
            msgToSend->getParity().to_string().c_str(), modifiedBit, lost? "Yes": "No", (int)duplicated, errorDelay);

    msgToSend->setPayload(payload);
    delay = this->senderInfo.offsetFromRealTime + Info::transmissionDelay;

    // If duplicated, send a duplicate message with the duplication delay.
    if (duplicated) {
        this->log("At time [%.3f], Node [%d] sent frame with seq_num = [%d] and payload = [%s] and trailer = [%s], Modified [%d], Lost [%s], Duplicate [%d], Delay [%.3f]",
                simTime().dbl() + this->senderInfo.offsetFromRealTime, this->id, dataSequenceNumber, ConvertBitsToString(payload).c_str(),
                msgToSend->getParity().to_string().c_str(), modifiedBit, lost? "Yes": "No", 2, errorDelay);
        if (!lost) {
            CustomMessage_Base* duplicatedMsg = msgToSend->dup();
            sendDelayed(duplicatedMsg, delay + Info::duplicationDelay + errorDelay, "peer$o");
        }
    }

    if (lost) delete msgToSend;
    else sendDelayed(msgToSend, delay + errorDelay, "peer$o");

    return true;
}

Timer* Node::createTimer(int ackSequence) {
    Timer* timer = NULL;
    if (!this->senderInfo.timers) {
        this->senderInfo.timers = new Timer();
        timer = this->senderInfo.timers;
    }
    else {
        Timer* t = this->senderInfo.timers;
        while (t) {
            if (!t->next) break;
            t = t->next;
        }
        t->next = new Timer();
        timer = t->next;
        timer->prev = t;
    }
    timer->msg = new CustomMessage_Base();
    timer->msg->setAckSequence(ackSequence);
    return timer;
}

Timer* Node::deleteTimers(int ackSequence, bool prev) {

    Timer* t = this->senderInfo.timers;

    while (t) {
        if (t->msg->getAckSequence() == this->modulus(ackSequence))
            break;
        t = t->next;
    }

    if (t) {

        if (prev) {
            this->senderInfo.timers = t->next;
            if (this->senderInfo.timers) this->senderInfo.timers->prev = NULL;
        } else {
            if (t->prev) t->next = NULL;
            else this->senderInfo.timers = NULL;
        }

        Timer* temp;
        while (t) {
            temp = prev ? t->prev : t->next;
            cancelAndDelete(t->msg);
            delete t;
            t = temp;
        }
    }

    return this->senderInfo.timers;
}

// prev is true on ack, false on nack
void Node::advanceWindowAndSendFrames(int ackSequence, bool prev) {

    this->senderInfo.offsetFromRealTime = 0;

    int oldWStart = this->senderInfo.wStart;

    // Updating wCurrent with the (n)ack sequence number.
    // This will be the sequence number of the next frame to send.
    this->senderInfo.wStart = ackSequence;

    // Updating the line offset with the difference that we advanced wCurrent with.
    this->senderInfo.currentLineOffset += this->modulus(this->senderInfo.wStart - oldWStart);

    // Canceling the timeout for this acknowledgment.
    // In case of ack: the timers for the preceding frames.
    // In case of nack: the timers of all frames (the preceding are acknowledged, and the following are resent)
    // and all the timeouts for ack preceding/following this one (accumulative n(ack))
    int startingSeq = prev ? this->modulus(ackSequence - 1) : this->senderInfo.timers->msg->getAckSequence();
    this->deleteTimers(startingSeq, prev);

    // Advancing wCurrent and sending new frames as much as our window can expand.
    float delay;
    bool errorFree = prev ? false : true; // errorFree is true for the first message to be sent in case of nack.

    int framesToSend = Info::windowSize - this->modulus(this->senderInfo.wCurrent - this->senderInfo.wStart);

    // Log output
    if (prev)
        this->log("At time [%.3f], Node [%d] received ack with seq_num = [%d]. Advancing window and sending %d frames.", simTime().dbl(), this->id,
                ackSequence, framesToSend);
    else
        this->log("At time [%.3f], Node [%d] received nack with seq_num = [%d]. Resending %d frames, starting from seq_num = [%d]", simTime().dbl(), this->id,
                ackSequence, framesToSend, this->senderInfo.wCurrent);


    for (int i = framesToSend; i > 0; i--) {

        int lineNumber = this->senderInfo.currentLineOffset + this->modulus(this->senderInfo.wCurrent - this->senderInfo.wStart);
        int dataSequenceNumber = this->senderInfo.wCurrent;
        if (!this->sendDataFrame(lineNumber, dataSequenceNumber, errorFree, delay)) {
            this->log("At time [%.3f], Node [%d] found no more lines to send. Checking if we should terminate now", simTime().dbl() + this->senderInfo.offsetFromRealTime, this->id);
            this->checkTermination();
            this->log("At time [%.3f], Node [%d] is waiting for outstanding acks to terminate.", simTime().dbl() + this->senderInfo.offsetFromRealTime, this->id);
            return;
        }

        // Setting errorFree to false in case of nack.
        // Increasing the delay by 0.001 as per the document.
        if (errorFree) {
            delay += 0.001;
            errorFree = false;
        }

        // Create a timer
        Timer* timer = this->createTimer(dataSequenceNumber);
        scheduleAt( simTime() + delay + Info::timeout, timer->msg);
        this->senderInfo.wCurrent = this->modulus(this->senderInfo.wCurrent + 1);
    }

    return;
}

void Node::sender(CustomMessage_Base *msg) {

    // If this is a self-message: either this is the initial self-message
    // or this is a timeout. Either case, start sending all messages again that fall
    // within the window.
    if (msg->isSelfMessage()) {

        int timedoutSequenceNumber = msg->getAckSequence();
        if (timedoutSequenceNumber != -1) this->log("At time [%.3f], Node [%d] timeout event for frame with seq_num = [%d]", simTime().dbl() + this->senderInfo.offsetFromRealTime, this->id, timedoutSequenceNumber);

        // Cancel all timers starting from the timer that expired and onwards, i.e: all timers.
        // Clarification: if a timer expired, it MUST have been the first timer in the window, by the very
        // structure of the way acknowledgements are serially handled in the code.
        if (this->senderInfo.timers) this->deleteTimers(this->senderInfo.timers->msg->getAckSequence(), false);

        int dataSequenceNumber, lineNumber;
        float delay; bool errorFree = false;
        this->senderInfo.offsetFromRealTime = 0;
        for (int i = 0; i < Info::windowSize; i++) {

            dataSequenceNumber =  this->modulus(this->senderInfo.wStart + i);
            lineNumber = this->senderInfo.currentLineOffset + i;
            errorFree = false;

            // If this is the message that caused the timeout, then send it error-free.
            if (dataSequenceNumber == timedoutSequenceNumber) errorFree = true;

            if (!this->sendDataFrame(lineNumber, dataSequenceNumber, errorFree, delay))
                return this->checkTermination();

            Timer* timer = this->createTimer(dataSequenceNumber);
            scheduleAt( simTime() + delay + Info::timeout, timer->msg);
        }

        senderInfo.wCurrent = this->modulus(senderInfo.wStart + Info::windowSize);
    }

    // We received an ACK or NACK.
    else {
        int sequenceNumber = msg->getAckSequence();
        switch (msg->getFrameType()) {
        case ACK_FRAME: {
            this->advanceWindowAndSendFrames(sequenceNumber, true);
            break;
        }

        case NACK_FRAME: {
            this->senderInfo.wCurrent = sequenceNumber;
            this->advanceWindowAndSendFrames(sequenceNumber, false);
            break;
        }

        default: break;
        }

        cancelAndDelete(msg);
    }
}

void Node::checkTermination() {
    if (this->senderInfo.wStart == this->senderInfo.wCurrent) {
        this->log("At time [%.3f], Node [%d] finished sending and receiving acks.", simTime().dbl() + this->senderInfo.offsetFromRealTime, this->id);
        exit(0);
    }
}

void Node::sendAck(int ACK_TYPE, bool LP) {

    bool lost = false;

    CustomMessage_Base* ack = new CustomMessage_Base();
    ack->setFrameType(ACK_TYPE);
    ack->setAckSequence(this->receiverInfo.expectedFrameSequence);

    if ((uniform(0,1) <= Info::ackLossProb) && LP) lost = true;
    else {

        this->receiverInfo.accumulatingProcessingTimes += Info::processingTime;
        sendDelayed(ack, this->receiverInfo.accumulatingProcessingTimes + Info::transmissionDelay, "peer$o");
    }

    this->log("At time [%.3f], Node [%d] sending [%s] with number [%d], loss [%s]", simTime().dbl() + this->receiverInfo.accumulatingProcessingTimes, this->id,
            ACK_TYPE ? "ACK" : "NACK", ack->getAckSequence(), lost? "Yes" : "No");

    if (lost) delete ack;
}

void Node::receiver(CustomMessage_Base *msg) {

    // If not an in-order message, drop and send an ack with the sequence of the next expected frame sequence.
    if (this->receiverInfo.lastAckSimTime != simTime().dbl()) {
        this->receiverInfo.lastAckSimTime = simTime().dbl();
        this->receiverInfo.accumulatingProcessingTimes = 0;
    }

    if (msg->getDataSequence() != this->receiverInfo.expectedFrameSequence) {
        this->log("At time [%.3f], Node [%d] dropped out-of-order frame with seq_num = [%d]. Expecting seq_num = [%d]", simTime().dbl() + this->receiverInfo.accumulatingProcessingTimes, this->id, msg->getDataSequence(), this->receiverInfo.expectedFrameSequence);
        cancelAndDelete(msg);
        this->sendAck(ACK_FRAME, false);
        return;
    }

    // Verify the checksum of the message
    bool valid = VerifyChecksum(*(msg->getPayload()), msg->getParity());
    this->receiverInfo.expectedFrameSequence += valid ? 1 : 0;
    this->receiverInfo.expectedFrameSequence = this->modulus(this->receiverInfo.expectedFrameSequence);

    if (valid) this->log("Uploading payload = \"%s\" and seq_num = [%d] to the network layer", ConvertBitsToString(*(msg->getPayload()), true).c_str(), msg->getDataSequence());
    this->sendAck(int(valid));

    cancelAndDelete(msg);
    return;
}

void Node::handleMessage(cMessage *msg)
{
    CustomMessage_Base* customMsg = check_and_cast<CustomMessage_Base *>(msg);

    // If this is the (initial) coordinator message
    if ((int)customMsg->getFrameType() == COORDINATOR_FRAME) {
        getInitializationInfo(customMsg);
        cancelAndDelete(msg);
        return;
    }

    // Call the appropriate function
    if (this->isSender) this->sender(customMsg);
    else this->receiver(customMsg);

}
