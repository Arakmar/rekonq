/* ============================================================
*
* This file is a part of the rekonq project
*
* Copyright (C) 2009 by Andrea Diamantini <adjam7 at gmail dot com>
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


// Self Includes
#include "homepage.h"
#include "homepage.moc"

// Auto Includes
#include "rekonq.h"

// Local Includes
#include "historymodels.h"
#include "bookmarks.h"
#include "application.h"
#include "mainwindow.h"
#include "mainview.h"

// KDE Includes
#include <KStandardDirs>
#include <KIconLoader>
#include <KDebug>
#include <KConfig>
#include <KConfigGroup>

// Qt Includes
#include <QFile>


HomePage::HomePage(QObject *parent)
    : QObject(parent)
{
    m_homePagePath = KStandardDirs::locate("data", "rekonq/htmls/home.html");
}


HomePage::~HomePage()
{
}


QString HomePage::rekonqHomePage(const KUrl &url)
{
    QFile file(m_homePagePath);
    bool isOpened = file.open(QIODevice::ReadOnly);
    if (!isOpened)
    {
        kWarning() << "Couldn't open the home.html file";
        return QString("");
    }

    QString imagesPath = QString("file://") + KGlobal::dirs()->findResourceDir("data", "rekonq/pics/bg.png") + QString("rekonq/pics");
    QString menu = homePageMenu();
    
    QString speed;
    if(url == KUrl("about:lastSites"))
        speed = fillRecentHistory();
    if(url == KUrl("about:history"))
        speed = history();
    if(url == KUrl("about:bookmarks"))
        speed = bookmarks();
    if(url == KUrl("about:home") || url == KUrl("about:preferred"))
        speed = speedDial();
    
    QString html = QString(QLatin1String(file.readAll()))
                        .arg(imagesPath)
                        .arg(menu)
                        .arg(speed)
                        ;
    return html;
}


QString HomePage::speedDial()
{
    QStringList names = ReKonfig::previewNames();
    QStringList urls = ReKonfig::previewUrls();

    QString speed = "<table><tr>";
    for(int i=0; i<4; ++i)
    {
        speed += "<td><div class=\"thumbnail\">";
        speed += "<object type=\"application/image-preview\" width=\"200\">";
        speed += "<param name=\"url\" value=\"" + urls.at(i) + "\">";
        speed += "<param name=\"position\" value=\"" + QString::number(i) + "\">"; 
        speed += "</object>";
        speed += "<br /><br />";
        speed += "<a href=\"" + urls.at(i) + "\">" + names.at(i) + "</a></div></td>";
    }
    speed += "</tr><tr>";
    for(int i=4; i<8; ++i)
    {
        speed += "<td><div class=\"thumbnail\">";
        speed += "<object type=\"application/image-preview\" width=\"200\">";
        speed += "<param name=\"url\" value=\"" + urls.at(i) + "\">";
        speed += "<param name=\"position\" value=\"" + QString::number(i) + "\">"; 
        speed += "</object>";
        speed += "<br /><br />";
        speed += "<a href=\"" + urls.at(i) + "\">" + names.at(i) + "</a></div></td>";
    }
    speed += "</tr></table>";
    
    return speed;
}


QString HomePage::recentlyClosedTabs()
{
    QString closed = "<h2>" + i18n("Recently closed tabs") + "</h2>";
    closed += "<ul>";
    KUrl::List links = Application::instance()->mainWindow()->mainView()->recentlyClosedTabs();
    
    Q_FOREACH(const KUrl &url, links)
    {
        closed += "<li><a href=\"" + url.prettyUrl() + "\">" + url.prettyUrl() + "</a></li>";
    }
    closed += "</ul>";
    return closed;
}


QString HomePage::fillRecentHistory()
{
    QString history = "<h2>" + i18n("Last 20 visited sites") + "</h2>";
    history += "<ul>";
    HistoryTreeModel *model = Application::historyManager()->historyTreeModel();
    int i = 0;
    do
    {
        QModelIndex index = model->index(i, 0, QModelIndex() );
        if(model->hasChildren(index))
        {
            for(int j=0; j< model->rowCount(index) && i<20 ; ++j)
            {
                QModelIndex son = model->index(j, 0, index );

                history += "<li>";
                history += QString("<a href=\"") + son.data(HistoryModel::UrlStringRole).toString() + QString("\">");
                history += son.data().toString();
                history += QString("</a>");
                history += "</li>";
                
                i++;
            }
        }
        i++;
    }
    while( i<20 || model->hasIndex( i , 0 , QModelIndex() ) );

    history += "<ul>";
    return history;
}


QString HomePage::homePageMenu()
{
    QString menu = "<ul>";
    menu += "<li><a href=\"about:lastSites\">Last Visited Sites</a></li>";
    menu += "<li><a href=\"about:history\">History</a></li>";
    menu += "<li><a href=\"about:bookmarks\">Bookmarks</a></li>";
    menu += "<li><a href=\"about:preferred\">Preferred</a></li>";
    menu += "</ul>";
    return menu;
}


QString HomePage::history()
{
    QString history = "<h2>" + i18n("History") + "</h2>";
    history += "<ul>";
    HistoryTreeModel *model = Application::historyManager()->historyTreeModel();
    int i = 0;
    do
    {
        QModelIndex index = model->index(i, 0, QModelIndex() );
        if(model->hasChildren(index))
        {
            for(int j=0; j< model->rowCount(index) && i<20 ; ++j)
            {
                QModelIndex son = model->index(j, 0, index );

                history += "<li>";
                history += QString("<a href=\"") + son.data(HistoryModel::UrlStringRole).toString() + QString("\">");
                history += son.data().toString();
                history += QString("</a>");
                history += "</li>";
                
                i++;
            }
        }
        i++;
    }
    while( i<20 || model->hasIndex( i , 0 , QModelIndex() ) );

    history += "<ul>";
    return history;
}


QString HomePage::bookmarks()
{
    return QString("");
}

