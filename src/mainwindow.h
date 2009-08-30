/* ============================================================
*
* This file is a part of the rekonq project
*
* Copyright (C) 2008-2009 by Andrea Diamantini <adjam7 at gmail dot com>
* Copyright (C) 2009 by Paweł Prażak <pawelprazak at gmail dot com>
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


#ifndef MAINWINDOW_H
#define MAINWINDOW_H


// Local Includes
#include "bookmarks.h"
#include "mainview.h"
#include "webview.h"

// KDE Includes
#include <KMainWindow>
#include <KActionCollection>
#include <KToolBar>

// Forward Declarations
class QWebFrame;

class KAction;
class KPassivePopup;

class HistoryMenu;
class FindBar;
class SidePanel;
class WebView;


/**
 * This class serves as the main window for rekonq.
 * It handles the menus, toolbars, and status bars.
 *
 */
class MainWindow : public KMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    ~MainWindow();

    MainView *mainView() const;
    WebView *currentTab() const;
    QAction *actionByName(const QString name);
    virtual QSize sizeHint() const;
    virtual KActionCollection *actionCollection () const;
    
private:
    void setupActions();
    void setupTools();
    void setupToolbar();

    void setupSidePanel();
    SidePanel *sidePanel();

public slots:
    void slotHome();
    void slotUpdateBrowser();

    /**
     * Notifies a message in a popup
     *
     * @param msg The message to notify
     *
     * @param status The status message
     *
     */
    void notifyMessage(const QString &msg, Rekonq::Notify status = Rekonq::Info);


protected:
    bool queryClose();
    void keyPressEvent(QKeyEvent *event);

private slots:
    void postLaunch();
    void slotUpdateConfiguration();
    void slotBrowserLoading(bool);
    void slotUpdateActions();
    void slotUpdateWindowTitle(const QString &title = QString());
    void slotOpenLocation();
    void geometryChangeRequested(const QRect &geometry);

    // history related
    void slotOpenPrevious();
    void slotOpenNext();

    // File Menu slots
    void slotFileOpen();
    void slotFilePrintPreview();
    void slotFileSaveAs();
    void printRequested(QWebFrame *frame);

    // Edit Menu slots
    void slotFind(const QString &);
    void slotFindNext();
    void slotFindPrevious();

    // View Menu slots
    void slotViewTextBigger();
    void slotViewTextNormal();
    void slotViewTextSmaller();
    void slotViewPageSource();
    void slotViewFullScreen(bool enable);

    // Tools Menu slots
    void slotToggleInspector(bool enable);
    void slotPrivateBrowsing(bool enable);

    // Settings Menu slots
    void slotShowMenubar(bool enable);
    void slotPreferences();

    // clear private data
    void clearPrivateData();

    void slotAboutToShowBackMenu();
    void slotOpenActionUrl(QAction *action);

private:
    MainView *m_view;
    FindBar *m_findBar;
    SidePanel *m_sidePanel;

    KAction *m_stopReloadAction;
    KAction *m_historyBackAction;
    KAction *m_historyForwardAction;
    KMenu *m_historyBackMenu;

    KToolBar *m_bmBar;
    KToolBar *m_mainBar;

    QString m_lastSearch;
    QString m_homePage;

    QPointer<KPassivePopup> m_popup;

    KActionCollection *m_ac;
};

#endif // MAINWINDOW_H

