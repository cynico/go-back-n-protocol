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
#include "CustomMessage_m.h"
#include <cstdio>
#include <string>

Define_Module(Node);

void Node::initialize()
{
    this->id = int(this->getName()[strlen(this->getName())-1] - '0');
    char fileName [32];
    int status = std::snprintf(fileName, 32, "..\\src\\input%d.txt", this->id);
    if (status < 0)
        throw std::runtime_error("Failed to format file name in node: " + std::to_string(this->id));

    input = new TextFile(std::string(fileName));

    this->wEnd = this->wStart + int(getParentModule()->par("WS"));
}

void Node::getInitializationInfo(CustomMessage_Base* msg) {
    std::istringstream iss(msg->getPayload());
    int startingNodeID;

    if ( !(iss >> startingNodeID >> this->startingTime) ) {
        std::cerr << "Invalid format of the coordinator message payload" << std::endl;
        exit(1);
    }

    if (startingNodeID == this->id) {
        sender = true;
        scheduleAt(startingTime, new CustomMessage_Base(""));
    }
    return;
}

void Node::handleMessage(cMessage *msg)
{
    CustomMessage_Base* customMsg = check_and_cast<CustomMessage_Base *>(msg);

    // If this is the (initial) coordinator message
    if ((int)customMsg->getFrameType() == 3) {
        getInitializationInfo(customMsg);
    }

    // If this is a self-message: either this is the initial self-message
    // or this is a timeout. Either case, start sending all messages again that fall
    // within the window.
    else if (msg->isSelfMessage()) {

//        for (int i = wStart; i < wEnd; i++) {
//            std::bitset<8> payload(input->ReadNthLine(currentLineOffset+i));
//            CustomMessage_Base* msg = new CustomMessage_Base();
//            msg->setPayload(line);
//            msg->setFrameType(DATA_FRAME);
//
//        }
        CustomMessage_Base* msg = new CustomMessage_Base("Hello there");
        send(msg, "peer$o");
        return;
    }

    // Otherwise
    else {

        EV << "Received: " << msg->getName() << std::endl;
    }
}
