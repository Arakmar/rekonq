/* ============================================================
*
* This file is a part of the rekonq project
*
* Copyright (C) 2010-2012 by Andrea Diamantini <adjam7 at gmail dot com>
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
#include "adblockmanager.h"
#include "adblockmanager.moc"

// Auto Includes
#include "rekonq.h"

// Local Includes
#include "adblocknetworkreply.h"
#include "adblockwidget.h"
#include "blockedelementswidget.h"

#include "webpage.h"

// KDE Includes
#include <KIO/FileCopyJob>
#include <KStandardDirs>

// Qt Includes
#include <QUrl>
#include <QWebElement>
#include <QNetworkReply>
#include <QNetworkRequest>


AdBlockManager::AdBlockManager(QObject *parent)
    : QObject(parent)
    , _isAdblockEnabled(false)
    , _isHideAdsEnabled(false)
{
    loadSettings();
}


AdBlockManager::~AdBlockManager()
{
    _whiteList.clear();
    _blackList.clear();
    _hideList.clear();
}


bool AdBlockManager::isEnabled()
{
    return _isAdblockEnabled;
}


bool AdBlockManager::isHidingElements()
{
    return _isHideAdsEnabled;
}


void AdBlockManager::loadSettings()
{
    // first, check this...
    QString adblockFilePath = KStandardDirs::locateLocal("appdata" , QL1S("adblockrc"));
    if (!QFile::exists(adblockFilePath))
    {
        QString generalAdblockFilePath = KStandardDirs::locate("appdata" , QL1S("adblockrc"));
        QFile adblockFile(generalAdblockFilePath);
        bool copied = adblockFile.copy(adblockFilePath);
        if (!copied)
        {
            kDebug() << "oh oh... Problems copying default adblock file";
            return;
        }
    }
    _adblockConfig = KSharedConfig::openConfig("adblockrc", KConfig::SimpleConfig, "appdata");
    // ----------------

    _hostWhiteList.clear();
    _hostBlackList.clear();

    _whiteList.clear();
    _blackList.clear();
    _hideList.clear();

    KConfigGroup settingsGroup(_adblockConfig, "Settings");
    _isAdblockEnabled = settingsGroup.readEntry("adBlockEnabled", false);

    // no need to load filters if adblock is not enabled :)
    if (!_isAdblockEnabled)
        return;

    // just to be sure..
    _isHideAdsEnabled = settingsGroup.readEntry("hideAdsEnabled", false);

    // ----------------------------------------------------------

    QDateTime today = QDateTime::currentDateTime();
    QDateTime lastUpdate = QDateTime::fromString(settingsGroup.readEntry("lastUpdate", QString()));
    int days = settingsGroup.readEntry("updateInterval", 7);

    bool allSubscriptionsNeedUpdate = (today > lastUpdate.addDays(days));
    if (allSubscriptionsNeedUpdate)
    {
        settingsGroup.writeEntry("lastUpdate", today.toString());
    }

    // (Eventually) update and load automatic rules
    KConfigGroup filtersGroup(_adblockConfig, "FiltersList");
    for (int i = 0; i < 60; i++)
    {
        QString n = QString::number(i + 1);
        if (!filtersGroup.hasKey("FilterEnabled-" + n))
            continue;

        bool isFilterEnabled = filtersGroup.readEntry("FilterEnabled-" + n, false);
        if (!isFilterEnabled)
            continue;

        bool fileExists = subscriptionFileExists(i);
        if (allSubscriptionsNeedUpdate || !fileExists)
        {
            kDebug() << "FILE SHOULDN'T EXIST. updating subscription";
            updateSubscription(i);
        }
        else
        {
            QString rulesFilePath = KStandardDirs::locateLocal("appdata" , QL1S("adblockrules_") + n);
            loadRules(rulesFilePath);
        }
    }

    // load local rules
    QString localRulesFilePath = KStandardDirs::locateLocal("appdata" , QL1S("adblockrules_local"));
    loadRules(localRulesFilePath);
}


void AdBlockManager::loadRules(const QString &rulesFilePath)
{
    QFile ruleFile(rulesFilePath);
    if (!ruleFile.open(QFile::ReadOnly | QFile::Text))
    {
        kDebug() << "Unable to open rule file" << rulesFilePath;
        return;
    }

    QTextStream in(&ruleFile);
    while (!in.atEnd())
    {
        QString stringRule = in.readLine();
        loadRuleString(stringRule);
    }
}


void AdBlockManager::loadRuleString(const QString &stringRule)
{
    // ! rules are comments
    if (stringRule.startsWith('!'))
        return;

    // [ rules are ABP info
    if (stringRule.startsWith('['))
        return;

    // empty rules are just dangerous..
    // (an empty rule in whitelist allows all, in blacklist blocks all..)
    if (stringRule.isEmpty())
        return;

    // white rules
    if (stringRule.startsWith(QL1S("@@")))
    {
        const QString filter = stringRule.mid(2);
        if (_hostWhiteList.tryAddFilter(filter))
            return;

        AdBlockRule rule(filter);
        _whiteList << rule;
        return;
    }

    // hide (CSS) rules
    if (stringRule.startsWith(QL1S("##")))
    {
        _hideList << stringRule.mid(2);
        return;
    }

    // TODO implement domain-specific hiding
    if (stringRule.contains(QL1S("##")))
        return;

    if (_hostBlackList.tryAddFilter(stringRule))
        return;

    AdBlockRule rule(stringRule);
    _blackList << rule;
}


QNetworkReply *AdBlockManager::block(const QNetworkRequest &request, WebPage *page)
{
    if (!_isAdblockEnabled)
        return 0;

    // we (ad)block just http traffic
    if (request.url().scheme() != QL1S("http"))
        return 0;

    QString urlString = request.url().toString();
    // We compute a lowercase version of the URL so each rule does not
    // have to do it.
    const QString urlStringLowerCase = urlString.toLower();
    const QString host = request.url().host();

    // check white rules before :)
    if (_hostWhiteList.match(host))
    {
        kDebug() << "ADBLOCK: WHITE RULE (@@) Matched by string: " << urlString;
        return 0;
    }

    Q_FOREACH(const AdBlockRule & filter, _whiteList)
    {
        if (filter.match(request, urlString, urlStringLowerCase))
        {
            kDebug() << "ADBLOCK: WHITE RULE (@@) Matched by string: " << urlString;
            return 0;
        }
    }

    // then check the black ones :(
    if (_hostBlackList.match(host))
    {
        kDebug() << "ADBLOCK: BLACK RULE Matched by string: " << urlString;
        AdBlockNetworkReply *reply = new AdBlockNetworkReply(request, urlString, this);
        _blockedElements << request.url().toString();
        page->setHasAdBlockedElements(true);
        return reply;
    }

    // get sure page is extant to perform elements check. bug:282012
    if (!page)
        return 0;

    Q_FOREACH(const AdBlockRule & filter, _blackList)
    {
        if (filter.match(request, urlString, urlStringLowerCase))
        {
            kDebug() << "ADBLOCK: BLACK RULE Matched by string: " << urlString;

            QWebElement document = page->mainFrame()->documentElement();
            QWebElementCollection elements = document.findAll("*");
            Q_FOREACH(QWebElement el, elements)
            {
                QString srcAttribute = el.attribute("src");
                if (srcAttribute.isEmpty())
                    continue;

                if (!srcAttribute.startsWith(QL1S("http")))
                    srcAttribute = host + srcAttribute;

                if (filter.match(request, srcAttribute, srcAttribute.toLower()))
                {
                    el.setStyleProperty(QL1S("visibility"), QL1S("hidden"));
                    el.setStyleProperty(QL1S("width"), QL1S("0"));
                    el.setStyleProperty(QL1S("height"), QL1S("0"));
                }
            }

            AdBlockNetworkReply *reply = new AdBlockNetworkReply(request, urlString, this);
            _blockedElements << request.url().toString();
            page->setHasAdBlockedElements(true);
            return reply;
        }
    }

    // no match
    return 0;
}


void AdBlockManager::applyHidingRules(WebPage *page)
{
    if (!page)
        return;

    if (!_isAdblockEnabled)
        return;

    if (!_isHideAdsEnabled)
        return;

    QWebElement document = page->mainFrame()->documentElement();

    // HIDE RULES
    Q_FOREACH(const QString & filter, _hideList)
    {
        QWebElementCollection elements = document.findAll(filter);

        Q_FOREACH(QWebElement el, elements)
        {
            if (el.isNull())
                continue;
            kDebug() << "Hide element: " << el.localName();
            el.setStyleProperty(QL1S("visibility"), QL1S("hidden"));
            el.removeFromDocument();
            _hidedElements++;
            page->setHasAdBlockedElements(true);
        }
    }
}


void AdBlockManager::updateSubscription(int i)
{
    KConfigGroup filtersGroup(_adblockConfig, "FiltersList");
    QString n = QString::number(i + 1);

    QString fUrl = filtersGroup.readEntry("FilterURL-" + n, QString());
    KUrl subUrl = KUrl(fUrl);

    QString rulesFilePath = KStandardDirs::locateLocal("appdata" , QL1S("adblockrules_") + n);
    KUrl destUrl = KUrl(rulesFilePath);

    KIO::FileCopyJob* job = KIO::file_copy(subUrl , destUrl, -1, KIO::HideProgressInfo);
    job->metaData().insert("ssl_no_client_cert", "TRUE");
    job->metaData().insert("ssl_no_ui", "TRUE");
    job->metaData().insert("UseCache", "false");
    job->metaData().insert("cookies", "none");
    job->metaData().insert("no-auth", "true");

    connect(job, SIGNAL(finished(KJob *)), this, SLOT(slotFinished(KJob *)));
}


void AdBlockManager::slotFinished(KJob *job)
{
    if (job->error())
        return;

    KIO::FileCopyJob *fJob = qobject_cast<KIO::FileCopyJob *>(job);
    KUrl url = fJob->destUrl();
    url.setProtocol(QString()); // this is needed to load local url well :(
    loadRules(url.url());
}


bool AdBlockManager::subscriptionFileExists(int i)
{
    QString n = QString::number(i + 1);

    QString rulesFilePath = KStandardDirs::locateLocal("appdata" , QL1S("adblockrules_") + n);
    return QFile::exists(rulesFilePath);
}


void AdBlockManager::showSettings()
{
    QPointer<KDialog> dialog = new KDialog();
    dialog->setCaption(i18nc("@title:window", "Ad Block Settings"));
    dialog->setButtons(KDialog::Ok | KDialog::Cancel);

    AdBlockWidget widget(_adblockConfig);
    dialog->setMainWidget(&widget);
    connect(dialog, SIGNAL(okClicked()), &widget, SLOT(save()));
    connect(dialog, SIGNAL(okClicked()), this, SLOT(loadSettings()));
    dialog->exec();

    dialog->deleteLater();
}


void AdBlockManager::addCustomRule(const QString &stringRule, bool reloadPage)
{
    // save rule in local filters
    QString localRulesFilePath = KStandardDirs::locateLocal("appdata" , QL1S("adblockrules_local"));

    QFile ruleFile(localRulesFilePath);
    if (!ruleFile.open(QFile::WriteOnly | QFile::Append))
    {
        kDebug() << "Unable to open rule file" << localRulesFilePath;
        return;
    }

    QTextStream out(&ruleFile);
    out << stringRule << '\n';

    ruleFile.close();

    // load it
    loadRuleString(stringRule);

    // eventually reload page
    if (reloadPage)
        emit reloadCurrentPage();
}


void AdBlockManager::showBlockedItemDialog()
{
    QPointer<KDialog> dialog = new KDialog();
    dialog->setCaption(i18nc("@title:window", "Blocked elements"));
    dialog->setButtons(KDialog::Ok);

    BlockedElementsWidget widget(this);
    widget.setBlockedElements(_blockedElements);
    widget.setHidedElements(_hidedElements);

    dialog->setMainWidget(&widget);
    dialog->exec();

    Q_FOREACH(const QString & r, widget.rulesToAdd())
    {
        addCustomRule(r);
    }

    if (widget.pageNeedsReload())
        emit reloadCurrentPage();

    dialog->deleteLater();
}


void AdBlockManager::clearElementsLists()
{
    _blockedElements.clear();
    _hidedElements = 0;
}
