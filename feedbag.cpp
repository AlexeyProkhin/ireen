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

#include "feedbag.h"
#include "snac.h"
#include "client_p.h"

#include <QCoreApplication>
#include <QQueue>
#include <QDateTime>
#include <QLatin1String>

Q_DECLARE_METATYPE(Ireen::FeedbagItem)

namespace Ireen {

QString getCompressedName(quint16 type, const QString &name)
{
	QString compressedName;
	if (type == SsiGroup) {
		bool ok = true;
		// Check is cheaper
		for (int i = 0; ok && i < name.size(); ++i)
			ok &= name[i].isLower();
		compressedName = ok ? name : name.toLower();
	} else {
		bool ok = true;
		// Check is cheaper
		for (int i = 0; ok && i < name.size(); ++i)
			ok &= !name[i].isSpace() && name[i].isLower();
		if (ok) {
			compressedName = name;
		} else {
			compressedName.reserve(name.size());
			for (int i = 0; i < name.size(); ++i) {
				const QChar &ch = name.at(i);
				if (ch.isSpace())
					continue;
				compressedName += ch.toLower();
			}
		}
	}
	return compressedName;
}

class FeedbagItemPrivate : public QSharedData
{
public:
	FeedbagItemPrivate();
	FeedbagItemPrivate(Feedbag *bag, quint16 type, quint16 item, quint16 group, const QString &name, bool inList = false);

	void send(const FeedbagItem &item, Feedbag::ModifyType operation);
	QByteArray data(Feedbag::ModifyType operation) const;
	inline void remove(FeedbagItem item);
	bool isSendingAllowed(const FeedbagItem &item, Feedbag::ModifyType operation);
	quint16 id() const { return itemType == SsiGroup ? groupId : itemId; }
	QString configId() const { return QString::number(quint64(quint64(itemType) << 16 | id()) << 32 | groupId); }

	QString recordName;
	quint16 groupId;
	quint16 itemId;
	quint16 itemType;
	TLVMap tlvs;
	Feedbag *feedbag;
	bool isInList;
};

struct FeedbagQueueItem
{
	FeedbagQueueItem(const FeedbagItem &i, Feedbag::ModifyType t) : item(i), type(t)
	{
	}
	FeedbagQueueItem() : type(static_cast<Feedbag::ModifyType>(0))
	{
	}
	FeedbagItem item;
	Feedbag::ModifyType type;
};

typedef QHash<QPair<quint16, QString>, quint16> ItemsNameHash;

struct FeedbagGroup
{
	FeedbagItem item;
	ItemsNameHash hashByName;
};

typedef QHash<QPair<quint16, quint16>, FeedbagItem> AllItemsHash;
typedef QHash<quint16, FeedbagGroup> GroupHash;


struct FeedbagRootGroup : public FeedbagGroup
{
	GroupHash regulars;
};


class FeedbagPrivate
{
	Q_DECLARE_PUBLIC(Feedbag)
public:
	FeedbagPrivate(Client *client_, Feedbag *q)
		: client(client_), q_ptr(q) {}
	QList<FeedbagItemHandler*> handlersForItem(const FeedbagItem &item);
	void handleItem(FeedbagItem &item, Feedbag::ModifyType type, FeedbagError error);
	FeedbagGroup *findGroup(quint16 id);
	quint16 generateId() const;
	void finishLoading();
	static QEvent::Type updateEvent();
	FeedbagItemPrivate *getFeedbagItemPrivate(const SNAC &snac);
	void updateList();
	void updateLocalCache();

	AllItemsHash itemsById;
	QHash<quint16, QSet<quint16> > itemsByType;
	QHash<QString, FeedbagItem> temporaryBuddies;

	QList<FeedbagItem> newItems;
	FeedbagRootGroup root;
	QList<FeedbagQueueItem> modifyQueue;
	QList<QList<FeedbagQueueItem> > itemsForRequests;
	Client *client;
	QHash<quint16, FeedbagItemHandler*> handlers;
	uint lastUpdateTime;
	bool firstPacket;
	QList<quint16> limits;
	Feedbag *q_ptr;
};

FeedbagError::FeedbagError(const SNAC &sn)
{
	m_error = static_cast<ErrorEnum>(sn.read<quint16>());
}

FeedbagError::FeedbagError(FeedbagError::ErrorEnum error):
	m_error(error)
{
}

FeedbagError::ErrorEnum FeedbagError::code()
{
	return m_error;
}

QString FeedbagError::errorString()
{
	QString errorStr;
	if (m_error == NoError)
		errorStr = QCoreApplication::translate("FeedbagError", "No error");
	if (m_error == ItemNotFound)
		errorStr = QCoreApplication::translate("FeedbagError", "Item you want to modify not found in list");
	else if (m_error == ItemAlreadyExists)
		errorStr = QCoreApplication::translate("FeedbagError", "Item you want to add allready exists");
	else if (m_error == CommonError)
		errorStr = QCoreApplication::translate("FeedbagError", "Error adding item (invalid id, allready in list, invalid data)");
	else if (m_error == LimitExceeded)
		errorStr = QCoreApplication::translate("FeedbagError", "Can't add item. Limit for this type of items exceeded");
	else if (m_error == AttemtToAddIcqContactToAimList)
		errorStr = QCoreApplication::translate("FeedbagError", "Trying to add ICQ contact to an AIM list");
	else if (m_error == RequiresAuthorization)
		errorStr = QCoreApplication::translate("FeedbagError", "Can't add this contact because it requires authorization");
	else
		errorStr = QCoreApplication::translate("FeedbagError", "Unknown error (Code: %1)").arg(m_error);
	return errorStr;
}

FeedbagItemPrivate::FeedbagItemPrivate() :
	feedbag(0), isInList(false)
{
}

FeedbagItemPrivate::FeedbagItemPrivate(Feedbag *bag, quint16 type, quint16 item, quint16 group, const QString &name, bool inList):
	feedbag(bag), isInList(inList)
{
	itemType = type;
	itemId = item;
	groupId = group;
	recordName = name;
}

void FeedbagItemPrivate::send(const FeedbagItem &item, Feedbag::ModifyType operation)
{
	Q_ASSERT(operation == Feedbag::Add || operation == Feedbag::Modify || operation == Feedbag::Remove);
	if (!isSendingAllowed(item, operation))
		return;
	FeedbagPrivate *d = feedbag->d.data();
	if (d->modifyQueue.isEmpty())
		qApp->postEvent(feedbag, new QEvent(FeedbagPrivate::updateEvent()));
	// Optimize changes
	for (int i = 0; i < d->modifyQueue.size(); ++i) {
		const FeedbagQueueItem &queueItem = d->modifyQueue.at(i);
		if (queueItem.item.pairId() == item.pairId()) {
			if (queueItem.type == Feedbag::Add && operation == Feedbag::Modify)
				operation = Feedbag::Add;
			d->modifyQueue.removeAt(i);
			if (queueItem.type == Feedbag::Add && operation == Feedbag::Remove)
				return;
			else
				break;
		}
	}
	if (item.type() == SsiBuddy) {
		d->temporaryBuddies.insert(getCompressedName(SsiBuddy, item.name()), item);
	}
	d->modifyQueue.append(FeedbagQueueItem(item, operation));
}

QByteArray FeedbagItemPrivate::data(Feedbag::ModifyType operation) const
{
	DataUnit unit;
	unit.append<quint16>(recordName);
	unit.append<quint16>(groupId);
	unit.append<quint16>(itemId);
	unit.append<quint16>(itemType);
	if (operation != Feedbag::Remove) {
		unit.append<quint16>(tlvs.valuesSize());
		unit.append(tlvs);
	} else {
		unit.append<quint16>(0);
	}
	return unit;
}

void FeedbagItemPrivate::remove(FeedbagItem item)
{
	item.d->tlvs.clear();
	isInList = false;
	send(item, Feedbag::Remove);
}

bool FeedbagItemPrivate::isSendingAllowed(const FeedbagItem &item, Feedbag::ModifyType operation)
{
	FeedbagPrivate *d = feedbag->d.data();
	if (!d->client->isConnected()) {
		warning() << "Trying to send the feedbag item while offline:" << item;
		return false;
	}
	if (operation == Feedbag::Add) {
		quint16 limit = d->limits.value(item.type());
		if (limit > 0 && d->itemsByType.value(item.type()).size() >= limit) {
			warning() << "Limit for feedbag item type" << item.type() << "exceeded";
			return false;
		}
	}
	return true;
}

FeedbagItem::FeedbagItem():
	d(0)
{
}

FeedbagItem::FeedbagItem(Feedbag *feedbag, quint16 type, quint16 itemId, quint16 groupId, const QString &name):
	d(new FeedbagItemPrivate(feedbag, type, itemId, groupId, name))
{
}

FeedbagItem::FeedbagItem(FeedbagItemPrivate *d):
	d(d)
{
}

FeedbagItem::FeedbagItem(const FeedbagItem &item) :
	d(item.d)
{
}

FeedbagItem::~FeedbagItem()
{
}

const FeedbagItem &FeedbagItem::operator=(const FeedbagItem &item)
{
	d = item.d;
	return *this;
}

void FeedbagItem::add()
{
	Q_ASSERT(!isInList());
	const FeedbagItem &item = *this;
	d->send(item, Feedbag::Add);
	d->isInList = true;
}

void FeedbagItem::update()
{
	Q_ASSERT(isInList());
	const FeedbagItem &item = *this;
	d->send(item, Feedbag::Modify);
	d->isInList = true;
}

void FeedbagItem::updateOrAdd()
{
	const FeedbagItem &item = *this;
	Feedbag::ModifyType op = d->isInList ? Feedbag::Modify : Feedbag::Add;
	d->send(item, op);
	d->isInList = true;
}

void FeedbagItem::remove()
{
	Q_ASSERT(isInList());
	const FeedbagItem &item = *this;
	d->send(item, Feedbag::Remove);
	d->isInList = false;
}

void FeedbagItem::clear()
{
	d = QSharedDataPointer<FeedbagItemPrivate>();
}

Feedbag *FeedbagItem::feedbag() const
{
	return d->feedbag;
}

bool FeedbagItem::isInList() const
{
	return !isNull() && d->isInList;
}

void FeedbagItem::setInList(bool inList)
{
	d->isInList = inList;
}

bool FeedbagItem::isEmpty() const
{
	return isNull();
}

bool FeedbagItem::isNull() const
{
	return d.data() == 0;
}

void FeedbagItem::setName(const QString &name)
{
	d->recordName = name;
}

void FeedbagItem::setId(quint16 itemId)
{
	d->itemId = itemId;
}

void FeedbagItem::setGroup(quint16 groupId)
{
	d->groupId = groupId;
}

void FeedbagItem::setField(quint16 field)
{
	d->tlvs.insert(field);
}

void FeedbagItem::setField(const TLV &tlv)
{
	d->tlvs.insert(tlv);
}

bool FeedbagItem::removeField(quint16 field)
{
	return d->tlvs.remove(field) > 0;
}

QString FeedbagItem::name() const
{
	return d->recordName;
}

quint16 FeedbagItem::type() const
{
	return d->itemType;
}

quint16 FeedbagItem::itemId() const
{
	return d->itemId;
}

quint16 FeedbagItem::groupId() const
{
	return d->groupId;
}

QPair<quint16, quint16> FeedbagItem::pairId() const
{
	return qMakePair(d->itemType, d->id());
}

QPair<quint16, QString> FeedbagItem::pairName() const
{
	return qMakePair(d->itemType, getCompressedName(d->itemType, d->recordName));
}

TLV FeedbagItem::field(quint16 field) const
{
	return d->tlvs.value(field);
}

bool FeedbagItem::containsField(quint16 field) const
{
	return d->tlvs.contains(field);
}

TLVMap &FeedbagItem::data()
{
	return d->tlvs;
}

const TLVMap &FeedbagItem::constData() const
{
	return d->tlvs;
}

void FeedbagItem::setData(const TLVMap &tlvs)
{
	d->tlvs = tlvs;
}

bool FeedbagItem::operator==(const FeedbagItem &rhs) const
{
	const FeedbagItemPrivate *rd = rhs.d;
	return (d->itemType == rd->itemType &&
			d->itemId == rd->itemId &&
			d->groupId == rd->groupId);
}

QDataStream &operator<<(QDataStream &out, const FeedbagItem &item)
{
	out << item.d->recordName << item.d->itemId
		<< item.d->groupId << item.d->itemType
		<< item.d->tlvs.count();
	TLVMap::const_iterator itr = item.d->tlvs.constBegin();
	TLVMap::const_iterator endItr = item.d->tlvs.constEnd();
	while (itr != endItr) {
		out << itr.key() << itr.value().data();
		++itr;
	}
	return out;
}

QDataStream &operator>>(QDataStream &in, FeedbagItem &item)
{
	item.d = new FeedbagItemPrivate();
	in >> item.d->recordName >> item.d->itemId >> item.d->groupId >> item.d->itemType;
	int count;
	in >> count;
	for (int i = 0; i < count; ++i) {
		quint16 id;
		QByteArray data;
		in >> id >> data;
		item.d->tlvs.insert(id, data);
	}
	return in;
}

QDebug &operator<<(QDebug &stream, Feedbag::ModifyType type)
{
	switch (type) {
	case Feedbag::Add:
		stream.nospace() << "Feedbag::Add";
		break;
	case Feedbag::Modify:
		stream.nospace() << "Feedbag::Modify";
		break;
	case Feedbag::Remove:
		stream.nospace() << "Feedbag::Remove";
		break;
	default:
		stream.nospace() << "Feedbag::UnknownType";
		break;
	}
	return stream;
}

static bool handlerLessThan(FeedbagItemHandler *lhs, FeedbagItemHandler *rhs)
{
	return lhs->priority() > rhs->priority();
}

QList<FeedbagItemHandler*> FeedbagPrivate::handlersForItem(const FeedbagItem &item)
{
	QList<FeedbagItemHandler*> suitableHandlers = handlers.values(item.type());
	if (suitableHandlers.count() > 1)
		qSort(suitableHandlers.begin(), suitableHandlers.end(), &handlerLessThan);
	return suitableHandlers;
}

void FeedbagPrivate::handleItem(FeedbagItem &item, Feedbag::ModifyType type, FeedbagError error)
{
	Q_Q(Feedbag);
	if (!handlers.contains(item.type())) {
		debug() << "The feedbag item ignored:" << item;
		return;
	}
	const QPair<quint16, quint16> id = item.pairId();
	const bool hasError = (error.code() != FeedbagError::NoError);

	if (hasError)
		item.d->isInList = type == Feedbag::Remove;
	else
		item.d->isInList = type != Feedbag::Remove;

	// If a group has been removed, remove its subitems first
	if (type == Feedbag::Remove && item.type() == SsiGroup && !hasError) {
		foreach (FeedbagItem subitem, itemsById) {
			if (subitem.type() != SsiGroup && subitem.groupId() == item.groupId())
				handleItem(subitem, type, error);
		}
	}

	// Handle the item.
	bool found = false;
	foreach (FeedbagItemHandler *handler, handlersForItem(item))
		found |= handler->handleFeedbagItem(q, item, type, error);
	if (!found) {
		if (error == FeedbagError::NoError) {
			if (type == Feedbag::Remove) {
				debug(DebugVerbose) << "The feedbag item has been removed:" << item;
			} else if (type == Feedbag::Modify) {
				debug(DebugVerbose) << "The feedbag item has been updated:" << item;
			} else {
				debug(DebugVerbose) << "The feedbag item has been added:" << item;
			}
		} else {
			if (type == Feedbag::Remove) {
				debug(DebugVerbose).nospace() << "The feedbag item has not been removed: "
										 << error.errorString() << ". (" << error.code() << ")" << item;
			} else if (type == Feedbag::Modify) {
				debug(DebugVerbose) << "The feedbag item has not been updated:"
							   << error.errorString() << ". (" << error.code() << ")" << item;
			} else {
				debug(DebugVerbose) << "The feedbag item has not been added:"
							   << error.errorString() << ". (" << error.code() << ")" << item;
			}
		}
	}

	// Store the item into local copy.
	// Notice that some handlers (like roster) relying on the operation
	// being performed after the handlers are called.
	if (!hasError) {
		if (type == Feedbag::Remove) {
			itemsById.remove(id);
			if (item.type() == SsiGroup) {
				root.regulars.remove(item.groupId());
			} else {
				FeedbagGroup *group = findGroup(item.groupId());
				Q_ASSERT(!group->item.isNull());
				group->hashByName.remove(item.pairName());
			}
			emit q->itemRemoved(item);
		} else {
			itemsById.insert(id, item);
			FeedbagGroup *group = findGroup(item.groupId());
			if (item.type() == SsiGroup) {
				group->item = item;
				root.hashByName.insert(item.pairName(), item.groupId());
			} else {
				Q_ASSERT(!group->item.isNull());
				group->hashByName.insert(item.pairName(), item.itemId());
			}
			if (type == Feedbag::Modify)
				q->itemUpdated(item);
			else
				q->itemAdded(item);
		}
	}
}

FeedbagGroup *FeedbagPrivate::findGroup(quint16 id)
{
	if (id == 0)
		return &root;
	return &root.regulars[id];
}

quint16 FeedbagPrivate::generateId() const
{
	return rand() & 0x7fff; //0x03e6;
}

void FeedbagPrivate::finishLoading()
{
	Q_Q(Feedbag);
	SNAC snac(ListsFamily, ListsGotList);
	client->send(snac); // Roster ack
	emit q->loaded();
}

QEvent::Type FeedbagPrivate::updateEvent()
{
	static QEvent::Type type = static_cast<QEvent::Type>(QEvent::registerEventType());
	return type;
}

FeedbagItemPrivate *FeedbagPrivate::getFeedbagItemPrivate(const SNAC &snac)
{
	QString recordName = snac.read<QString, quint16>(Util::utf8Codec());
	quint16 groupId = snac.read<quint16>();
	quint16 itemId = snac.read<quint16>();
	quint16 itemType = snac.read<quint16>();
	if (!handlers.contains(itemType)) {
		// TODO: add better debugging.
		debug() << "The feedbag item ignored with type" << itemType << "and name" << recordName;
		snac.skipData(snac.read<quint16>());
		return 0;
	}
	FeedbagItemPrivate *item = new FeedbagItemPrivate(q_func(), itemType, itemId, groupId, recordName);
	item->tlvs = snac.read<DataUnit, quint16>().read<TLVMap>();
	return item;
}

void FeedbagPrivate::updateList()
{
	if (modifyQueue.isEmpty())
		return;
	temporaryBuddies.clear();
	client->sendSnac(ListsFamily, ListsCliModifyStart);
	SNAC snac;
	QList<FeedbagQueueItem> items;
	debug() << "Trying to change" << modifyQueue.size() << "items:";
	for (int i = 0; i <= modifyQueue.size(); ++i) {
		const FeedbagQueueItem *item = i < modifyQueue.size() ? &modifyQueue.at(i) : 0;
		if (item)
			debug() << item->type << item->item;
		QByteArray data = item ? item->item.d->data(item->type) : QByteArray();
		if (!item || item->type != snac.subtype() || !snac.canAppend(data.size())) {
			if (!items.isEmpty()) {
				itemsForRequests.append(items);
				items.clear();
				client->send(snac);
			}
			if (item) {
				snac = SNAC(ListsFamily, item->type);
				items.append(*item);
			}
		}
		snac.append(data);
	}
	client->sendSnac(ListsFamily, ListsCliModifyEnd);
	modifyQueue.clear();
}

void FeedbagPrivate::updateLocalCache()
{
	QList<FeedbagItem> upToDateItems;
	qSwap(newItems, upToDateItems);
	AllItemsHash allItems;
	allItems = itemsById;
	itemsById.reserve(upToDateItems.size());
	const FeedbagError noError(FeedbagError::NoError);
	foreach (FeedbagItem item, upToDateItems) {
		const QPair<quint16, quint16> id = item.pairId();
		FeedbagItem oldItem = allItems.take(id);
		Feedbag::ModifyType type = oldItem.isNull() ? Feedbag::Add : Feedbag::Modify;
		handleItem(item, type, noError);
	}
	for (AllItemsHash::Iterator it = allItems.begin(); it != allItems.end(); ++it)
		handleItem(it.value(), Feedbag::Remove, noError);
	newItems.clear();
}

Feedbag::Feedbag(Client *client):
	QObject(client), d(new FeedbagPrivate(client, this))
{
	//qRegisterMetaTypeStreamOperators<Ireen::FeedbagItem>("Ireen::FeedbagItem");

	m_infos << SNACInfo(ListsFamily, ListsError)
			<< SNACInfo(ServiceFamily, ServiceServerAsksServices)
			<< SNACInfo(ListsFamily, ListsUpToDate)
			<< SNACInfo(ListsFamily, ListsList)
			<< SNACInfo(ListsFamily, ListsUpdateGroup)
			<< SNACInfo(ListsFamily, ListsAddToList)
			<< SNACInfo(ListsFamily, ListsRemoveFromList)
			<< SNACInfo(ListsFamily, ListsAck)
			<< SNACInfo(ListsFamily, ListsCliModifyStart)
			<< SNACInfo(ListsFamily, ListsCliModifyEnd)
			<< SNACInfo(ListsFamily, ListsSrvReplyLists);
	QList<SNACInfo> m_initSnacs;
	m_initSnacs << SNACInfo(ListsFamily, ListsCliCheck)
			<< SNACInfo(ListsFamily, ListsCliRequest)
			<< SNACInfo(ListsFamily, ListsCliReqLists)
			<< SNACInfo(ListsFamily, ListsGotList);
	client->registerInitializationSnacs(m_initSnacs);

	connect(client, SIGNAL(disconnected()), SLOT(onDisconnected()));
	client->d_func()->setFeedbag(this);
}

Feedbag::~Feedbag()
{
}

void Feedbag::setCache(const QList<FeedbagItem> &cache)
{
	Q_ASSERT(d->itemsById.isEmpty());
	foreach (FeedbagItem item, cache) {
		item.d->feedbag = this;
		item.d->isInList = true;
		d->itemsById.insert(item.pairId(), item);
		d->itemsByType[item.type()].insert(item.d->id());
		FeedbagGroup *group = d->findGroup(item.groupId());
		if (item.type() == SsiGroup) {
			group->item = item;
			d->root.hashByName.insert(item.pairName(), item.groupId());
			foreach (FeedbagItemHandler *handler, d->handlersForItem(item))
				handler->handleFeedbagItem(this, item, Add, FeedbagError::NoError);
		} else {
			group->hashByName.insert(item.pairName(), item.itemId());
		}
	}

	foreach (const FeedbagItem &item, d->itemsById) {
		if (item.type() == SsiGroup)
			continue;
		foreach (FeedbagItemHandler *handler, d->handlersForItem(item))
			handler->handleFeedbagItem(this, item, Add, FeedbagError::NoError);
	}
}

QList<FeedbagItem> Feedbag::allItems() const
{
	return d->itemsById.values();
}

bool Feedbag::event(QEvent *ev)
{
	if (ev->type() == FeedbagPrivate::updateEvent()) {
		d->updateList();
		return true;
	}
	return QObject::event(ev);
}

bool Feedbag::removeItem(quint16 type, quint16 id)
{
	FeedbagItem item = d->itemsById.value(qMakePair(type, id));
	if (!item.isNull())
		item.remove();
	return !item.isNull();
}

bool Feedbag::removeItem(quint16 type, const QString &name)
{
	QList<FeedbagItem> list = items(type, name);
	foreach (FeedbagItem item, list)
		item.remove();
	return !list.isEmpty();
}

FeedbagItem Feedbag::buddyForChange(const QString &uin) const
{
	QString uiniqueName = getCompressedName(SsiBuddy, uin);
	FeedbagItem item = d->temporaryBuddies.value(uiniqueName);
	if (item.isNull())
		item = items(SsiBuddy, uiniqueName, GenerateId).value(0);
	return item;
}

FeedbagItem Feedbag::itemByType(quint16 type, ItemLoadFlags flags) const
{
	QSet<quint16> &ids = d->itemsByType[type];
	if (!ids.isEmpty()) {
		FeedbagItem item = d->itemsById.value(qMakePair(type, *ids.begin()));
		Q_ASSERT(!item.isNull());
		return item;
	}
	if (flags & CreateItem) {
		quint16 id = 0;
		if (flags & GenerateId)
			id = uniqueItemId(type);
		return FeedbagItem(const_cast<Feedbag*>(this), type,
						   type == SsiGroup ? 0 : id,
						   type == SsiGroup ? id : 0, "");
	}
	return FeedbagItem();
}

FeedbagItem Feedbag::item(quint16 type, quint16 id, quint16 group, ItemLoadFlags flags) const
{
	if (!(flags & DontLoadLocal)) {
		FeedbagItem item = d->itemsById.value(qMakePair(type, id));
		if (!item.isNull())
			return item;
	}
	if (flags & CreateItem) {
		if (flags & GenerateId)
			id = uniqueItemId(type);
		return FeedbagItem(const_cast<Feedbag*>(this), type,
						   type == SsiGroup ? 0 : id,
						   type == SsiGroup ? id : group, "");
	}
	return FeedbagItem();
}

FeedbagItem Feedbag::item(quint16 type, const QString &name, quint16 group, ItemLoadFlags flags) const
{
	QString uniqueName = getCompressedName(type, name);
	if (!(flags & DontLoadLocal)) {
		FeedbagGroup *groupStruct = d->findGroup(type == SsiBuddy ? group : 0);
		ItemsNameHash::ConstIterator it = groupStruct->hashByName.constFind(qMakePair(type, uniqueName));
		if (it != groupStruct->hashByName.constEnd()) {
			const quint16 id = it.value();
			FeedbagItem item = d->itemsById.value(qMakePair(type, id));
			if (!item.isNull())
				return item;
		}
	}
	if (flags & CreateItem) {
		return FeedbagItem(const_cast<Feedbag*>(this), type,
						   type != SsiGroup ? uniqueItemId(type) : 0,
						   type == SsiGroup ? uniqueItemId(type) : group,
						   name);
	}
	return FeedbagItem();
}

FeedbagItem Feedbag::item(quint16 type, const QString &name, ItemLoadFlags flags) const
{
	return items(type, name, flags | ReturnOne).value(0);
}

QList<FeedbagItem> Feedbag::items(quint16 type, const QString &name, ItemLoadFlags flags) const
{
	QList<FeedbagItem> items;
	const QString uniqueName = getCompressedName(type, name);
	if (!(flags & DontLoadLocal)) {
		if (type == SsiBuddy) {
			for (GroupHash::Iterator it = d->root.regulars.begin();
				 it != d->root.regulars.end(); ++it) {
				ItemsNameHash::ConstIterator jt = it.value().hashByName.constFind(qMakePair(type, uniqueName));
				if (jt != it.value().hashByName.constEnd()) {
					const quint16 id = jt.value();
					FeedbagItem item = d->itemsById.value(qMakePair(type, id));
					if (!item.isNull()) {
						items << item;
						if (flags & ReturnOne)
							return items;
					}
				}
			}
		} else {
			ItemsNameHash::ConstIterator it = d->root.hashByName.constFind(qMakePair(type, uniqueName));
			if (it != d->root.hashByName.constEnd()) {
				const quint16 id = it.value();
				FeedbagItem item = d->itemsById.value(qMakePair(type, id));
				if (!item.isNull()) {
					items << item;
					if (flags & ReturnOne)
						return items;
				}
			}
		}
	}
	if (items.isEmpty() && (flags & CreateItem)) {
		items << FeedbagItem(const_cast<Feedbag*>(this), type,
						   type != SsiGroup ? uniqueItemId(type) : 0,
						   type == SsiGroup ? uniqueItemId(type) : 0,
						   name);
	}
	return items;
}

FeedbagItem Feedbag::groupItem(quint16 id, ItemLoadFlags flags) const
{
	return item(SsiGroup, id, 0, flags | ReturnOne);
}

FeedbagItem Feedbag::groupItem(const QString &name, ItemLoadFlags flags) const
{
	QList<FeedbagItem> list = items(SsiGroup, name, flags | ReturnOne);
	if (list.isEmpty())
		return FeedbagItem();
	return list.first();
}

bool Feedbag::containsItem(quint16 type, quint16 id) const
{
	return d->itemsById.contains(qMakePair(type, id));
}

bool Feedbag::containsItem(quint16 type, const QString &name) const
{
	const QString uniqueName = getCompressedName(type, name);
	if (type == SsiBuddy) {
		for (GroupHash::Iterator it = d->root.regulars.begin();
			 it != d->root.regulars.end(); ++it) {
			if (it.value().hashByName.contains(qMakePair(type, uniqueName)))
				return true;
		}
		return false;
	} else {
		return d->root.hashByName.contains(qMakePair(type, uniqueName));
	}
}

quint16 Feedbag::uniqueItemId(quint16 type) const
{
	forever {
		quint16 id = d->generateId();
		if (d->itemsById.contains(qMakePair(type, id)))
			continue;
		if (type == SsiBuddy) {
			bool ok = true;
			foreach (const FeedbagItem &item, d->temporaryBuddies) {
				if (item.itemId() == id) {
					ok = false;
					break;
				}
			}
			if (!ok)
				continue;
		}
		return id;
	}
}

void Feedbag::registerHandler(FeedbagItemHandler *handler)
{
	const QSet<quint16> &types = handler->types();
	foreach (quint16 type, types)
		d->handlers.insertMulti(type, handler);
	if (types.contains(SsiGroup)) {
		foreach (quint16 id, d->itemsByType.value(SsiGroup)) {
			const FeedbagItem item = d->itemsById.value(qMakePair(quint16(SsiGroup), id));
			if (types.contains(item.type()))
				handler->handleFeedbagItem(this, item, Add, FeedbagError::NoError);
		}
	}
	for (AllItemsHash::Iterator it = d->itemsById.begin(); it != d->itemsById.end(); ++it) {
		const quint16 type = it.key().first;
		if (type != SsiGroup && types.contains(type)) {
			const FeedbagItem &item = it.value();
			handler->handleFeedbagItem(this, item, Add, FeedbagError::NoError);
		}
	}
}

Client *Feedbag::client() const
{
	return d->client;
}

void Feedbag::handleSNAC(AbstractConnection *conn, const SNAC &sn)
{
	Q_ASSERT(conn == d->client);
	switch ((sn.family() << 16) | sn.subtype()) {
	case ListsFamily << 16 | ListsError: {
		 ProtocolError error(sn);
		 debug() << QString("Error (%1, %2): %3")
				 .arg(error.code(), 2, 16)
				 .arg(error.subcode(), 2, 16)
				 .arg(error.errorString());
		 break;
	}
	case ServiceFamily << 16 | ServiceServerAsksServices: {
		// Request server-stored information (SSI) service limitations
		SNAC snac(ListsFamily, ListsCliReqLists);
		snac.appendTLV<quint16>(0x0B, 0x000F); // mimic ICQ 6
		conn->send(snac);

		// Requesting roster
		if (d->lastUpdateTime > 0) {
			snac.reset(ListsFamily, ListsCliCheck);
			snac.append<quint32>(d->lastUpdateTime);
			snac.append<quint16>(d->itemsById.size());
			conn->send(snac);
		} else {
			snac.reset(ListsFamily, ListsCliRequest);
			conn->send(snac);
		}
		break;
	}
	case ListsFamily << 16 | ListsUpToDate: {
		 debug() << "Local contactlist is up to date";
		 d->firstPacket = true;
		 d->finishLoading();
		 break;
	}
	case ListsFamily << 16 | ListsList: { // Server sends contactlist
		if (d->firstPacket) {
			emit reloadingStarted();
			d->limits.clear();
			d->firstPacket = false;
		}
		quint8 version = sn.read<quint8>();
		quint16 count = sn.read<quint16>();
		bool isLast = !(sn.flags() & 0x0001);
		debug() << "SSI: number of entries is" << count << "version is" << version;
		for (uint i = 0; i < count; i++) {
			FeedbagItemPrivate *itemPrivate = d->getFeedbagItemPrivate(sn);
			if (itemPrivate) {
				FeedbagItem item(itemPrivate);
				debug() << "Receive item:" << item;
				d->newItems << item;
			}
		}
		if (isLast) {
			d->firstPacket = true;
			d->lastUpdateTime = sn.read<quint32>();
			d->updateLocalCache();
			d->finishLoading();
		}
		break;
	}
	case ListsFamily << 16 | ListsUpdateGroup: // Server sends contact list updates
	case ListsFamily << 16 | ListsAddToList: // Server sends new items
	case ListsFamily << 16 | ListsRemoveFromList: { // Items have been removed
		while (sn.dataSize() != 0) {
			FeedbagItemPrivate *itemPrivate = d->getFeedbagItemPrivate(sn);
			if (itemPrivate) {
				FeedbagItem item(itemPrivate);
				d->handleItem(item, static_cast<ModifyType>(sn.subtype()), FeedbagError::NoError);
			}
		}
		break;
	}
	case ListsFamily << 16 | ListsAck: {
		while (sn.dataSize() != 0) {
			debug() << "Received with id:" << sn.id();
			QSet<quint16> groups;
			foreach (FeedbagQueueItem operation, d->itemsForRequests.takeFirst()) {
				FeedbagError error(sn);
				d->handleItem(operation.item, operation.type, error);
				if (error.code() == FeedbagError::NoError
						&& operation.item.type() == SsiBuddy
						&& (operation.type == Add || operation.type == Remove)) {
					groups.insert(operation.item.groupId());
				}
			}
			foreach (quint16 group, groups) {
				FeedbagItem item = groupItem(group);
				if (!item.isNull())
					item.update();
			}
		}
		break;
	}
	case ListsFamily << 16 | ListsCliModifyStart:
		debug(DebugVerbose) << "The server has started modification of the contact list";
		break;
	case ListsFamily << 16 | ListsCliModifyEnd:
		debug(DebugVerbose) << "The server has ended modification of the contact list";
		break;
	// Server sends SSI service limitations to client
	case ListsFamily << 16 | ListsSrvReplyLists: {
		TLVMap tlvs = sn.read<TLVMap>();
		if (tlvs.contains(0x04)) {
			DataUnit data = tlvs.value(0x04);
			while (data.dataSize() >= 2)
				d->limits << data.read<quint16>();
		}
		break;
	}
	}
}

void Feedbag::onDisconnected()
{
	d->modifyQueue.clear();
	d->itemsForRequests.clear();
	d->temporaryBuddies.clear();
}

FeedbagItemHandler::~FeedbagItemHandler()
{
}

FeedbagItemHandler::FeedbagItemHandler(quint16 priority) :
	m_priority(priority)
{
}

} // namespace Ireen

QDebug &operator<<(QDebug &stream, const Ireen::FeedbagItem &item)
{
	QString name = qPrintable(item.name());
	if (!name.isEmpty())
		stream.nospace() << "Name: " << name << "; type: ";
	else
		stream.nospace() << "Type: ";
	stream.nospace() << item.type() << "; ";
	if (item.type() != Ireen::SsiGroup)
		stream.nospace() << "item id: " << item.itemId() << "; ";
	stream.nospace() << "group id: " << item.groupId() << " (";
	bool first = true;
	foreach(const Ireen::TLV &tlv, item.constData())
	{
		if (!first)
			stream.nospace() << ", ";
		else
			first = false;
		stream.nospace() << "0x" << hex << tlv.type();
	}
	stream.nospace() << ")";
	return stream;
}

