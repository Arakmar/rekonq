/* ============================================================
*
* This file is a part of the rekonq project
*
* Copyright (C) 2008 by Andrea Diamantini <adjam7 at gmail dot com>
* Copyright (C) 2009 by Paweł Prażak <pawelprazak at gmail dot com>
*
*
* This program is free software; you can redistribute it
* and/or modify it under the terms of the GNU General
* Public License as published by the Free Software Foundation;
* either version 3, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* ============================================================ */



#ifndef APPLICATION_H
#define APPLICATION_H

// KDE Includes
#include <KUniqueApplication>
#include <KIcon>
#include <KUrl>
#include <kio/job.h>
#include <kio/jobclasses.h>

// Qt Includes
#include <QPointer>

// Forward Declarations
class KIcon;
class KUrl;
class BookmarkProvider;
class CookieJar;
class HistoryManager;
class MainWindow;
class NetworkAccessManager;


namespace Rekonq
{
    /**
     * @short notifying message status
     * Different message status
     */

    enum Notify
    {
        Success,    ///< url successfully (down)loaded
        Error,      ///< url failed to (down)load
        Download,   ///< downloading url
        Info        ///< information, (default)
    };

    /**
     * @short Open link options
     * Different modes of opening new tab
     */
    enum OpenType
    {
        CurrentTab,     ///< open url in current tab
        SettingOpenTab, ///< open url according to users settings
        NewCurrentTab,  ///< open url in new tab and make it current
        NewBackTab      ///< open url in new tab in background
    };

}


/**
  *
  */
class Application : public KUniqueApplication
{
    Q_OBJECT

public:
    Application();
    ~Application();
    int newInstance();
    static Application *instance();

    MainWindow *mainWindow();

    static KIcon icon(const KUrl &url);

    static HistoryManager *historyManager();
    static CookieJar *cookieJar();
    static NetworkAccessManager *networkAccessManager();
    static BookmarkProvider *bookmarkProvider();

public slots:
    /**
     * Save application's configuration
     * @see ReKonfig::self()->writeConfig();
     */
    void slotSaveConfiguration() const;

public slots:

    void loadUrl( const KUrl& url,
                  const Rekonq::OpenType& type = Rekonq::CurrentTab
                );
           
    void loadUrl( const QString& urlString,
                  const Rekonq::OpenType& type = Rekonq::CurrentTab
                );    

                
private slots:

    /**
     * Any actions that can be delayed until the window is visible
     */
    void postLaunch();


private:

    KUrl guessUrlFromString(const QString &url);

    static QPointer<HistoryManager> s_historyManager;
    static QPointer<NetworkAccessManager> s_networkAccessManager;
    static QPointer<BookmarkProvider> s_bookmarkProvider;

    QPointer<MainWindow> m_mainWindow;
};

#endif // APPLICATION_H
