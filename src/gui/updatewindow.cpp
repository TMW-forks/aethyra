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

#include "updatewindow.h"
#include "ok_dialog.h"
#include "gui.h"
#include "../main.h"
#include "../log.h"
#include "../resources/resourcemanager.h"
#include <curl/curl.h>
#include <sstream>
#include <iostream>
#include <cstdio>
#include <SDL_thread.h>


UpdaterWindow::UpdaterWindow():
    Window("Updating...")
{
    mThread = NULL;
    mMutex = NULL;
    mDownloadStatus = UPDATE_NEWS;
    mUpdateHost = "";
    mCurrentFile = "news.txt";
    mDownloadComplete = true;
    mBasePath = "";
    mStoreInMemory = true;
    mDownloadedBytes = 0;
    mMemoryBuffer = NULL;
    mCurlError = new char[CURL_ERROR_SIZE];
    mCurlError[0] = 0;

    int h = 240;
    int w = 320;
    setContentSize(w, h);

    mBrowserBox = new BrowserBox();
    mBrowserBox->setOpaque(false);
    mCancelButton = new Button("Cancel");
    mCancelButton->setPosition(5, h - 5 - mCancelButton->getHeight());
    mCancelButton->setEventId("cancel");
    mCancelButton->addActionListener(this);
    mPlayButton = new Button("Play");
    mPlayButton->setPosition(mCancelButton->getX() +
                             mCancelButton->getWidth() + 5,
                             h - 5 - mPlayButton->getHeight());
    mPlayButton->setEventId("play");
    mPlayButton->setEnabled(false);
    mPlayButton->addActionListener(this);
    mProgressBar = new ProgressBar(0.0, 5, mCancelButton->getY() - 20 - 5,
                                   w - 10, 20, 37, 70, 23);
    mLabel = new gcn::Label("Connecting...");
    mLabel->setPosition(5, mProgressBar->getY() - mLabel->getHeight() - 5);
    mScrollArea = new ScrollArea(mBrowserBox);
    mScrollArea->setDimension(gcn::Rectangle(5, 5, 310,
                                             mLabel->getY() - 12));

    add(mScrollArea);
    add(mLabel);
    add(mProgressBar);
    add(mCancelButton);
    add(mPlayButton);

    mCancelButton->requestFocus();
    setLocationRelativeTo(getParent());
}

UpdaterWindow::~UpdaterWindow()
{
    delete mCurlError;
    delete mLabel;
    delete mProgressBar;
    delete mCancelButton;
    delete mPlayButton;
}

void UpdaterWindow::setProgress(float p)
{
    mProgressBar->setProgress(p);
}

void UpdaterWindow::setLabel(const std::string &str)
{
    mLabel->setCaption(str);
    mLabel->adjustSize();
}

void UpdaterWindow::enable()
{
    mPlayButton->setEnabled(true);
    mPlayButton->requestFocus();
}

void UpdaterWindow::action(const std::string& eventId)
{
    if (eventId == "cancel") {
        // Skip the updating process
        if (mDownloadStatus == UPDATE_COMPLETE)
        {
            state = EXIT;
        }
        else {
            mDownloadStatus = UPDATE_ERROR;
        }
    }
    else if (eventId == "play") {
        state = LOGIN;
    }
}

void UpdaterWindow::loadNews()
{
    int contentsLength = mDownloadedBytes;
    char *fileContents = mMemoryBuffer;

    if (!fileContents)
    {
        logger->log("Couldn't load news");
        return;
    }

    // Reallocate and include terminating 0 character
    fileContents = (char*)realloc(fileContents, contentsLength + 1);
    fileContents[contentsLength] = '\0';

    mBrowserBox->clearRows();

    // Tokenize and add each line separately
    char *line = strtok(fileContents, "\n");
    while (line != NULL)
    {
        mBrowserBox->addRow(line);
        line = strtok(NULL, "\n");
    }

    //free(fileContents);

    mScrollArea->setVerticalScrollAmount(0);
    setVisible(true);
}

void UpdaterWindow::addRow(const std::string &row)
{
    mBrowserBox->addRow(row);
    mScrollArea->setVerticalScrollAmount(mScrollArea->getVerticalMaxScroll());
}

int UpdaterWindow::updateProgress(void *ptr, double dt, double dn, double ut, double un)
{
    float progress = dn/dt;
    UpdaterWindow *uw = reinterpret_cast<UpdaterWindow *>(ptr);

    if (progress < 0)
    {
        progress = 0.0f;
    }
    std::stringstream progressString;
    progressString << uw->mCurrentFile << " (" << ((int)(progress*100)) << "%)";
    uw->setLabel(progressString.str().c_str());
    uw->setProgress(progress);

    if (state != UPDATE || uw->mDownloadStatus == UPDATE_ERROR) {
        // If the action was canceled return an error code to stop the mThread
        return -1;
    }

    return 0;
}

size_t UpdaterWindow::memoryWrite(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    UpdaterWindow *uw = reinterpret_cast<UpdaterWindow *>(stream);
    uw->mMemoryBuffer = (char *)realloc(uw->mMemoryBuffer, uw->mDownloadedBytes + nmemb * size + 1);
    if (uw->mMemoryBuffer)
    {
        memcpy(&(uw->mMemoryBuffer[uw->mDownloadedBytes]), ptr, nmemb * size);
        uw->mDownloadedBytes += nmemb;
        uw->mMemoryBuffer[uw->mDownloadedBytes] = 0;
    }
    return nmemb;
}

int UpdaterWindow::downloadThread(void *ptr)
{
    CURL *curl;
    CURLcode res;
    FILE *outfile = NULL;
    UpdaterWindow *uw = reinterpret_cast<UpdaterWindow *>(ptr);
    std::string outFilename;
    std::string url(uw->mUpdateHost + "/" + uw->mCurrentFile);

    curl = curl_easy_init();

    if (curl)
    {
        logger->log("Downloading: %s", url.c_str());

        if (uw->mStoreInMemory)
        {
            uw->mDownloadedBytes = 0;
            curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                                   UpdaterWindow::memoryWrite);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, ptr);
        }
        else
        {
            // Download in the proper folder : ./data under win,
            // /home/user/.tmw/data for unices
            outFilename =  uw->mBasePath + "/data/download.temp";
            outfile = fopen(outFilename.c_str(), "wb");
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, outfile);
        }

        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, uw->mCurlError);
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
        curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION,
                               UpdaterWindow::updateProgress);
        curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, ptr);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15);

        if ((res = curl_easy_perform(curl)) != 0)
        {
            uw->mDownloadStatus = UPDATE_ERROR;
            std::cerr << "curl error " << res << " : " << uw->mCurlError
                << std::endl;
        }

        curl_easy_cleanup(curl);
        uw->mDownloadComplete = true;

        if (!uw->mStoreInMemory)
        {
            fclose(outfile);
            // If the download was successful give the file the proper name
            // else it will be deleted later
            std::string newName(uw->mBasePath + "/data/" +
                                uw->mCurrentFile.c_str());
            rename(outFilename.c_str(), newName.c_str());
        }
    }

    return 0;
}

void UpdaterWindow::download()
{
    mDownloadComplete = false;
    mThread = SDL_CreateThread(UpdaterWindow::downloadThread, this);

    if (mThread == NULL) {
        logger->log("Unable to create mThread");
        mDownloadStatus = UPDATE_ERROR;
    }
}

void UpdaterWindow::updateData()
{
    std::ifstream in;
    std::vector<std::string> files;

    state = UPDATE;
    unsigned int fileIndex = 0;

    mUpdateHost = config.getValue("updatehost", "themanaworld.org/files");
    mBasePath = config.getValue("homeDir", ".");

    // Try to download the updates list
    download();

    while (state == UPDATE)
    {
        // Handle SDL events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    state = EXIT;
                    break;

                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_ESCAPE)
                    {
                        state = EXIT;
                    }
                    break;
            }

            guiInput->pushInput(event);
        }

        switch (mDownloadStatus) {
            case UPDATE_ERROR:
                if (mThread)
                {
                    SDL_WaitThread(mThread, NULL);
                    mThread = NULL;
                }
                addRow("");
                addRow("##1  The update process is incomplete.");
                addRow("##1  It is strongly recommended that");
                addRow("##1  you try again later");
                addRow(mCurlError);
                mDownloadStatus = UPDATE_COMPLETE;
                break;
            case UPDATE_NEWS:
                if (mDownloadComplete) {
                    // Try to open news.txt
                    loadNews();
                    // Doesn't matter if it couldn't find news.txt,
                    // go to the next step
                    mCurrentFile = "resources.txt";
                    if (mMemoryBuffer != NULL)
                    {
                        free(mMemoryBuffer);
                        mMemoryBuffer = NULL;
                    }
                    download();
                    mDownloadStatus = UPDATE_LIST;
                }
                break;
            case UPDATE_LIST:
                if (mDownloadComplete) {
                    if (mMemoryBuffer != NULL)
                    {
                        // Tokenize and add each line separately
                        char *line = strtok(mMemoryBuffer, "\n");
                        while (line != NULL)
                        {
                            files.push_back(line);
                            line = strtok(NULL, "\n");
                        }
                        mStoreInMemory = false;
                        mDownloadStatus = UPDATE_RESOURCES;
                    }
                    else {
                        logger->log("Unable to download resources.txt");
                        mDownloadStatus = UPDATE_ERROR;
                    }
                }
                break;
            case UPDATE_RESOURCES:
                if (mDownloadComplete)
                {
                    if (mThread)
                    {
                        SDL_WaitThread(mThread, NULL);
                        mThread = NULL;
                    }

                    if (fileIndex < files.size())
                    {
                        mCurrentFile = files[fileIndex];
                        std::ifstream temp(
                                (mBasePath + "/data/" + mCurrentFile).c_str());
                        if (!temp.is_open()) {
                            temp.close();
                            download();
                        }
                        else {
                            logger->log("%s already here", mCurrentFile.c_str());
                        }
                        fileIndex++;
                    }
                    else {
                        // Download of updates completed
                        mDownloadStatus = UPDATE_COMPLETE;
                    }
                }
                break;
            case UPDATE_COMPLETE:
                enable();
                setLabel("Completed");
                break;
            case UPDATE_IDLE:
                break;
        }

        gui->logic();

        guiGraphics->drawImage(login_wallpaper, 0, 0);
        gui->draw();
        guiGraphics->updateScreen();
    }

    if (mThread)
    {
         SDL_WaitThread(mThread, NULL);
         mThread = NULL;
    }

    free(mMemoryBuffer);
    in.close();
    // Remove downloaded files
    remove((mBasePath + "/data/news.txt").c_str());
    remove((mBasePath + "/data/resources.txt").c_str());
    remove((mBasePath + "/data/download.temp").c_str());
}