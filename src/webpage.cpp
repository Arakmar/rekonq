/* ============================================================
*
* This file is a part of the rekonq project
*
* Copyright (C) 2008 Benjamin C. Meyer <ben@meyerhome.net>
* Copyright (C) 2008 Dirk Mueller <mueller@kde.org>
* Copyright (C) 2008 Urs Wolfer <uwolfer @ kde.org>
* Copyright (C) 2008 Michael Howell <mhowell123@gmail.com>
* Copyright (C) 2008-2010 by Andrea Diamantini <adjam7 at gmail dot com>
* Copyright (C) 2010 by Matthieu Gicquel <matgic78 at gmail dot com>
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
#include "webpage.h"
#include "webpage.moc"

// Auto Includes
#include "rekonq.h"

// Local Includes
#include "application.h"
#include "mainwindow.h"
#include "mainview.h"
#include "webtab.h"
#include "webpluginfactory.h"
#include "networkaccessmanager.h"
#include "adblockmanager.h"
#include "urlbar.h"

#include "sslinfodialog_p.h"

// KDE Includes
#include <KStandardDirs>
#include <KUrl>
#include <KToolInvocation>
#include <KProtocolManager>
#include <kwebwallet.h>

#include <kparts/browseropenorsavequestion.h>

#include <kio/renamedialog.h>

#include <KDE/KMimeTypeTrader>
#include <KDE/KRun>
#include <KDE/KFileDialog>
#include <KDE/KMessageBox>
#include <KDE/KJobUiDelegate>

// Qt Includes
#include <QtGui/QContextMenuEvent>
#include <QtGui/QWheelEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QClipboard>
#include <QtGui/QKeyEvent>

#include <QtWebKit/QWebFrame>


// Returns true if the scheme and domain of the two urls match...
static bool domainSchemeMatch(const QUrl& u1, const QUrl& u2)
{
    if (u1.scheme() != u2.scheme())
        return false;

    QStringList u1List = u1.host().split(QL1C('.'), QString::SkipEmptyParts);
    QStringList u2List = u2.host().split(QL1C('.'), QString::SkipEmptyParts);

    if (qMin(u1List.count(), u2List.count()) < 2)
        return false;  // better safe than sorry...

    while (u1List.count() > 2)
        u1List.removeFirst();

    while (u2List.count() > 2)
        u2List.removeFirst();

    return (u1List == u2List);
}


// ---------------------------------------------------------------------------------


WebPage::WebPage(QWidget *parent)
        : KWebPage(parent, KWalletIntegration)
        , _networkAnalyzer(false)
        , _isOnRekonqPage(false)
{
    // ----- handling unsupported content...
    setForwardUnsupportedContent(true);
    connect(this, SIGNAL(unsupportedContent(QNetworkReply *)), this, SLOT(handleUnsupportedContent(QNetworkReply *)));

    // ----- rekonq Network Manager
    NetworkAccessManager *manager = new NetworkAccessManager(this);
    manager->setCache(0);   // disable QtWebKit cache to just use KIO one..

    // set cookieJar window ID..
    if (parent && parent->window())
        manager->setCookieJarWindowId(parent->window()->winId());

    setNetworkAccessManager(manager);

    // activate ssl warnings
    setSessionMetaData("ssl_activate_warnings", "TRUE");

    // Override the 'Accept' header sent by QtWebKit which favors XML over HTML!
    // Setting the accept meta-data to null will force kio_http to use its own
    // default settings for this header.
    setSessionMetaData(QL1S("accept"), QString());

    // ----- Web Plugin Factory
    setPluginFactory(new WebPluginFactory(this));

    // ----- last stuffs
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(manageNetworkErrors(QNetworkReply*)));
    connect(this, SIGNAL(loadFinished(bool)), this, SLOT(loadFinished(bool)));

    // protocol handler signals
    connect(&_protHandler, SIGNAL(downloadUrl(const KUrl &)), this, SLOT(downloadUrl(const KUrl &)));
}


WebPage::~WebPage()
{
    disconnect();
}


bool WebPage::acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request, NavigationType type)
{
    _isOnRekonqPage = false;
    _loadingUrl = request.url();

    KIO::AccessManager *manager = qobject_cast<KIO::AccessManager*>(networkAccessManager());
    KIO::MetaData metaData = manager->requestMetaData();

    // Get the SSL information sent, if any...
    if (metaData.contains(QL1S("ssl_in_use")))
    {
        WebSslInfo info;
        info.fromMetaData(metaData.toVariant());
        info.setUrl(request.url());
        _sslInfo = info;
    }

    if (frame)
    {
        if (_protHandler.preHandling(request, frame))
        {
            return false;
        }

        switch (type)
        {
        case QWebPage::NavigationTypeLinkClicked:
            if (_sslInfo.isValid())
            {
                setRequestMetaData("ssl_was_in_use", "TRUE");
            }
            break;

        case QWebPage::NavigationTypeFormSubmitted:
            break;

        case QWebPage::NavigationTypeFormResubmitted:
            if (KMessageBox::warningContinueCancel(view(),
                                                   i18n("Are you sure you want to send your data again?"),
                                                   i18n("Resend form data")
                                                  )
                    == KMessageBox::Cancel)
            {
                return false;
            }
            break;

        case QWebPage::NavigationTypeReload:
        case QWebPage::NavigationTypeBackOrForward:
        case QWebPage::NavigationTypeOther:
            break;

        default:
            break;
        }

        if (frame == mainFrame())
        {
            setRequestMetaData("main_frame_request", "TRUE");
        }
        else
        {
            setRequestMetaData("main_frame_request", "FALSE");
        }
    }
    return KWebPage::acceptNavigationRequest(frame, request, type);
}


WebPage *WebPage::createWindow(QWebPage::WebWindowType type)
{
    // added to manage web modal dialogs
    if (type == QWebPage::WebModalDialog)
        kDebug() << "Modal Dialog";

    WebTab *w = 0;
    if (ReKonfig::openTabNoWindow())
    {
        w = Application::instance()->mainWindow()->mainView()->newWebTab(!ReKonfig::openTabsBack(), ReKonfig::openTabsNearCurrent());
    }
    else
    {
        w = Application::instance()->newMainWindow()->mainView()->currentWebTab();
    }
    return w->page();
}


void WebPage::handleUnsupportedContent(QNetworkReply *reply)
{
    Q_ASSERT (reply);
    // NOTE: 
    // Until kio implements a way to resume/continue a network
    // request. We must abort the reply to prevent a zombie process
    // from continuing to download the unsupported content!
    reply->abort();

    // This is probably needed just in ONE stupid case..
    if (_protHandler.postHandling(reply->request(), mainFrame()))
        return;

    if (reply->error() == QNetworkReply::NoError)
    {
        KUrl replyUrl = reply->url();

        // HACK -------------------------------------------
        QString mimeType;
        QString suggestedFileName;
        
        QString app = reply->header(QNetworkRequest::ContentTypeHeader).toString();
        QStringList headerList = app.split( ';' );
        
        if(headerList.count() > 0)
        {
            mimeType = headerList.takeFirst().trimmed();
            Q_FOREACH(const QString &head, headerList)
            {
                if( head.contains( QL1S("name") ) )
                {
                    // this is not so sure.. :)
                    suggestedFileName = head;
                    suggestedFileName = suggestedFileName.remove( QL1S("name=") );
                    suggestedFileName = suggestedFileName.remove( '"' );
                    suggestedFileName = suggestedFileName.trimmed();
                    break;
                }
            }
        }
        else
        {
            mimeType = reply->header(QNetworkRequest::ContentTypeHeader).toString();
        }
        // ------------------------------------------------
        
        KService::Ptr appService = KMimeTypeTrader::self()->preferredService(mimeType);

        bool isLocal = replyUrl.isLocalFile();

        if (appService.isNull())  // no service can handle this. We can just download it..
        {
            kDebug() << "no service can handle this. We can just download it..";

            isLocal
            ? KMessageBox::sorry(view(), i18n("No service can handle this :("))
            : downloadThings(reply->request(), suggestedFileName);

            return;
        }

        if (!isLocal)
        {

            KParts::BrowserOpenOrSaveQuestion dlg(Application::instance()->mainWindow(), replyUrl, mimeType);
            if(!suggestedFileName.isEmpty())
                dlg.setSuggestedFileName(suggestedFileName);
            
            switch (dlg.askEmbedOrSave())
            {
            case KParts::BrowserOpenOrSaveQuestion::Save:
                kDebug() << "user choice: no services, just download!";
                downloadThings(reply->request(), suggestedFileName);
                return;

            case KParts::BrowserOpenOrSaveQuestion::Cancel:
                return;

            default: // non extant case
                break;
            }
        }

        // case KParts::BrowserRun::Embed
        KService::List partServices = KMimeTypeTrader::self()->query(mimeType, QL1S("KParts/ReadOnlyPart"));
        if (partServices.count() > 0)
        {
            QString p = replyUrl.pathOrUrl();
            
            // A part can handle this. Embed it!
            QString html;
            html += "<html>";
            html += "<head>";
            html += "<title>";
            html += p;
            html += "</title>";
            html += "<style type=\"text/css\">";
            html += "* { border: 0; padding: 0; margin: 0; }";
            html += "</style>";
            html += "</head>";
            html += "<body>";
            html += "<object type=\"" + mimeType + "\" data=\"" + p + "\" width=\"100%\" height=\"100%\" />";
            html += "</body>";
            html += "</html>";

            mainFrame()->setHtml(html);            
            _isOnRekonqPage = true;
            Application::instance()->mainWindow()->mainView()->urlBar()->setQUrl(replyUrl);
            Application::instance()->mainWindow()->updateActions();
        }
        else
        {
            // No parts, just app services. Load it!
            KRun::run(*appService, replyUrl, 0);
        }

        return;
    }
}


void WebPage::loadFinished(bool ok)
{
    Q_UNUSED(ok);
    
    Application::adblockManager()->applyHidingRules(this);

    QStringList list = ReKonfig::walletBlackList();

    // KWallet Integration
    if (wallet()
            && !list.contains(mainFrame()->url().toString())
       )
    {
        wallet()->fillFormData(mainFrame());
    }
}


void WebPage::manageNetworkErrors(QNetworkReply *reply)
{
    Q_ASSERT(reply);

    QWebFrame* frame = qobject_cast<QWebFrame *>(reply->request().originatingObject());
    const bool isMainFrameRequest = (frame == mainFrame());

    if (isMainFrameRequest
            && _sslInfo.isValid()
            && !domainSchemeMatch(reply->url(), _sslInfo.url())
       )
    {
        // Reseting cached SSL info...
        _sslInfo = WebSslInfo();
    }

    // NOTE: These are not all networkreply errors,
    // but just that supported directly by KIO
    switch (reply->error())
    {

    case QNetworkReply::NoError:                             // no error. Simple :)
        if (isMainFrameRequest && !_sslInfo.isValid())
        {
            // Obtain and set the SSL information if any...
            _sslInfo.fromMetaData(reply->attribute(static_cast<QNetworkRequest::Attribute>(KIO::AccessManager::MetaData)));
            _sslInfo.setUrl(reply->url());
        }
        break;

    case QNetworkReply::ContentAccessDenied:                 // access to remote content denied (similar to HTTP error 401)
        kDebug() << "We (hopefully) are managing this through the adblock :)";
        break;

    case QNetworkReply::UnknownNetworkError:                 // unknown network-related error detected
        if (_protHandler.postHandling(reply->request(), mainFrame()))
            break;

    case QNetworkReply::ConnectionRefusedError:              // remote server refused connection
    case QNetworkReply::HostNotFoundError:                   // invalid hostname
    case QNetworkReply::TimeoutError:                        // connection time out
    case QNetworkReply::OperationCanceledError:              // operation canceled via abort() or close() calls
    case QNetworkReply::ProxyNotFoundError:                  // invalid proxy hostname
    case QNetworkReply::ContentOperationNotPermittedError:   // operation requested on remote content not permitted
    case QNetworkReply::ContentNotFoundError:                // remote content not found on server (similar to HTTP error 404)
    case QNetworkReply::ProtocolUnknownError:                // Unknown protocol
    case QNetworkReply::ProtocolInvalidOperationError:       // requested operation is invalid for this protocol

        if (reply->url() == _loadingUrl)
        {
            mainFrame()->setHtml(errorPage(reply));
            _isOnRekonqPage = true;
        }
        break;

    default:
        // Nothing to do here..
        break;

    }
}


QString WebPage::errorPage(QNetworkReply *reply)
{
    // display "not found" page
    QString notfoundFilePath =  KStandardDirs::locate("data", "rekonq/htmls/rekonqinfo.html");
    QFile file(notfoundFilePath);

    bool isOpened = file.open(QIODevice::ReadOnly);
    if (!isOpened)
    {
        return QString("Couldn't open the rekonqinfo.html file");
    }

    QString title = i18n("Error loading: %1", reply->url().path());
    QString urlString = reply->url().toString(QUrl::RemoveUserInfo | QUrl::RemoveQuery);

    QString iconPath = QString("file://") + KIconLoader::global()->iconPath("dialog-warning" , KIconLoader::Small);
    iconPath.replace(QL1S("16"), QL1S("128"));

    QString msg;
    msg += "<table>";
    msg += "<tr><td>";
    msg += "<img src=\"" + iconPath + "\" />";
    msg += "</td><td>";
    msg += "<h1>" + reply->errorString() + "</h1>";
    msg += "<h2>" + i18nc("%1=an URL, e.g.'kde.org'", "When connecting to: <b>%1</b>", urlString) + "</h2>";
    msg += "</td></tr></table>";

    msg += "<ul><li>" + i18n("Check the address for errors such as <b>ww</b>.kde.org instead of <b>www</b>.kde.org");
    msg += "</li><li>" + i18n("If the address is correct, try to check the network connection.") + "</li><li>" ;
    msg += i18n("If your computer or network is protected by a firewall or proxy, make sure that rekonq is permitted to access the network.");
    msg += "</li><li>" + i18n("Of course, if rekonq does not work properly, you can always say it is a programmer error ;)");
    msg += "</li></ul><br/><br/>";
    msg += "<input type=\"button\" id=\"reloadButton\" onClick=\"document.location.href='" + urlString + "';\" value=\"";
    msg += i18n("Try Again") + "\" />";

    QString html = QString(QL1S(file.readAll()))
                   .arg(title)
                   .arg(msg)
                   ;
    return html;
}


// WARNING
// this code is actually copied from KWebPage::downloadRequest to save
// downloads data before. If you have some better ideas about,
// feel free to let us know about :)
void WebPage::downloadThings(const QNetworkRequest &request, const QString &suggestedFileName)
{
    KUrl destUrl;
    KUrl srcUrl(request.url());
    
    if( !ReKonfig::kgetDownload() && suggestedFileName.isEmpty() )
    {
        kDebug() << "Using KWebPage downloadRequest..";
        Application::historyManager()->addDownload(srcUrl.pathOrUrl() , destUrl.pathOrUrl());
        KWebPage::downloadRequest(request);
        return;
    }
    
    int result = KIO::R_OVERWRITE;

    do
    {
        QString fName = suggestedFileName.isEmpty()
            ? srcUrl.fileName()
            : suggestedFileName;
            
        destUrl = KFileDialog::getSaveFileName(fName, QString(), view());

        if (destUrl.isLocalFile())
        {
            QFileInfo finfo(destUrl.toLocalFile());
            if (finfo.exists())
            {
                QDateTime now = QDateTime::currentDateTime();
                QPointer<KIO::RenameDialog> dlg = new KIO::RenameDialog(view(),
                        i18n("Overwrite File?"),
                        srcUrl,
                        destUrl,
                        KIO::RenameDialog_Mode(KIO::M_OVERWRITE | KIO::M_SKIP),
                        -1,
                        finfo.size(),
                        now.toTime_t(),
                        finfo.created().toTime_t(),
                        now.toTime_t(),
                        finfo.lastModified().toTime_t()
                                                                       );
                result = dlg->exec();
                delete dlg;
            }
        }
    }
    while (result == KIO::R_CANCEL && destUrl.isValid());

    if (result == KIO::R_OVERWRITE && destUrl.isValid())
    {
        // now store data
        // now, destUrl, srcUrl
        Application::historyManager()->addDownload(srcUrl.pathOrUrl() , destUrl.pathOrUrl());

        if (ReKonfig::kgetDownload())
        {
            //KGet integration:
            if (!QDBusConnection::sessionBus().interface()->isServiceRegistered("org.kde.kget"))
            {
                KToolInvocation::kdeinitExecWait("kget");
            }
            QDBusInterface kget("org.kde.kget", "/KGet", "org.kde.kget.main");
            if (kget.isValid())
            {
                kget.call("addTransfer", srcUrl.prettyUrl(), destUrl.prettyUrl(), true);
                return;
            }
        }

        // else, use KIO or fallback to it
        KIO::Job *job = KIO::file_copy(srcUrl, destUrl, -1, KIO::Overwrite);
        QVariant attr = request.attribute(static_cast<QNetworkRequest::Attribute>(KIO::AccessManager::MetaData));
        if (attr.isValid() && attr.type() == QVariant::Map)
            job->setMetaData(KIO::MetaData(attr.toMap()));

        job->addMetaData(QL1S("MaxCacheSize"), QL1S("0")); // Don't store in http cache.
        job->addMetaData(QL1S("cache"), QL1S("cache"));   // Use entry from cache if available.
        job->uiDelegate()->setAutoErrorHandlingEnabled(true);
    }
}


void WebPage::downloadAllContentsWithKGet(QPoint)
{
    QSet<QString> contents;
    KUrl baseUrl(currentFrame()->url());
    KUrl relativeUrl;

    QWebElementCollection images = mainFrame()->documentElement().findAll("img");
    foreach(QWebElement img, images)
    {
        relativeUrl.setEncodedUrl(img.attribute("src").toUtf8(), KUrl::TolerantMode);
        contents << baseUrl.resolved(relativeUrl).toString();
    }

    QWebElementCollection links = mainFrame()->documentElement().findAll("a");
    foreach(QWebElement link, links)
    {
        relativeUrl.setEncodedUrl(link.attribute("href").toUtf8(), KUrl::TolerantMode);
        contents << baseUrl.resolved(relativeUrl).toString();
    }

    if (!QDBusConnection::sessionBus().interface()->isServiceRegistered("org.kde.kget"))
    {
        KToolInvocation::kdeinitExecWait("kget");
    }
    QDBusInterface kget("org.kde.kget", "/KGet", "org.kde.kget.main");
    if (kget.isValid())
    {
        kget.call("importLinks", QVariant(contents.toList()));
    }
}


void WebPage::showSSLInfo(QPoint)
{
    if (_sslInfo.isValid())
    {
        QPointer<KSslInfoDialog> dlg = new KSslInfoDialog(view());
        dlg->setSslInfo(_sslInfo.certificateChain(),
                        _sslInfo.peerAddress().toString(),
                        mainFrame()->url().host(),
                        _sslInfo.protocol(),
                        _sslInfo.ciphers(),
                        _sslInfo.usedChiperBits(),
                        _sslInfo.supportedChiperBits(),
                        KSslInfoDialog::errorsFromString(_sslInfo.certificateErrors())
                       );

        dlg->exec();
        delete dlg;

        return;
    }

    if (mainFrame()->url().scheme() == QL1S("https"))
    {
        KMessageBox::error(view(),
                           i18n("The SSL information for this site appears to be corrupt."),
                           i18nc("Secure Sockets Layer", "SSL")
                          );
    }
    else
    {
        KMessageBox::information(view(),
                                 i18n("This site doesn't contain SSL information."),
                                 i18nc("Secure Sockets Layer", "SSL")
                                );
    }
}


void WebPage::updateImage(bool ok)
{
    if (ok)
    {
        NewTabPage p(mainFrame());
        p.snapFinished();
    }
}
