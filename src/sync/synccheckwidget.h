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


#ifndef SYNC_CHECK_WIDGET_H
#define SYNC_CHECK_WIDGET_H


// Rekonq Includes
#include "rekonq_defines.h"

// Ui Includes
#include "ui_sync_check.h"

// Qt Includes
#include <QWizardPage>


class SyncCheckWidget : public QWizardPage, private Ui::SyncCheck
{
    Q_OBJECT

public:
    SyncCheckWidget(QWidget *parent = 0);
    void initializePage();

private Q_SLOTS:
    void updateWidget(Rekonq::SyncData, bool, QString);
};

#endif // SYNC_CHECK_WIDGET_H
