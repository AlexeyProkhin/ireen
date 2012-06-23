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

#ifndef IREEN_FEEDBAG_H
#define IREEN_FEEDBAG_H

#include "ireen_global.h"
#include "core/snachandler.h"
#include "core/tlv.h"
#include <QSharedData>

namespace Ireen {

class Feedbag;
class FeedbagPrivate;
class FeedbagItemPrivate;
class FeedbagItemHandler;
class Client;

class IREEN_EXPORT FeedbagError
{
public:
	enum ErrorEnum
	{
		NoError = 0x0000,
		ItemNotFound = 0x0002,
		ItemAlreadyExists = 0x0003,
		CommonError = 0x000a,
		LimitExceeded = 0x000c,
		AttemtToAddIcqContactToAimList = 0x000d,
		RequiresAuthorization = 0x000e
	};
	FeedbagError(ErrorEnum error);
	FeedbagError(const SNAC &snac);
	ErrorEnum code();
	QString errorString();
	bool operator==(ErrorEnum code) { return m_error == code; }
	bool operator!=(ErrorEnum code) { return m_error != code; }
protected:
	ErrorEnum m_error;
};

class IREEN_EXPORT FeedbagItem
{
public:
	FeedbagItem();
	FeedbagItem(Feedbag *feedbag, quint16 type, quint16 itemId, quint16 groupId, const QString &name = QString());
	FeedbagItem(const FeedbagItem &item);
	virtual ~FeedbagItem();
	const FeedbagItem &operator=(const FeedbagItem &item);
	void add();
	void update();
	void updateOrAdd();
	void remove();
	void clear();
	Feedbag *feedbag() const;
	bool isInList() const;
	void setInList(bool inList);
	bool isEmpty() const;
	bool isNull() const;
	void setName(const QString &name);
	void setId(quint16 itemId);
	void setGroup(quint16 groupId);
	void setField(quint16 field);
	void setField(const TLV &tlv);
	template<typename T>
	void setField(quint16 field, const T &data);
	bool removeField(quint16 field);
	QString name() const;
	quint16 type() const;
	quint16 itemId() const;
	quint16 groupId() const;
	QPair<quint16, quint16> pairId() const;
	QPair<quint16, QString> pairName() const;
	TLV field(quint16 field) const;
	template<typename T>
	T field(quint16 field, const T &def = T()) const;
	bool containsField(quint16 field) const;
	TLVMap &data();
	const TLVMap &constData() const;
	void setData(const TLVMap &tlvs);
	bool operator==(const FeedbagItem &rhs) const;
protected:
	FeedbagItem(FeedbagItemPrivate *d);
private:
	friend class Feedbag;
	friend class FeedbagPrivate;
	friend class FeedbagItemPrivate;
	friend QDataStream &operator<<(QDataStream &out, const FeedbagItem &item);
	friend QDataStream &operator>>(QDataStream &in, FeedbagItem &item);
	QSharedDataPointer<FeedbagItemPrivate> d;
};

IREEN_EXPORT QDataStream &operator<<(QDataStream &out, const FeedbagItem &item);
IREEN_EXPORT QDataStream &operator>>(QDataStream &in, FeedbagItem &item);

template<typename T>
Q_INLINE_TEMPLATE T FeedbagItem::field(quint16 f, const T &def) const
{
	return constData().value(f, def);
}

template<typename T>
Q_INLINE_TEMPLATE void FeedbagItem::setField(quint16 field, const T &d)
{
	setField(TLV(field, d));
}

class IREEN_EXPORT Feedbag : public QObject, public SNACHandler
{
	Q_OBJECT
	Q_INTERFACES(Ireen::SNACHandler)
public:
	enum ModifyType {
		Add = ListsAddToList,
		Modify = ListsUpdateGroup,
		Remove =  ListsRemoveFromList
	};
	enum ItemLoadFlag
	{
		NoFlags = 0x0000,
		CreateItem = 0x0001,
		GenerateId = CreateItem | 0x0002,
		DontLoadLocal = 0x0010,
		ReturnOne = 0x0020,
		DontCheckGroup = 0x0040
	};
	Q_DECLARE_FLAGS(ItemLoadFlags, ItemLoadFlag)

	Feedbag(Client *client);
	virtual ~Feedbag();
	void setCache(const QList<FeedbagItem> &cache);
	QList<FeedbagItem> allItems() const;

	bool removeItem(quint16 type, quint16 id);
	bool removeItem(quint16 type, const QString &name);

	FeedbagItem buddyForChange(const QString &uin) const;
	FeedbagItem itemByType(quint16 type, ItemLoadFlags flags = NoFlags) const;

	FeedbagItem item(quint16 type, quint16 id, quint16 group, ItemLoadFlags flags = NoFlags) const;
	FeedbagItem item(quint16 type, const QString &name, quint16 group, ItemLoadFlags flags = NoFlags) const;
	FeedbagItem item(quint16 type, const QString &name, ItemLoadFlags flags = NoFlags) const;

	QList<FeedbagItem> items(quint16 type, const QString &name, ItemLoadFlags flags = NoFlags) const;

	FeedbagItem groupItem(quint16 id, ItemLoadFlags flags = NoFlags) const;
	FeedbagItem groupItem(const QString &name, ItemLoadFlags flags = NoFlags) const;
	bool containsItem(quint16 type, quint16 id) const;
	bool containsItem(quint16 type, const QString &name) const;

	quint16 uniqueItemId(quint16 type) const;

	void registerHandler(FeedbagItemHandler *handler);
	Client *client() const;
signals:
	void loaded();
	void reloadingStarted();
	void itemAdded(const Ireen::FeedbagItem &item);
	void itemUpdated(const Ireen::FeedbagItem &item);
	void itemRemoved(const Ireen::FeedbagItem &item);
protected:
	virtual void handleSNAC(AbstractConnection *conn, const SNAC &snac);
	bool event(QEvent *event);
private slots:
	void onDisconnected();
private:
	friend class FeedbagPrivate;
	friend class FeedbagItem;
	friend class FeedbagItemPrivate;
	QScopedPointer<FeedbagPrivate> d;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Feedbag::ItemLoadFlags)

class IREEN_EXPORT FeedbagItemHandler
{
public:
	virtual ~FeedbagItemHandler();
	const QSet<quint16> &types() { return m_types; }
	virtual bool handleFeedbagItem(Feedbag *feedbag, const FeedbagItem &item, Feedbag::ModifyType type, FeedbagError error) = 0;
	quint16 priority() { return m_priority; }
protected:
	FeedbagItemHandler(quint16 priority = 50);
	QSet<quint16> m_types;
private:
	quint16 m_priority;
};

} // namespace Ireen

IREEN_EXPORT QDebug &operator<<(QDebug &stream, const Ireen::FeedbagItem &item);
IREEN_EXPORT QDebug &operator<<(QDebug &stream, Ireen::Feedbag::ModifyType type);
Q_DECLARE_INTERFACE(Ireen::FeedbagItemHandler, "org.qutim.ireen.FeedbagItemHandler")

#endif // IREEN_FEEDBAG_H

