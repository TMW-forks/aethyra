/*
 *  The Mana World
 *  Copyright 2004 The Mana World Development Team
 *
 *  This file is part of The Mana World.
 *
 *  The Mana World is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  any later version.
 *
 *  The Mana World is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with The Mana World; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  $Id$
 */

#include "npc.h"

#include <sstream>

#include "button.h"
#include "scrollarea.h"
#include "listbox.h"

#include "../game.h"

#include "../net/messageout.h"
#include "../net/protocol.h"

NpcListDialog::NpcListDialog():
    Window("NPC")
{
    mItemList = new ListBox(this);
    ScrollArea *scrollArea = new ScrollArea(mItemList);
    Button *okButton = new Button("OK");
    Button *cancelButton = new Button("Cancel");

    setContentSize(260, 175);
    scrollArea->setHorizontalScrollPolicy(gcn::ScrollArea::SHOW_NEVER);
    scrollArea->setDimension(gcn::Rectangle(
                5, 5, 250, 160 - okButton->getHeight()));
    cancelButton->setPosition(
            260 - 5 - cancelButton->getWidth(),
            175 - 5 - cancelButton->getHeight());
    okButton->setPosition(
            cancelButton->getX() - 5 - okButton->getWidth(),
            cancelButton->getY());

    mItemList->setEventId("item");
    okButton->setEventId("ok");
    cancelButton->setEventId("cancel");

    mItemList->addActionListener(this);
    okButton->addActionListener(this);
    cancelButton->addActionListener(this);

    add(scrollArea);
    add(okButton);
    add(cancelButton);

    setLocationRelativeTo(getParent());
}

int
NpcListDialog::getNumberOfElements()
{
    return items.size();
}

std::string
NpcListDialog::getElementAt(int i)
{
    return items[i];
}

void
NpcListDialog::parseItems(const std::string &itemString)
{
    std::istringstream iss(itemString);

    std::string tmp;
    while(getline(iss, tmp, ':')) {
        items.push_back(tmp);
    }
}

void
NpcListDialog::reset()
{
    items.clear();
}

void
NpcListDialog::action(const std::string& eventId)
{
    int choice = 0;

    if (eventId == "ok")
    {
        // Send the selected index back to the server
        int selectedIndex = mItemList->getSelected();
        if (selectedIndex > -1)
        {
            choice = selectedIndex + 1;
        }
    }
    else if (eventId == "cancel")
    {
        choice = 0xff; // 0xff means cancel
    }

    if (choice)
    {
        MessageOut outMsg;
        outMsg.writeInt16(CMSG_NPC_LIST_CHOICE);
        outMsg.writeInt32(current_npc);
        outMsg.writeInt8(choice);
        setVisible(false);
        reset();
        current_npc = 0;
    }
}
