/* ============================================================
*
* This file is a part of the rekonq project
*
* Copyright (C) 2009-2012 by Andrea Diamantini <adjam7 at gmail dot com>
* Copyright (C) 2009 by Yoram Bar-Haim <<yoram.b at zend dot com>
* Copyright (C) 2009-2011 by Lionel Chauvin <megabigbug@yahoo.fr>
*
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; either version 2 of
* the License or (at your option) version 3 or any later version
* accepted by the membership of KDE e.V. (or its successor approved
* by the membership of KDE e.V.), which shall act as a proxy
* defined in Section 14 of version 3 of the license.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
* ============================================================ */


#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H


// Rekonq Includes
#include "rekonq_defines.h"

// Qt Includes
#include <QObject>
#include <QString>

// Forward Declarations
class TabHistory;
class MainWindow;

/**
  * Session Management: Needs clean up :)
  */
class REKONQ_TESTS_EXPORT SessionManager : public QObject
{
    Q_OBJECT
public:
    SessionManager(QObject *parent = 0);

    inline void setSessionManagementEnabled(bool on)
    {
        m_isSessionEnabled = on;
    }

    QList<TabHistory> closedSites();

    // This method restores session
    // while turning back from Private mode
    int restoreSavedSession();

    // This method restores a single MainWindow
    bool restoreMainWindow(MainWindow * window);

public Q_SLOTS:
    // This method restores session
    // on restart when restore at startup is chosen
    bool restoreSessionFromScratch();

private Q_SLOTS:
    void saveSession();

    // This method restores session
    // after a crash
    void restoreCrashedSession();

private:
    QString m_sessionFilePath;

    bool m_safe;
    bool m_isSessionEnabled;
};


#endif // SESSION_MANAGER_H
