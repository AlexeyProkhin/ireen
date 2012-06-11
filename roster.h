/****************************************************************************
**
** Ireen — cross-platform OSCAR protocol library
**
** Copyright © 2012 Ruslan Nigmatullin <euroelessar@yandex.ru>
**                  Alexey Prokhin <alexey.prokhin@yandex.ru>
**
*****************************************************************************
**
** $IREEN_BEGIN_LICENSE$
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as
** published by the Free Software Foundation, either version 3
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
** See the GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program.  If not, see http://www.gnu.org/licenses/.
** $IREEN_END_LICENSE$
**
****************************************************************************/

#ifndef IREEN_ROSTER_H
#define IREEN_ROSTER_H

#include "capability.h"

#include <QExplicitlySharedDataPointer>
#include <QDateTime>
#include "core/snachandler.h"
#include "core/sessiondataitem.h"
#include "feedbag.h"

namespace Ireen {

class ContactItemPrivate;
class StatusItemPrivate;
class Client;

class IREEN_EXPORT ContactItem
{
public:
	ContactItem();
	~ContactItem();
	ContactItem(const ContactItem &other);
	void operator=(const ContactItem &other);
	QString uin() const;
	QString name() const;
	quint16 groupId() const;
	QString group() const;
	QString protocol() const;
	QString comment() const;
private:
	friend class Roster;
	QExplicitlySharedDataPointer<ContactItemPrivate> d;
};

class IREEN_EXPORT StatusItem
{
public:
	StatusItem();
	~StatusItem();
	StatusItem(const StatusItem &other);
	void operator=(const StatusItem &other);
	quint16 statusId() const;
	quint16 statusFlags() const;
	QString statusText() const;
	QDateTime statusTextUpdateTime() const;
	Capabilities capabilities() const;
	QDateTime onlineSince() const;
	QDateTime awaySince() const;
	QDateTime registrationTime() const;
	// Returns parsed tlv 0x1D that, for some reason only oscar devs know about,
	// contains a lot of data:
	//    1. status text
	//    2. status text update time
	//    3. new-style extended statuses
	//    4. avatar hash
	SessionDataItemMap statusData() const;
private:
	friend class Roster;
	QExplicitlySharedDataPointer<StatusItemPrivate> d;
};

class IREEN_EXPORT Roster : public QObject, public SNACHandler, public FeedbagItemHandler
{
	Q_OBJECT
	Q_INTERFACES(Ireen::SNACHandler Ireen::FeedbagItemHandler)
public:
	Roster(Client *client);
signals:
	void contactItemReceived(const Ireen::ContactItem &item);
	void contactStatusUpdated(const QString &uin, const Ireen::StatusItem &item);
	void contactItemRemoved(const QString &uin);
	void groupItemAdded(const QString &groupName);
	void groupItemRenamed(const QString &newGroupName, const QString &oldGroupName);
	void groupItemRemoved(const QString &groupName);
protected:
	bool handleFeedbagItem(Feedbag *feedbag, const FeedbagItem &item, Feedbag::ModifyType type, FeedbagError error);
	void handleAddModifyCLItem(const FeedbagItem &item, Feedbag::ModifyType type);
	void handleRemoveCLItem(const FeedbagItem &item);
	virtual void handleSNAC(AbstractConnection *conn, const SNAC &snac);
	void handleNewStatus(const SNAC &snac, bool online);
private slots:
	void reloadingStarted();
	void loginFinished();
};

} // namespace Ireen

#endif // IREEN_ROSTER_H
