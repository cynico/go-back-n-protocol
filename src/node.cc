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
#include <cstdio>

Define_Module(Node);

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

        // Starting operation at the set starting time
        CustomMessage_Base* initialMsg = new CustomMessage_Base("");
        initialMsg->setAckSequence(-1);
        scheduleAt(this->senderInfo.startingTime, initialMsg);
    }
    return;
}

// This function performs mod operation that is valid for
// negative numbers as well as positive numbers.
int Node::modulus(int a) {
    int b = Info::windowSize + 1;
    return ((a % b) + b) % b;
}

void Node::modifyPayload(vecBitset8 &payload) {
    int bit = intuniform(0,payload.size()*8);
    payload[int(bit/8)][bit%8] = (~payload[int(bit/8)][bit%8]);
}

bool Node::sendDataFrame(int lineNumber, int dataSequenceNumber, bool errorFree, float &delay) {

    std::string line;
    if (!input->ReadNthLine(lineNumber, line)) return false;

    vecBitset8 payload = ConvertStringToBits(line.substr(5), true);

    CustomMessage_Base* msgToSend = new CustomMessage_Base();
    msgToSend->setFrameType(DATA_FRAME);
    msgToSend->setParity(CalculateChecksum(payload));
    msgToSend->setDataSequence(dataSequenceNumber);

    // Applying errors..
    if (!errorFree) {

        std::bitset<4> errorPrefix(line.substr(0, 4));
        if (errorPrefix.test(3)) this->modifyPayload(payload);
        if (errorPrefix.test(2)) {
            delete msgToSend;
            return true;
        }

    }

    msgToSend->setPayload(payload);

    delay = Info::processingTime + Info::transmissionDelay;
    sendDelayed(msgToSend, delay, "peer$o");
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
    for (int i =  Info::windowSize - this->modulus(this->senderInfo.wCurrent - this->senderInfo.wStart); i > 0; i--) {

        int lineNumber = this->senderInfo.currentLineOffset + this->modulus(this->senderInfo.wCurrent - this->senderInfo.wStart);
        int dataSequenceNumber = this->senderInfo.wCurrent;
        if (!this->sendDataFrame(lineNumber, dataSequenceNumber, errorFree, delay)) return this->checkTermination();

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
}

void Node::sender(CustomMessage_Base *msg) {

    // If this is a self-message: either this is the initial self-message
    // or this is a timeout. Either case, start sending all messages again that fall
    // within the window.
    if (msg->isSelfMessage()) {

        std::cout << "A timer has expired. Sending all outstanding messages. Ack Sequence = " << msg->getAckSequence() << std::endl;
        int timedoutSequenceNumber = msg->getAckSequence();

        // Cancel all timers
        if (this->senderInfo.timers)
            this->deleteTimers(this->senderInfo.timers->msg->getAckSequence(), false);

        int dataSequenceNumber, lineNumber;
        float delay; bool errorFree = false;
        for (int i = 0; i < Info::windowSize; i++) {

            dataSequenceNumber =  this->modulus(this->senderInfo.wStart + i);
            lineNumber = this->senderInfo.currentLineOffset + i;
            errorFree = false;
            std::cout << "dataSequenceNumber: " << dataSequenceNumber << " lineNumber: " << lineNumber << std::endl;

            // If this is the message that caused the timeout, then send it error-free.
            if (dataSequenceNumber == timedoutSequenceNumber) errorFree = true;

            if (!this->sendDataFrame(lineNumber, dataSequenceNumber, errorFree, delay))
                return this->checkTermination();

            Timer* timer = this->createTimer(dataSequenceNumber);
            scheduleAt( simTime() + delay + Info::timeout, timer->msg);
        }

        senderInfo.wCurrent = this->modulus(senderInfo.wStart + Info::windowSize);

        cancelAndDelete(msg);
    }

    // We received an ACK or NACK.
    else {
        int sequenceNumber = msg->getAckSequence();
        switch (msg->getFrameType()) {
        case ACK_FRAME: {
            std::cout << "Received ack = " << sequenceNumber << std::endl;
            this->advanceWindowAndSendFrames(sequenceNumber, true);
            break;
        }

        case NACK_FRAME: {
            std::cout << "Received nack = " << sequenceNumber << std::endl;
            this->senderInfo.wCurrent = sequenceNumber;
            this->advanceWindowAndSendFrames(sequenceNumber, false);
            break;
        }

        default: break;
        }
    }

    // cancelAndDelete(msg);
}


void Node::checkTermination() {
    if (this->senderInfo.wStart == this->senderInfo.wCurrent) {
        std::cout << "Finishing operation" << std::endl;
        exit(0);
    }
}

void Node::sendAck(int ACK_TYPE, bool LP = true) {

    CustomMessage_Base* ack = new CustomMessage_Base();
    ack->setFrameType(ACK_TYPE);
    ack->setAckSequence(this->receiverInfo.expectedFrameSequence);

    // Apply the LP
    if ((uniform(0,1) <= Info::ackLossProb) && LP) {
        std::cerr << "Applying loss probability to " << (valid? "ack" : "nack") << " frame " << this->receiverInfo.expectedFrameSequence << std::endl;
        delete ack;
    }
    else
        sendDelayed(ack, Info::processingTime + Info::transmissionDelay, "peer$o");

}

void Node::receiver(CustomMessage_Base *msg) {

    CustomMessage_Base* ack = new CustomMessage_Base();

    // TODO: If not an in-order message, drop and send an ack with the sequence of the next expected frame sequence.
    if (msg->getDataSequence() != this->receiverInfo.expectedFrameSequence) {
        std::cerr << "Dropping out-of-order frame " << msg->getDataSequence() << ". Sending ack with sequence: " << this->receiverInfo.expectedFrameSequence << std::endl;
        this->sendAck(ACK_FRAME, false);
        return;
    }

    // Verify the checksum of the message
    bool valid = VerifyChecksum(*(msg->getPayload()), msg->getParity());
    this->receiverInfo.expectedFrameSequence += valid ? 1 : 0;
    this->receiverInfo.expectedFrameSequence = this->modulus(this->receiverInfo.expectedFrameSequence);

    this->sendAck(int(valid));
    if (valid) {
        Info::log << "Uploading payload = \"" + ConvertBitsToString(*(msg->getPayload()), true) << "\" and seq_num = " << msg->getDataSequence() << " to the network layer\n" << std::endl;
        Info::log.flush();
    } else std::cerr << "Received errored frame. Sending NACK" << std::endl;

}

void Node::handleMessage(cMessage *msg)
{
    CustomMessage_Base* customMsg = check_and_cast<CustomMessage_Base *>(msg);

    // If this is the (initial) coordinator message
    if ((int)customMsg->getFrameType() == 3) {
        getInitializationInfo(customMsg);
        cancelAndDelete(msg);
        return;
    }

    // Call the appropriate function
    if (this->isSender)
        this->sender(customMsg);
    else
        this->receiver(customMsg);

}
