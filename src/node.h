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


class Node : public cSimpleModule
{
  protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);
    virtual void getInitializationInfo(CustomMessage_Base* msg);
    virtual void sender(CustomMessage_Base *msg);
    virtual void receiver(CustomMessage_Base *msg);

  private:
    int id;
    TextFile *input;
    bool isSender = false;
    float startingTime;

    int wCurrent = 0;
    int currentLineOffset = 0;

};

#endif
