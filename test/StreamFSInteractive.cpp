/*
 * If not stated otherwise in this file or this component's LICENSE
 * file the following copyright and licenses apply:
 *
 * Copyright (c) 2022 Nuuday.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <iostream>
#include <zconf.h>
#include "StreamFSInteractive.h"
#include <menu.h>
#include <cstring>
#include <sys/stat.h>
#include <fcntl.h>
#include "YouseeChannels.h"
#include "utils/json.hpp"
#define KEY_TAB_ALTERNATIVE 9
#define KEY_ENTER_ALTERNATIVE 10
#define MAX_DRM_SIZE        9216

#define GET_PID(p)         (((p[1] << 8) + p[2]) & 0x1FFF)
#define STANDARD_CA_DESCRIPTOR_LENGTH 6


#define READ_AND_CHECK(fd, buffer, size, maxSize)        \
    if (maxSize < size)                                  \
    {                                                    \
        continue;                                        \
    }                                                    \
    res = read(fd, buffer, size);                        \
    if (res < 0) {                                       \
        if (errno == ENOTCONN ||                         \
            errno == EBADF ||                            \
            errno == ECONNRESET) {                       \
            break;                                       \
        } else {                                         \
            std::this_thread::sleep_for(std::chrono::seconds (2)); \
            continue;                                    \
        }                                                \
    }


static WINDOW *create_newwin(int height, int width, int starty, int startx);

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#define CTRLD 	4

WINDOW *create_newwin(int height, int width, int starty, int startx)
{	WINDOW *local_win;

    local_win = newwin(height, width, starty, startx);
    box(local_win, 0 , 0);

    wrefresh(local_win);

    return local_win;
}

StreamFSInteractive::StreamFSInteractive(char *streamfsFccPath)
        : mChannels(yousee::YouseeChannels::getInstance()),
          mFccPath(streamfsFccPath) {

    initscr();
    cbreak();
    noecho();
    clear();

    if (LINES < 10 || COLS < 10) {
        fprintf(stderr, "Terminal window too small. Exiting");
        exit(-1);
    }

    mWinSepX = COLS / 2;
    mWinSepY = 3 * LINES / 4;

    refresh();

    mLeftTopWin = create_newwin(mWinSepY, mWinSepX, 0, 0);
    mRightTopWin = create_newwin(LINES - 1, mWinSepX, 0, mWinSepX);

    scrollok(mRightTopWin, TRUE);
    scrollok(mLeftTopWin, TRUE);

    wrefresh(mLeftTopWin);
    wrefresh(mRightTopWin);

    keypad(mLeftTopWin, TRUE);
    mChannelSelectorPath = mFccPath + std::string("/chan_select0");

    mInfoThread = new std::thread(&StreamFSInteractive::drm0PollThread, this);
    mStatsThread = new std::thread(&StreamFSInteractive::statCollectorThread, this);
}

static void printMiddle(WINDOW *win, int startX, int startY, int width, char *string, chtype color);

StreamFSInteractive::~StreamFSInteractive() {
    refresh();
    delwin(mLeftTopWin);
    delwin(mRightTopWin);
    endwin();

    mRun = false;

    if (mInfoThread != nullptr)
        mInfoThread->join();

    if (mStatsThread != nullptr)
        mStatsThread->join();

    delete mInfoThread;
    delete mStatsThread;
}

static void printMiddle(WINDOW *win, int startX, int startY, int width, char *string, chtype color) {
    int length, x, y;
    float temp;

    if (win == nullptr)
        win = stdscr;

    getyx(win, y, x);
    if (startY != 0)
        x = startY;
    if (startX != 0)
        y = startX;
    if (width == 0)
        width = 80;

    length = strlen(string);
    temp = (width - length) / 2;
    x = startY + (int) temp;
    wattron(win, color);
    mvwprintw(win, y, x, "  %s  ", string);
    wattroff(win, color);
    refresh();
}

void StreamFSInteractive::showMenu() {
    ITEM **menuItems;
    int c;
    MENU *menuImpl;
    int n_choices, i;
    ITEM *currentItem= nullptr;
    ITEM *oldItem = nullptr;

    keypad(stdscr, TRUE);
    init_pair(1, COLOR_RED, COLOR_BLACK);

    auto& channels = mChannels.getChannels();

    n_choices = mChannels.getChannels().size();

    menuItems = (ITEM **)calloc(n_choices + 1, sizeof(ITEM *));

    i = 0;
    for (auto& it : channels) {
        menuItems[i] = new_item(it.second.name, it.second.uri);
        i++;
    }


    menuItems[n_choices] = (ITEM *)NULL;

    menuImpl = new_menu((ITEM **)menuItems);
    set_menu_win(menuImpl, mLeftTopWin);
    set_menu_sub(menuImpl, derwin(mLeftTopWin, mWinSepY - 3 , mWinSepX - 1 , 3, 1));
    /* Set menu mark to the string " * " */
    set_menu_mark(menuImpl, " * ");

    /* Print a border around the main window and print a title */
    box(mLeftTopWin, 0, 0);
    printMiddle(mLeftTopWin, 1, 0, mWinSepX - 1, "Channel selection", COLOR_PAIR(1));
    mvwaddch(mLeftTopWin, 2, 0, ACS_LTEE);
    mvwhline(mLeftTopWin, 2, 1, ACS_HLINE, mWinSepX);
    mvwaddch(mLeftTopWin, 2, mWinSepX - 1, ACS_RTEE);
    mvprintw(LINES - 2, 0, "Channel selection: [up][down] + [enter], [left], [right], [Tab] - previous # CTRL-C to exit");
    refresh();

    /* Post the menu */
    post_menu(menuImpl);
    wrefresh(mLeftTopWin);

    while((c = getch()) != KEY_F(1))
    {
        std::lock_guard<std::mutex> lock(mWindowMtx);

        switch(c) {
            case KEY_DOWN:
                menu_driver(menuImpl, REQ_DOWN_ITEM);
                break;
            case KEY_UP:
                menu_driver(menuImpl, REQ_UP_ITEM);
                break;
            case KEY_RIGHT:
                menu_driver(menuImpl, REQ_DOWN_ITEM);
                oldItem = currentItem;
                currentItem = current_item(menuImpl);
                mChannelSelector.select(mChannelSelectorPath.c_str(), item_description(currentItem));
                break;
            case KEY_LEFT:
                menu_driver(menuImpl, REQ_UP_ITEM);
                oldItem = currentItem;
                currentItem = current_item(menuImpl);
                mChannelSelector.select(mChannelSelectorPath.c_str(), item_description(currentItem));
                break;
            case KEY_STAB:
            case KEY_TAB_ALTERNATIVE:
                if (oldItem != nullptr) {
                    ITEM * tmp = currentItem;
                    currentItem = oldItem;
                    oldItem = tmp;
                    set_current_item(menuImpl, currentItem);
                    mChannelSelector.select(mChannelSelectorPath.c_str(), item_description(currentItem));
                }
                break;
                case KEY_ENTER_ALTERNATIVE:

            case KEY_ENTER:
                oldItem = currentItem;
                currentItem = current_item(menuImpl);
                //printMiddle(mLeftTopWin, 1, 0, mWinSepX - 1, (char*) item_description(cur_item), COLOR_PAIR(1));
                mChannelSelector.select(mChannelSelectorPath.c_str(), item_description(currentItem));
                break;
        }

        wrefresh(mLeftTopWin);
    }

    free_item(menuItems[0]);
    free_item(menuItems[1]);
    unpost_menu(menuImpl);
    free_menu(menuImpl);
}

void StreamFSInteractive::drm0PollThread() {
    fd_set  rfds;
    char bufCfg[MAX_DRM_SIZE];

    std::string cfgFileLocation = mFccPath + std::string("/drm0");

    int fd = open64(cfgFileLocation.c_str(), O_RDONLY);

    while (mRun) {
        fd_set  rfds;

        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);

        int res = select(fd+1, &rfds, nullptr, nullptr, nullptr);

        if (!FD_ISSET(fd, &rfds)) {
            break;
        }
        lseek(fd, 0, SEEK_SET);
        READ_AND_CHECK(fd, bufCfg, MAX_DRM_SIZE, MAX_DRM_SIZE);

        mSeekInfo = " ";
        updateDrm0State(bufCfg);

        updateRightWindow();
    }

}

void StreamFSInteractive::updateRightWindow() {
    std::lock_guard<std::mutex> lock(mWindowMtx);
    json::JSON obj = json::JSON::Load(mDrm);
    wclear(mRightTopWin);

    auto ecm = obj["ecm"].ToString();
    auto pmt = obj["pmt"].ToString();

    std::string out = "channel " + obj["channel"].ToString()
            + "\nclear:"  + std::to_string(obj["clear"].ToBool())
            + "\ndrm0 update count:" + std::to_string(mDrmCount)
            + "\nPrevious states [P]-protected [E]-error [C]-clear, [N]-no channel:\n"  + mDrm0UpdateList +
            + "\n\n" + mChanSwitchTimeMs +
            + "\nSeek:" + mSeekInfo +
            + "\nECM :" + obj["ecm"].ToString().substr()
            + "\n\nPMT :" + obj["pmt"].ToString()
            + "\n\nPAT :" + obj["pat"].ToString();

    mvwprintw(mRightTopWin, 1, 0, "%s", out.c_str());

    wrefresh(mRightTopWin);
    refresh();
}

void StreamFSInteractive::updateDrm0State(char *cfg) {
    mDrm = std::string(cfg);

    char errorState;
    json::JSON obj = json::JSON::Load(mDrm);
    auto ecm = obj["ecm"].ToString();
    auto pmt = obj["pmt"].ToString();
    bool isClear = obj["clear"].ToBool();
    auto pat = obj["pat"].ToString();

    if (ecm.empty() && pmt.empty() && pat.empty()) {
        errorState = 'N';
    } else if (isClear) {
        errorState = 'C';
    } else if (mDrmCount > 0  && (ecm == mOldEcm || pat == mOldPat || pmt == mOldPmt) ) {
        errorState = 'E';
    } else {
        errorState = 'P';
    }

    mOldEcm = ecm;
    mOldPat = pat;
    mOldPmt = pmt;

    if (mDrm0UpdateList.size() > DRM0_UPDATE_DISPLAY_COUNT) {
        mDrm0UpdateList = mDrm0UpdateList.substr(mDrm0UpdateList.size() - DRM0_UPDATE_DISPLAY_COUNT, mDrm0UpdateList.size() - 1);
    }
    mDrm0UpdateList += std::string("[") + errorState + std::string("]");
    mDrmCount++;

    // Measure time when we tune to a clear on encrypted channel
    if (errorState == 'P' || errorState == 'C') {
        auto currentTime = std::chrono::system_clock::now();
        auto timeDiff = currentTime - mChannelSelector.getLastChanSwitchTime();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(timeDiff).count();
        mChanSwitchTimeMs = std::string("Channel switch - time to drm0 update [ms]: ") + std::to_string(millis);
    }


}

void StreamFSInteractive::statCollectorThread() {
    const std::string seekPath = mFccPath + std::string("/seek0");
    static const int maxLen = 1000;

    char buf[maxLen];
    bool updated = false;
    while (mRun) {
        updated = false;
        std::this_thread::sleep_for (std::chrono::seconds(1));
        FILE* f = fopen64(seekPath.c_str(), "r");
        if (f) {
            auto ret = fread(buf, sizeof(char), maxLen, f);
            if (ret > 0) {
                updated = true;
                mSeekInfo = std::string(buf, ret);
            }
            fclose(f);
        }

        if (updated) {
            updateRightWindow();
        }
    }
}

static int dirExists(const char *path)
{
    struct stat info{};

    if(stat( path, &info ) != 0)
        return 0;
    else if(info.st_mode & S_IFDIR)
        return 1;
    else
        return 0;
}

int main(int argc, char* argv[]) {

    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <streamfs fcc mount point>" << std::endl;
        exit (-1);
    }

    if (!dirExists(argv[1])) {
        std::cerr << "Directory must exists:" << argv[1] << std::endl;
        exit(-1);

    }
    StreamFSInteractive si(argv[1]);
    si.showMenu();
    sleep(1);
    return 0;
}

void ChannelSelector::select(const char *fccPath, const char *channel) {
    std::lock_guard<std::mutex> lockGuard(mTimeMeasureMtx);

    FILE* f = fopen64(fccPath, "w");
    if (f != nullptr) {
        fwrite(channel, strlen(channel), 1, f);
        fclose(f);
    } else {
        fprintf(stderr, "Failed to open %s error = %d", fccPath, errno);
    }

    mChannelSwitchStartTime = std::chrono::high_resolution_clock::now();
}
