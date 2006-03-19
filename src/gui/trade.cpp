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
 *  $Id $
 */

#include "trade.h"

#include <sstream>

#include <guichan/widgets/label.hpp>

#include "button.h"
#include "chat.h"
#include "inventorywindow.h"
#include "item_amount.h"
#include "itemcontainer.h"
#include "scrollarea.h"
#include "textfield.h"

#include "../inventory.h"
#include "../item.h"

#include "../net/messageout.h"
#include "../net/protocol.h"

#include "../resources/iteminfo.h"

#include "../utils/tostring.h"

TradeWindow::TradeWindow(Network *network):
    Window("Trade: You"),
    mNetwork(network),
    mMyInventory(new Inventory()),
    mPartnerInventory(new Inventory())
{
    setContentSize(322, 150);

    mAddButton = new Button("Add", "add", this);
    mOkButton = new Button("Ok", "ok", this);
    mCancelButton = new Button("Cancel", "cancel", this);
    mTradeButton = new Button("Trade", "trade", this);

    mMyItemContainer = new ItemContainer(mMyInventory.get());
    mMyItemContainer->setPosition(2, 2);

    mMyScroll = new ScrollArea(mMyItemContainer);
    mMyScroll->setPosition(8, 8);

    mPartnerItemContainer = new ItemContainer(mPartnerInventory.get());
    mPartnerItemContainer->setPosition(2, 58);

    mPartnerScroll = new ScrollArea(mPartnerItemContainer);
    mPartnerScroll->setPosition(8, 64);

    mMoneyLabel = new gcn::Label("You get: 0z");
    mMoneyLabel2 = new gcn::Label("You give:");
    mMoneyField = new TextField();

    mAddButton->adjustSize();
    mOkButton->adjustSize();
    mCancelButton->adjustSize();
    mTradeButton->adjustSize();

    mTradeButton->setEnabled(false);

    mItemNameLabel = new gcn::Label("Name:");
    mItemDescriptionLabel = new gcn::Label("Description:");

    add(mMyScroll);
    add(mPartnerScroll);
    add(mAddButton);
    add(mOkButton);
    add(mCancelButton);
    add(mTradeButton);
    add(mItemNameLabel);
    add(mItemDescriptionLabel);
    add(mMoneyLabel2);
    add(mMoneyField);
    add(mMoneyLabel);

    mMoneyField->setPosition(8 + 60, getHeight() - 20);
    mMoneyField->setWidth(50);

    mMoneyLabel->setPosition(8 + 60 + 50 + 6, getHeight() - 20);
    mMoneyLabel2->setPosition(8, getHeight() - 20);

    mCancelButton->setPosition(getWidth() - 48, getHeight() - 49);
    mTradeButton->setPosition(mCancelButton->getX() - 40
        , getHeight() - 49);
    mOkButton->setPosition(mTradeButton->getX() - 24,
        getHeight() - 49);
    mAddButton->setPosition(mOkButton->getX() - 32,
        getHeight() - 49);

    mMyItemContainer->setSize(getWidth() - 24 - 12 - 1,
        (INVENTORY_SIZE * 24) / (getWidth() / 24) - 1);
    mMyScroll->setSize(getWidth() - 16, (getHeight() - 76) / 2);

    mPartnerItemContainer->setSize(getWidth() - 24 - 12 - 1,
        (INVENTORY_SIZE * 24) / (getWidth() / 24) - 1);
    mPartnerScroll->setSize(getWidth() - 16, (getHeight() - 76) / 2);

    mItemNameLabel->setPosition(8,
        mPartnerScroll->getY() + mPartnerScroll->getHeight() + 4);
    mItemDescriptionLabel->setPosition(8,
        mItemNameLabel->getY() + mItemNameLabel->getHeight() + 4);

    setContentSize(getWidth(), getHeight());
}

TradeWindow::~TradeWindow()
{
}

void TradeWindow::addMoney(int amount)
{
    mMoneyLabel->setCaption("You get: " + toString(amount) + "z");
    mMoneyLabel->adjustSize();
}

void TradeWindow::addItem(int id, bool own, int quantity, bool equipment)
{
    if (own) {
        mMyInventory->addItem(id, quantity, equipment);
    } else {
        mPartnerInventory->addItem(id, quantity, equipment);
    }
}

void TradeWindow::removeItem(int id, bool own)
{
    if (own) {
        mMyInventory->removeItem(id);
    } else {
        mPartnerInventory->removeItem(id);
    }
}

void TradeWindow::changeQuantity(int index, bool own, int quantity)
{
    if (own) {
        mMyInventory->getItem(index)->setQuantity(quantity);
    } else {
        mPartnerInventory->getItem(index)->setQuantity(quantity);
    }
}

void TradeWindow::increaseQuantity(int index, bool own, int quantity)
{
    if (own) {
        mMyInventory->getItem(index)->increaseQuantity(quantity);
    } else {
        mPartnerInventory->getItem(index)->increaseQuantity(quantity);
    }
}

void TradeWindow::reset()
{
    mMyInventory->clear();
    mPartnerInventory->clear();
    mTradeButton->setEnabled(false);
    mOkButton->setEnabled(true);
    mOkOther = false;
    mOkMe = false;
    mMoneyLabel->setCaption("You get: 0z");
    mMoneyField->setEnabled(true);
    mMoneyField->setText("");
}

void TradeWindow::setTradeButton(bool enabled)
{
    mTradeButton->setEnabled(enabled);
}

void TradeWindow::receivedOk(bool own)
{
    if (own) {
        mOkMe = true;
        if (mOkOther) {
            mTradeButton->setEnabled(true);
            mOkButton->setEnabled(false);
        } else {
            mTradeButton->setEnabled(false);
            mOkButton->setEnabled(false);
        }
    } else {
        mOkOther = true;
        if (mOkMe) {
            mTradeButton->setEnabled(true);
            mOkButton->setEnabled(false);
        } else {
            mTradeButton->setEnabled(false);
            mOkButton->setEnabled(true);
        }
    }
}

void TradeWindow::tradeItem(Item *item, int quantity)
{
    MessageOut outMsg(mNetwork);
    outMsg.writeInt16(CMSG_TRADE_ITEM_ADD_REQUEST);
    outMsg.writeInt16(item->getInvIndex());
    outMsg.writeInt32(quantity);
}

void TradeWindow::mouseClick(int x, int y, int button, int count)
{
    Window::mouseClick(x, y, button, count);

    Item *item;

    // mMyItems selected
    if (x >= mMyScroll->getX() + 3
        && x <= mMyScroll->getX() + mMyScroll->getWidth() - 10
        && y >= mMyScroll->getY() + 16
        && y <= mMyScroll->getY() + mMyScroll->getHeight() + 15
        && (item = mMyItemContainer->getItem()))
    {
            mPartnerItemContainer->selectNone();
    // mPartnerItems selected
    }
    else if (x >= mPartnerScroll->getX() + 3
        && x <= mPartnerScroll->getX() + mPartnerScroll->getWidth() - 20
        && y >= mPartnerScroll->getY() + 16
        && y <= mPartnerScroll->getY() + mPartnerScroll->getHeight() + 15
        && (item = mPartnerItemContainer->getItem()))
    {
            mMyItemContainer->selectNone();
    } else {
        return;
    }

    // Show Name and Description
    std::string SomeText;
    SomeText = "Name: " + item->getInfo()->getName();
    mItemNameLabel->setCaption(SomeText);
    mItemNameLabel->adjustSize();
    SomeText = "Description: " + item->getInfo()->getDescription();
    mItemDescriptionLabel->setCaption(SomeText);
    mItemDescriptionLabel->adjustSize();
}

void TradeWindow::action(const std::string &eventId)
{
    Item *item = inventoryWindow->getItem();

    if (eventId == "add") {
        if (!item) {
            return;
        }

        if (mMyInventory->getFreeSlot() < 1) {
            return;
        }

        if (mMyInventory->contains(item)) {
            chatWindow->chatLog("Failed adding item. You can not "
                    "overlap one kind of item on the window.", BY_SERVER);
            return;
        }

        if (item->getQuantity() == 1) {
            tradeItem(item, 1);
        }
        else {
            // Choose amount of items to trade
            new ItemAmountWindow(AMOUNT_TRADE_ADD, this, item);
        }
    }
    else if (eventId == "cancel")
    {
        MessageOut outMsg(mNetwork);
        outMsg.writeInt16(CMSG_TRADE_CANCEL_REQUEST);
    }
    else if (eventId == "ok")
    {
        std::stringstream tempMoney(mMoneyField->getText());
        int tempInt;
        if (tempMoney >> tempInt)
        {
            mMoneyField->setText(toString(tempInt));

            MessageOut outMsg(mNetwork);
            outMsg.writeInt16(CMSG_TRADE_ITEM_ADD_REQUEST);
            outMsg.writeInt16(0);
            outMsg.writeInt32(tempInt);
        } else {
            mMoneyField->setText("");
        }
        mMoneyField->setEnabled(false);
        MessageOut outMsg(mNetwork);
        outMsg.writeInt16(CMSG_TRADE_ADD_COMPLETE);
    }
    else if (eventId == "trade")
    {
        MessageOut outMsg(mNetwork);
        outMsg.writeInt16(CMSG_TRADE_OK);
    }
}
