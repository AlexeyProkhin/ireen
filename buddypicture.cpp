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

#include "buddypicture.h"
#include "sessiondataitem.h"
#include "client.h"
#include "abstractconnection_p.h"
#include "roster.h"
#include <QSet>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QNetworkProxy>
#include <QCryptographicHash>

namespace Ireen {

QByteArray emptyHash = QByteArray::fromHex("0201d20472");

class BuddyPictureHandlerPrivate : public AbstractConnectionPrivate
{
public:
	Client *client;
	QHash<QString, SNAC> deferredRequests;
	QByteArray cookie;
	QByteArray accountAvatar;
	QByteArray avatarHash;
};

BuddyPictureHandler::BuddyPictureHandler(Client *client, Roster *roster) :
	AbstractConnection(new BuddyPictureHandlerPrivate, client)
{
	Q_D(BuddyPictureHandler);
	m_infos << SNACInfo(ServiceFamily, ServerRedirectService)
			<< SNACInfo(ServiceFamily, ServiceServerExtstatus)
			<< SNACInfo(AvatarFamily, AvatarGetReply)
			<< SNACInfo(AvatarFamily, AvatarUploadAck);
	m_types << SsiBuddyIcon;

	d->client = client;

	socket()->setProxy(d->client->socket()->proxy());
	registerHandler(this);
	d->client->registerHandler(this);
	d->client->feedbag()->registerHandler(this);

	connect(roster, SIGNAL(contactStatusUpdated(QString,Ireen::StatusItem)),
			this, SLOT(statusChanged(QString,Ireen::StatusItem)));
}

BuddyPictureHandler::~BuddyPictureHandler()
{
}

void BuddyPictureHandler::requestPicture(const QString &uin, const BuddyPicture &picture)
{
	Q_D(BuddyPictureHandler);
	if (picture.hash == d->avatarHash)
		return; // Don't request pictures that we are currently uploading.
	debug() << "BuddyPicture: request avatar of" << uin;
	SNAC snac(AvatarFamily, AvatarGetRequest);
	snac.append<quint8>(uin);
	snac.append<quint8>(1); // unknown
	snac.append<quint16>(picture.id);
	snac.append<quint8>(picture.flags);
	snac.append<quint8>(picture.hash);
	if (state() == Connected)
		send(snac);
	else
		d->deferredRequests.insert(uin, snac);
}

void BuddyPictureHandler::uploadAccountAvatar(const QString &avatar)
{
	Q_D(BuddyPictureHandler);
	d->accountAvatar.clear();
	QFile image(avatar);
	if (!image.open(QIODevice::ReadOnly))
		return;
	d->accountAvatar = image.read(8178); // TODO: notify user if the image size limit is exceeded
	// Md5 hash.
	d->avatarHash = QCryptographicHash::hash(d->accountAvatar, QCryptographicHash::Md5);
	// Request for update of avatar.
	FeedbagItem item = d->client->feedbag()->itemByType(SsiBuddyIcon, Feedbag::GenerateId);
	TLV data(0x00d5);
	data.append<quint8>(1);
	data.append<quint8>(d->avatarHash);
	item.setField(data);
	if (!item.isInList())
		item.setName("1");
	item.updateOrAdd();
}

void BuddyPictureHandler::handleSNAC(AbstractConnection *conn, const SNAC &snac)
{
	Q_D(BuddyPictureHandler);
	if (this == conn) {
		AbstractConnection::handleSNAC(this, snac);
		snac.resetState();
		if (snac.family() == ServiceFamily && snac.subtype() == ServiceServerAsksServices) {
			SNAC snac(ServiceFamily, ServiceClientReady);
			snac.append(QByteArray::fromHex(
					"0001 0004 0110 164f" // ServiceFamily
					"000f 0001 0110 164f"));// AvatarFamily
			send(snac);
			setState(Connected);
			foreach (SNAC snac, d->deferredRequests)
				send(snac);
			d->deferredRequests.clear();
		}
	} else {
		if (snac.family() == ServiceFamily && snac.subtype() == ServerRedirectService) {
			TLVMap tlvs = snac.read<TLVMap>();
			quint16 id = tlvs.value(0x0D).read<quint16>();
			if (id == AvatarFamily) {
				QList<QByteArray> list = tlvs.value(0x05).data().split(':');
				d->cookie = tlvs.value(0x06).data();
				socket()->connectToHost(list.at(0), list.size() > 1 ? atoi(list.at(1).constData()) : 5190);
			}
		} else if (snac.family() == ServiceFamily && snac.subtype() == ServiceServerAsksServices) {
			// Requesting avatar service
			SNAC snac(ServiceFamily, ServiceClientNewService);
			snac.append<quint16>(AvatarFamily);
			conn->send(snac);
		}
	}
	switch ((snac.family() << 16) | snac.subtype()) {
	case AvatarFamily << 16 | AvatarGetReply: {
		QString uin = snac.read<QString, quint8>();
		snac.skipData(3); // skip iconId and iconFlag
		QByteArray hash = snac.read<QByteArray, quint8>();
		snac.skipData(21);
		QByteArray image = snac.read<QByteArray, quint16>();
		debug() << "BuddyPicture: avatar of" << uin << "received";
		emit avatarReceived(uin, hash, image);
		break;
	}
	case ServiceFamily << 16 | ServiceServerExtstatus: { // account avatar changed
		TLVMap tlvs = snac.read<TLVMap>();
		if (tlvs.contains(0x0200)) {
			TLV tlv = tlvs.value(0x0200);
			quint8 type = tlv.read<quint8>();
			if (type == 0x0001) {
				quint8 flags = tlv.read<quint8>();
				QByteArray hash = tlv.read<QByteArray, quint8>();
				if (flags >> 6 & 0x1 && !d->accountAvatar.isEmpty()) { // does it really work???
					SNAC snac(AvatarFamily, AvatarUploadRequest);
					snac.append<quint16>(1); // reference number ?
					snac.append<quint16>(d->accountAvatar);
					send(snac);
				}

				updateAvatar(d->client->uin(), hash, 1, flags);
			}
		}
		break;
	}
	case AvatarFamily << 16 | AvatarUploadAck: { // avatar uploaded
		snac.skipData(4); // unknown
		QByteArray hash = snac.read<QByteArray, quint8>();
		if (hash == d->avatarHash) {
			debug() << "Account's avatar has been successfully updated";
			emit avatarReceived(d->client->uin(), d->avatarHash, d->accountAvatar);
		} else {
			debug() << "Error occurred when updating account avatar";
		}
		d->avatarHash.clear();
		d->accountAvatar.clear();
		break;
	}
	}
}

void BuddyPictureHandler::processNewConnection()
{
	Q_D(BuddyPictureHandler);
	AbstractConnection::processNewConnection();
	FLAP flap(0x01);
	flap.append<quint32>(0x01);
	flap.appendTLV<QByteArray>(0x0006, d->cookie);
	d->cookie.clear();
	send(flap);
}

void BuddyPictureHandler::processCloseConnection()
{
	AbstractConnection::processCloseConnection();
}

bool BuddyPictureHandler::handleFeedbagItem(Feedbag *feedbag, const FeedbagItem &item,
									 Feedbag::ModifyType type, FeedbagError error)
{
	Q_D(BuddyPictureHandler);
	Q_UNUSED(feedbag);
	Q_ASSERT(item.type() == SsiBuddyIcon);
	if (error != FeedbagError::NoError || type == Feedbag::Remove)
		return false;
	if (d->avatarHash.isEmpty() && item.containsField(0x00d5)) {
		DataUnit data(item.field(0x00d5));
		quint8 flags = data.read<quint8>();
		QByteArray hash = data.read<QByteArray, quint8>();
		updateAvatar(d->client->uin(), hash, 1, flags);
	}
	return true;
}

void BuddyPictureHandler::statusChanged(const QString &uin, const StatusItem &status)
{
	foreach (const SessionDataItem &item, status.statusData()) {
		if (item.type() != staticAvatar && item.type() != miniAvatar &&
			item.type() != flashAvatar && item.type() != photoAvatar)
		{
			continue;
		}
		debug() << "BuddyPicture:" << uin << "updated his/her avatar";
		QByteArray hash = item.readData(16);
		updateAvatar(uin, hash, item.type(), item.flags());
		break;
	}
}

void BuddyPictureHandler::onDisconnect()
{
	Q_D(BuddyPictureHandler);
	d->deferredRequests.clear();
	d->avatarHash.clear();
	d->accountAvatar.clear();
	AbstractConnection::onDisconnect();
}

void BuddyPictureHandler::updateAvatar(const QString &uin, const QByteArray &hash, quint16 id, quint16 flags)
{
	if (hash != emptyHash) {
		BuddyPicture picture = { hash, id, flags };
		emit avatarUpdated(uin, picture);
	} else {
		emit avatarRemoved(uin);
	}
}

} // namespace Ireen

