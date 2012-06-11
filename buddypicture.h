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

#ifndef IREEN_BUDDYPICTURE_H
#define IREEN_BUDDYPICTURE_H

#include "client.h"
#include "snachandler.h"
#include "feedbag.h"

namespace Ireen {

class Client;
class BuddyPictureHandlerPrivate;
class StatusItem;
class Roster;

enum IconItemType
{
	miniAvatar = 0x0000, // AIM mini avatar
	staticAvatar = 0x0001, // AIM/ICQ avatar ID/hash (len 5 or 16 bytes)
	iChatOnlineMessage = 0x0002, // iChat online message
	flashAvatar = 0x0008, // ICQ Flash avatar hash (16 bytes)
	itunesLink = 0x0009, // iTunes music store link
	photoAvatar = 0x000c, // ICQ contact photo (16 bytes)
	lastTime = 0x000D, // Last update time of online message
	statusMood = 0x000e // Status mood
};

struct IREEN_EXPORT BuddyPicture
{
	QString hash;
	quint16 id;
	quint16 flags;
};

class IREEN_EXPORT BuddyPictureHandler: public AbstractConnection, public FeedbagItemHandler
{
	Q_OBJECT
	Q_INTERFACES(Ireen::FeedbagItemHandler)
	Q_DECLARE_PRIVATE(BuddyPictureHandler)
public:
	BuddyPictureHandler(Client *client, Roster *roster);
	virtual ~BuddyPictureHandler();
	void requestPicture(const QString &uin, const BuddyPicture &picture);
	void uploadAccountAvatar(const QString &avatar);
signals:
	void avatarReceived(const QString &uin, const QByteArray &hash, const QByteArray &avatar);
	void avatarUpdated(const QString &uin, const BuddyPicture &avatar);
	void avatarRemoved(const QString &uin);
protected:
	void handleSNAC(AbstractConnection *conn, const SNAC &snac);
	void connectToServ(const QString &addr, quint16 port, const QByteArray &cookie);
	void processNewConnection();
	void processCloseConnection();
	bool handleFeedbagItem(Feedbag *feedbag, const FeedbagItem &item, Feedbag::ModifyType type, FeedbagError error);
	void onDisconnect();
private slots:
	void statusChanged(const QString &uin, const Ireen::StatusItem &status);
private:
	void updateAvatar(const QString &uin, const QString &hash, quint16 id, quint16 flags);
};

} // namespace Ireen

#endif // IREEN_BUDDYPICTURE_H

