/* ============================================================
*
* This file is a part of the rekonq project
*
* Copyright (C) 2011 by Andrea Diamantini <adjam7 at gmail dot com>
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


// Auto Includes
#include "sslwidget.h"
#include "sslwidget.moc"

// Local includes
#include "application.h"
#include "historymanager.h"

// KDE Includes

// Qt Includes
#include <QtGui/QDialogButtonBox>
#include <QtGui/QFormLayout>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>


SSLWidget::SSLWidget(const QUrl &url, const WebSslInfo &info, QWidget *parent)
    : QMenu(parent)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setMinimumWidth(400);

    QList<QSslCertificate> caList = info.certificateChain();
    QSslCertificate firstCA = caList.first();
    
    QFormLayout *layout = new QFormLayout(this);

    QLabel *label;
    
    label = new QLabel(this);
    label->setText( QL1S("<h4>") + url.host() + QL1S("</h4>") );
    layout->addRow(label);

    if (firstCA.isNull())
    {
        label = new QLabel(this);
        label->setText( i18n("Warning: this site is carrying a NULL certificate") );
        layout->addRow(label);
    }
    else
    {
        if (firstCA.isValid())
        {
            label = new QLabel(this);
            label->setText( i18n("This certificate for this site is Valid and has been verified by ") 
                            + firstCA.subjectInfo(QSslCertificate::CommonName) );
            layout->addRow(label);
        }
        else
        {
            label = new QLabel(this);
            label->setText( i18n("Warning: The certificate for this site is NOT valid!") );
            layout->addRow(label);
        }
        
        label = new QLabel(this);
        label->setText( QL1S("<hr /><h4>") + i18n("Issued to") + QL1S("</h4>") ); // ----------------------------------------------- //
        layout->addRow(label);

            label = new QLabel(this);
            label->setText( QL1S("COMMON NAME: ") + firstCA.subjectInfo(QSslCertificate::CommonName) );
            layout->addRow(label);

            label = new QLabel(this);
            label->setText( QL1S("ORGANIZATION: ") + firstCA.subjectInfo(QSslCertificate::Organization) );
            layout->addRow(label);
            
            label = new QLabel(this);
            label->setText( QL1S("ORGANIZATION UNIT: ") + firstCA.subjectInfo(QSslCertificate::OrganizationalUnitName) );
            layout->addRow(label);
            
            label = new QLabel(this);
            label->setText( QL1S("SERIAL NUMBER: ") + QL1S( firstCA.serialNumber().toHex() ) );
            layout->addRow(label);        
            
        label = new QLabel(this);
        label->setText( QL1S("<h4>") + i18n("Issued by") + QL1S("</h4>") );
        layout->addRow(label);

            label = new QLabel(this);
            label->setText( QL1S("COMMON NAME: ") + firstCA.issuerInfo(QSslCertificate::CommonName) );
            layout->addRow(label);

            label = new QLabel(this);
            label->setText( QL1S("ORGANIZATION: ") + firstCA.issuerInfo(QSslCertificate::Organization) );
            layout->addRow(label);
            
            label = new QLabel(this);
            label->setText( QL1S("ORGANIZATION UNIT: ") + firstCA.issuerInfo(QSslCertificate::OrganizationalUnitName) );
            layout->addRow(label);
            
        label = new QLabel(this);
        label->setText( QL1S("<h4>") + i18n("Validity Period") + QL1S("</h4>") );
        layout->addRow(label);

            label = new QLabel(this);
            label->setText( i18n("Issued on: ") + firstCA.effectiveDate().date().toString(Qt::SystemLocaleShortDate) );
            layout->addRow(label);

            label = new QLabel(this);
            label->setText( i18n("Expires on: ") + firstCA.expiryDate().date().toString(Qt::SystemLocaleShortDate) );
            layout->addRow(label);

            
        label = new QLabel(this);
        label->setText( QL1S("<h4>") + i18n("FingerPrints") + QL1S("</h4>") );
        layout->addRow(label);
        
            label = new QLabel(this);
            label->setText( QL1S("SHA1 FingerPrint: ") + QL1S( firstCA.digest(QCryptographicHash::Sha1).toHex() ) );
            layout->addRow(label);

            label = new QLabel(this);
            label->setText( QL1S("MD5 FingerPrint: ") + QL1S( firstCA.digest(QCryptographicHash::Md5).toHex() ) );
            layout->addRow(label);

    }
    
    // ------------------------------------------------------------------------------------------------------------------
    label = new QLabel(this);
    label->setText( QL1S("<hr /><h4>") + i18n("Site Information") + QL1S("</h4>") );
    layout->addRow(label);

        label = new QLabel(this);

        if (rApp->historyManager()->historyContains(url.toString())) //FIXME change with visit count > 1
        {
            label->setText( i18n("You just visited this site") );
        }
        else
        {
            label->setText( i18n("It's your first time visiting this site") );            
        }
        layout->addRow(label);

    // -----------------------------------------------------------------------------------
    setLayout(layout);
}


void SSLWidget::showAt(const QPoint &pos)
{
    adjustSize();

    QPoint p(pos.x() - width(), pos.y() + 10);
    move(p);
    show();
}


void SSLWidget::accept()
{

    close();
}