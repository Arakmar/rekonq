/* ============================================================
*
* This file is a part of the rekonq project
*
* Copyright (C) 2011-2012 by Andrea Diamantini <adjam7 at gmail dot com>
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


#ifndef WEB_VIEW_H
#define WEB_VIEW_H


// Local Includes
#include "webpage.h"

// KDE Includes
#include <KWebView>

// Qt Includes
#include <QUrl>


class WebView : public KWebView
{
    Q_OBJECT

public:

    enum ContextType
    {
        EmptySelection  = 0x00000000,
        LinkSelection   = 0x00000001,
        ImageSelection  = 0x00000010,
        TextSelection   = 0x00000100
    };

    explicit WebView(QWidget *parent = 0);

    WebPage *page();

private Q_SLOTS:
    void menuRequested(const QPoint &);
    void openLinkInDefaultBrowser();

    void sendByMail();
    void findSelectedText();
    void search();

    void viewImage(Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers);
    void slotCopyImageLocation();

private:
    WebPage *m_page;
};

#endif // WEB_VIEW_H
