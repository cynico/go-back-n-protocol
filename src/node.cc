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

    if ( !(iss >> startingNodeID >> this->startingTime) ) {
        std::cerr << "Invalid format of the coordinator message payload" << std::endl;
        exit(1);
    }

    if (startingNodeID == this->id) {
        isSender = true;
        scheduleAt(startingTime, new CustomMessage_Base(""));
    }

    return;
}

void Node::sender(CustomMessage_Base *msg) {

    // If this is a self-message: either this is the initial self-message
    // or this is a timeout. Either case, start sending all messages again that fall
    // within the window.
    if (msg->isSelfMessage()) {


        for (int i = 0; i < Info::windowSize; i++) {

            CustomMessage_Base* msgToSend = new CustomMessage_Base();

            vecBitset8 payload = ConvertStringToBits(input->ReadNthLine(currentLineOffset+i));
            msgToSend->setPayload(payload);
            msgToSend->setFrameType(DATA_FRAME);
            msgToSend->setParity(CalculateChecksum(payload));
            msgToSend->setDataSequence((wCurrent + i) % (Info::windowSize + 1));

            sendDelayed(msgToSend, Info::processingTime + Info::transmissionDelay, "peer$o");
        }

    }

    // We received an ACK or NACK.
    else {
        switch (msg->getFrameType()) {
        case ACK_FRAME: {

            int oldWCurrent = wCurrent;
            wCurrent = msg->getAckSequence();

            // Updating the line offset with the difference that we advanced wCurrent with.
            currentLineOffset += msg->getAckSequence() - oldWCurrent;

            // Circular sequence numbers. Had to be done after the above line for correct subtraction.
            wCurrent = wCurrent % (Info::windowSize + 1);

        }

        case NACK_FRAME: {
        }

        default: return;
        }
    }
}

void Node::receiver(CustomMessage_Base *msg) {

    // Verify the checksum of the message
    bool valid = VerifyChecksum(*(msg->getPayload()), msg->getParity());
    if (valid) {
        CustomMessage_Base* ack = new CustomMessage_Base();
        ack->setFrameType(ACK_FRAME);
        ack->setAckSequence((msg->getDataSequence()+1) % (Info::windowSize));

        std::fprintf(Info::log, "Uploading payload = %s and seq_num = %d to the network layer\n", ConvertBitsToString(*(msg->getPayload()), true), msg->getDataSequence());
    } else {
        // NACK
    }

    // Deleting the received message, we no longer need it.
    cancelAndDelete(msg);
}


void Node::handleMessage(cMessage *msg)
{
    CustomMessage_Base* customMsg = check_and_cast<CustomMessage_Base *>(msg);

    // If this is the (initial) coordinator message
    if ((int)customMsg->getFrameType() == 3) {
        getInitializationInfo(customMsg);
        return;
    }

    // Call the appropriate function
    if (this->isSender)
        this->sender(customMsg);
    else
        this->receiver(customMsg);

}
