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

#ifndef _TMW_STATS_H
#define _TMW_STATS_H

#include "window.h"
#include "progressbar.h"

/**
 * The player status dialog.
 *
 * \ingroup GUI
 */
class StatusWindow : public Window {
    public:
        /**
         * Constructor.
         */
        StatusWindow();

        /**
         * Destructor.
         */
        ~StatusWindow();

        /**
         * Updates this dialog with values from PLAYER_INFO *char_info
         */
        void update();

    private:
        gcn::Label *hp, *sp, *gp;
        gcn::Label *hpValue, *spValue;
        gcn::Label *expLabel, *jobExpLabel;
        ProgressBar *healthBar, *manaBar;
        ProgressBar *xpBar, *jobXpBar;
	gcn::Button *statsButton, *skillsButton, *inventoryButton;
};

#endif
