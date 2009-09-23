/* ============================================================
*
* This file is a part of the rekonq project
*
* Copyright (C) 2008-2009 by Andrea Diamantini <adjam7 at gmail dot com>
* Copyright (C) 2009 by Lionel Chauvin <megabigbug@yahoo.fr>
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


#ifndef WEBVIEW_H
#define WEBVIEW_H


// KDE Includes
#include <KUrl>

// Qt Includes
#include <QWebView>

// Forward Declarations
class WebPage;


class WebView : public QWebView
{
    Q_OBJECT

public:
    explicit WebView(QWidget *parent = 0);

    WebPage *page();
    KUrl url() const;
    QString lastStatusBarText() const;
    int progress();

signals:
    // switching tabs
    void ctrlTabPressed();
    void shiftCtrlTabPressed();

protected:
    void contextMenuEvent(QContextMenuEvent *event);
    void mousePressEvent(QMouseEvent *event);
    /**
    * Filters (SHIFT + ) CTRL + TAB events and emit (shift)ctrlTabPressed()
    * to make switch tab
    */
    void keyPressEvent(QKeyEvent *event);
    void wheelEvent(QWheelEvent *event);

private slots:
    void setStatusBarText(const QString &string);
    void slotSearch();
    void slotUpdateProgress(int progress);
    void slotLoadFinished(bool);

    void printFrame();

    void openLinkInNewWindow();
    void openLinkInNewTab();

private:
    WebPage *m_page;
    int m_progress;
    QString m_statusBarText;
};

#endif
