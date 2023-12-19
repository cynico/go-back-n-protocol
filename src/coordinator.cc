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

#include "coordinator.h"
#include "CustomMessage_m.h"
#include "util.h"
Define_Module(Coordinator);

void Coordinator::setGlobalInfo()
{
    // Set shared info according to parameters
    Info::windowSize = (int)getParentModule()->par("WS");
    Info::timeout = getParentModule()->par("TO").doubleValue();
    Info::processingTime = getParentModule()->par("PT").doubleValue();
    Info::transmissionDelay = getParentModule()->par("TD").doubleValue();
    Info::errorDelay = getParentModule()->par("ED").doubleValue();
    Info::duplicationDelay = getParentModule()->par("DD").doubleValue();
    Info::ackLossProb = getParentModule()->par("LP").doubleValue();

    Info::log.open("..\\log.txt");
    if (!Info::log.good())
        throw std::runtime_error("Error opening log file.");
}

void Coordinator::initialize()
{
    this->setGlobalInfo();

    // Opening coordinator file and reading starting info.
    coordinatorFile.open("..\\src\\coordinator.txt");
    if (!coordinatorFile.good()) {
        std::cerr << "Error opening the coordinator.txt file" << std::endl;
        exit(1);
    }

    std::string line;
    std::getline(coordinatorFile, line);
    coordinatorFile.close();

    // Sending the starting info the the nodes
    CustomMessage_Base *initMsg = new CustomMessage_Base();
    initMsg->setPayload(ConvertStringToBits(line));
    initMsg->setFrameType(COORDINATOR_FRAME);
    send(initMsg, "node0$o");
    send(initMsg->dup(), "node1$o");
}

void Coordinator::handleMessage(cMessage *msg)
{
    // TODO - Generated method body
}
