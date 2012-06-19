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

#include "roster.h"
#include "feedbag.h"
#include "status.h"
#include "client.h"
#include "core/sessiondataitem.h"
#include "core/util.h"

namespace Ireen {

enum SsiBuddyTlvs
{
	SsiBuddyReqAuth = 0x0066,
	SsiBuddyProto   = 0x0084,
	SsiBuddyNick    = 0x0131,
	SsiBuddyComment = 0x013c
};

class ContactItemPrivate : public QSharedData
{
public:
	void setFeedbagItem(const FeedbagItem &item);
private:
	friend class ContactItem;
	FeedbagItem feedbagItem;
};

class StatusItemPrivate : public QSharedData
{
public:
	void setTlvs(const TLVMap &tlvs, bool online);
private:
	friend class StatusItem;
	bool online;
	quint16 statusId;
	quint16 statusFlags;
	TLVMap tlvs;
	SessionDataItemMap statusData;
};

class RosterPrivate
{
public:
	Feedbag *feedbag;
};

ContactItem::ContactItem() :
	d(new ContactItemPrivate)
{
}

ContactItem::ContactItem(const FeedbagItem &item) :
	d(new ContactItemPrivate)
{
	d->setFeedbagItem(item);
}


ContactItem::~ContactItem()
{
}

ContactItem::ContactItem(const ContactItem &other) :
	d(other.d)
{
}

void ContactItem::operator=(const ContactItem &other)
{
	d = other.d;
}

void ContactItemPrivate::setFeedbagItem(const FeedbagItem &item)
{
	feedbagItem = item;
}

QString ContactItem::uin() const
{
	return d->feedbagItem.name();
}

QString ContactItem::name() const
{
	return d->feedbagItem.field<QString>(SsiBuddyNick);
}

void ContactItem::setName(const QString &name)
{
	d->feedbagItem.setField(SsiBuddyNick, name);
}

quint16 ContactItem::groupId() const
{
	return d->feedbagItem.groupId();
}

QString ContactItem::group() const
{
	FeedbagItem group = d->feedbagItem.feedbag()->groupItem(groupId());
	Q_ASSERT(!group.isNull());
	return group.name();
}

QString ContactItem::protocol() const
{
	return d->feedbagItem.field<QString>(SsiBuddyProto);
}

QString ContactItem::comment() const
{
	return d->feedbagItem.field<QString>(SsiBuddyComment);
}

void ContactItem::setComment(const QString &comment)
{
	d->feedbagItem.setField(SsiBuddyComment, comment);
}

bool ContactItem::isAuthorizedBy() const
{
	return !d->feedbagItem.containsField(SsiBuddyReqAuth);
}

FeedbagItem &ContactItem::feedbagItem() const
{
	return d->feedbagItem;
}

StatusItem::StatusItem() :
	d(new StatusItemPrivate)
{
}

StatusItem::~StatusItem()
{
}

StatusItem::StatusItem(const StatusItem &other) :
	d(other.d)
{
}

void StatusItem::operator=(const StatusItem &other)
{
	d = other.d;
}

void StatusItemPrivate::setTlvs(const TLVMap &tlvs_, bool online_)
{
	online = online_;
	tlvs = tlvs_;
	if (online && tlvs.contains(0x06)) {
		DataUnit data(tlvs.value(0x06));
		statusFlags = data.read<quint16>();
		statusId = data.read<quint16>();
		//if (!status.setStatusFlag(statusId & 0x0fff))
		//	status.setStatusType(Status::Online);
	} else {
		statusId = online ? Status::Online : Status::Offline;
	}
	statusData = tlvs.value(0x1D);
}

quint16 StatusItem::statusId() const
{
	return d->statusId;
}

quint16 StatusItem::statusFlags() const
{
	return d->statusFlags;
}

QDateTime StatusItem::statusTextUpdateTime() const
{
	if (d->statusData.contains(0x0d)) {
		quint16 time = d->statusData.value(0x0d).read<quint16>();
		debug() << "Status note update time" << time;
		return QDateTime::fromTime_t(time);
	}
	return QDateTime();
}

QString StatusItem::statusText() const
{
	if (d->statusData.contains(0x02)) {
		DataUnit data(d->statusData.value(0x02));
		QByteArray note_data = data.read<QByteArray, quint16>();
		QByteArray encoding = data.read<QByteArray, quint16>();
		QTextCodec *codec = 0;
		if (!encoding.isEmpty()) {
			codec = QTextCodec::codecForName(encoding);
			if (!codec)
				debug() << "Server sent wrong encoding for status note";
		}
		if (!codec)
			codec = Util::utf8Codec();
		return codec->toUnicode(note_data);
	}
	return QString();
}

Capabilities StatusItem::capabilities() const
{
	Capabilities caps;
	if (d->tlvs.contains(0x000d)) {
		DataUnit data(d->tlvs.value(0x000d));
		while (data.dataSize() >= 16)
			caps << data.read<Capability>();
	}
	if (d->tlvs.contains(0x0019)) {
		DataUnit data(d->tlvs.value(0x0019));
		while (data.dataSize() >= 2)
			caps << Capability(data.readData(2));
	}
	return caps;
}

QDateTime StatusItem::onlineSince() const
{
	if (d->tlvs.contains(0x000f))
		return QDateTime::fromTime_t(QDateTime::currentDateTime().toTime_t() - d->tlvs.value<quint32>(0x000f));
	else if (d->tlvs.contains(0x0003))
		return QDateTime::fromTime_t(d->tlvs.value<quint32>(0x0003));
	else
		return QDateTime();
}

QDateTime StatusItem::awaySince() const
{
	if (d->tlvs.contains(0x0004))
		return QDateTime::currentDateTime().addSecs(-60 * d->tlvs.value<quint32>(0x0004));
	else if (d->statusId != Status::Online && d->online)
		return QDateTime::currentDateTime();
	else
		return QDateTime();
}

QDateTime StatusItem::registrationTime() const
{
	if (d->tlvs.contains(0x0005))
		return QDateTime::fromTime_t(d->tlvs.value<quint32>(0x0005));
	else
		return QDateTime();
}

SessionDataItemMap StatusItem::statusData() const
{
	return d->statusData;
}

Roster::Roster(Client *client, Feedbag *feedbag)
{
	m_infos << SNACInfo(ServiceFamily, ServiceServerAsksServices)
			<< SNACInfo(BuddyFamily, UserOnline)
			<< SNACInfo(BuddyFamily, UserOffline)
			<< SNACInfo(BuddyFamily, UserSrvReplyBuddy)
			<< SNACInfo(ListsFamily, ListsAuthRequest)
			<< SNACInfo(ListsFamily, ListsSrvAuthResponse);
	m_types << SsiBuddy << SsiGroup;

	client->registerHandler(this);
	feedbag->registerHandler(this);
	client->registerInitializationSnac(BuddyFamily, UserCliReqBuddy);
}

bool Roster::handleFeedbagItem(Feedbag *feedbag, const FeedbagItem &item, Feedbag::ModifyType type, FeedbagError error)
{
	Q_UNUSED(feedbag);
	if (type != Feedbag::Remove && error == FeedbagError::RequiresAuthorization) {
		// Failed to add the contact because it requires authorization.
		// Try to readd the contact with SsiBuddyReqAuth field present.
		Q_ASSERT(!item.isInList());
		FeedbagItem itemCopy = item;
		itemCopy.setId(item.feedbag()->uniqueItemId(SsiBuddy));
		itemCopy.setField(SsiBuddyReqAuth);
		itemCopy.add();
		return true;
	} else if (error != FeedbagError::NoError) {
		return false;
	}

	if (type == Feedbag::Remove)
		handleRemoveCLItem(item);
	else
		handleAddModifyCLItem(item, type);
	return true;
}

void Roster::handleAddModifyCLItem(const FeedbagItem &item, Feedbag::ModifyType type)
{
	Q_UNUSED(type);
	switch (item.type()) {
	case SsiBuddy: {
		if (item.name().isEmpty())
			break;
		debug(DebugVerbose) << "The contact" << item.name() << "has been added or updated";
		ContactItem newContact;
		newContact.d->setFeedbagItem(item);
		emit contactItemReceived(newContact);
		break;
	}
	case SsiGroup: {
		if (item.groupId() == 0) // Skip Root group
			break;
		FeedbagItem old = item.feedbag()->groupItem(item.groupId());
		if (old.isInList()) {
			if (old.name() != item.name()) {
				debug(DebugVerbose) << "The group" << old.name() << "has been renamed to" << item.name();
				emit groupItemRenamed(item.name(), old.name());
			}
		} else {
			debug(DebugVerbose) << "The group" << item.name() << "has been added";
			emit groupItemAdded(item.name());

		}
		break;
	}
	}
}

void Roster::handleRemoveCLItem(const FeedbagItem &item)
{
	switch (item.type()) {
	case SsiBuddy: {
		debug(DebugVerbose) << "The contact" << item.name() << "has been removed";
		emit contactItemRemoved(item.name());
		break;
	}
	case SsiGroup: {
		if (item.groupId() == 0) // Skip Root group
			break;
		debug(DebugVerbose) << "The group" << item.name() << "has been removed";
		emit groupItemRemoved(item.name());
		break;
	}
	}
}

void Roster::handleSNAC(AbstractConnection *conn, const SNAC &sn)
{
	switch ((sn.family() << 16) | sn.subtype()) {
	case ServiceFamily << 16 | ServiceServerAsksServices: {
		// Requesting client-side contactlist rights
		SNAC snac(BuddyFamily, UserCliReqBuddy);
		snac.appendTLV<quint16>(0x0005, 0x0f);
//		snac.appendTLV(0x0006, QByteArray::fromHex("000000"));
//		snac.appendTLV<quint8>(0x0007, 0x00);
		// Request facebook contacts
		snac.appendTLV<quint8>(0x0008, 1);
		// Query flags: 1 = Enable Avatars
		//              2 = Enable offline status message notification
		//              4 = Enable Avatars for offline contacts
		//              8 = Use reject for not authorized contacts
		//snac.appendTLV<quint16>(0x05, 7);
		conn->send(snac);
		break;
	}
	case BuddyFamily << 16 | UserOnline:
		handleNewStatus(sn, true);
		break;
	case BuddyFamily << 16 | UserOffline:
		handleNewStatus(sn, false);
		break;
	case BuddyFamily << 16 | UserSrvReplyBuddy:
		debug() << IMPLEMENT_ME << "BuddyFamily, UserSrvReplyBuddy";
		break;
	case ListsFamily << 16 | ListsAuthRequest: {
		QString uin = sn.read<QString, quint8>();
		QString reason = sn.read<QString, qint16>();
		debug() << QString("Authorization request from \"%1\" with reason \"%2").arg(uin).arg(reason);
		emit authorizationRequestReceived(uin, reason);
		break;
	}
	case ListsFamily << 16 | ListsSrvAuthResponse: {
		QString uin = sn.read<QString, qint8>();
		bool isAccepted = sn.read<qint8>();
		QString reason = sn.read<QString, qint16>();
		QString verb = isAccepted ? "accepted" : "declined";
		debug() << QString("Our authorization request to \"%1\" has been %2 with reason \"%3")
				   .arg(uin)
				   .arg(verb)
				   .arg(reason);
		emit authorizationRequestReply(uin, isAccepted, reason);
		break;
	}
	}
}

void Roster::handleNewStatus(const SNAC &snac, bool online)
{
	QString uin = snac.read<QString, quint8>();
	quint16 warning_level = snac.read<quint16>();
	Q_UNUSED(warning_level);
	TLVMap tlvs = snac.read<TLVMap, quint16>();

	StatusItem status;
	status.d->setTlvs(tlvs, online);
	emit contactStatusUpdated(uin, status);
}

} // namespace Ireen
