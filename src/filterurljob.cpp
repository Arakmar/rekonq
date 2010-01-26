/* ============================================================
*
* This file is a part of the rekonq project
*
* Copyright (C) 2010 by Andrea Diamantini <adjam7 at gmail dot com>
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
#include "filterurljob.h"

// KDE Includes
#include <KUriFilter>
#include <KUriFilterData>

// Qt Includes
#include <QUrl>


FilterUrlJob::FilterUrlJob(WebView *view, const QString &urlString, QObject *parent)
    : Job(parent)
    , _view(view)
    , _urlString(urlString)
{
}


WebView *FilterUrlJob::view()
{
    return _view;
}


KUrl FilterUrlJob::url()
{
    return _url;
}


void FilterUrlJob::run()
{
    // this should let rekonq filtering URI info and supporting
    // the beautiful KDE web browsing shortcuts
    KUriFilterData data(_urlString);
    data.setCheckForExecutables(false); // if true, queries like "rekonq" or "dolphin" are considered as executables
    _url = KUriFilter::self()->filterUri(data) 
        ? data.uri().pathOrUrl() 
        : QUrl::fromUserInput( _urlString );
}