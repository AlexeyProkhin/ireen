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

#ifndef IREEN_CONNECTION_H
#define IREEN_CONNECTION_H

#include "ireen_global.h"
#include "abstractconnection.h"
#include "status.h"
#if IREEN_USE_MD5_LOGIN
#include "md5login.h"
#else
#include "oscarauth.h"
#endif

namespace Ireen {

class SNACHandler;
class SNAC;
class ProtocolNegotiation;
class BuddyPictureHandler;
class Roster;
class Feedbag;
class Md5Login;
class ClientPrivate;

class IREEN_EXPORT Client: public AbstractConnection
{
	Q_OBJECT
	Q_DECLARE_PRIVATE(Client)
public:
	Client(const QString &uin, QObject *parent);
	void login(const QString &password);
	void disconnectFromHost(bool force = false);
	QString uin() const;
	void sendStatus(Status status);
	QAbstractSocket::SocketState socketState() const;
	Status status() const;
	bool isConnected();
#if IREEN_USE_MD5_LOGIN
	void setLoginServer(const QString &server, quint16 port);
	QString loginServer() const;
	quint16 loginServerPort() const;
#endif
	void setCapability(const Capability &capability, const QString &type);
	bool removeCapability(const Capability &capability);
	bool removeCapability(const QString &type);
	bool containsCapability(const Capability &capability) const;
	bool containsCapability(const QString &type) const;
	QList<Capability> capabilities() const;
	Feedbag *feedbag() const;
signals:
	void loginFinished();
protected:
	void handleSNAC(AbstractConnection *conn, const SNAC &snac);
private slots:
	void onDisconnect();
	void onError(ConnectionError error);
	void md5Error(Ireen::AbstractConnection::ConnectionError error);
	void onCookieTimeout();
	void finishLogin();
private:
	void setIdle(bool allow);
	void processNewConnection();
	void processCloseConnection();
private:
	friend class Cookie;
	friend class Md5Login;
};

} // namespace Ireen

#endif // IREEN_CONNECTION_H

