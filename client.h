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

namespace Ireen {

class SNACHandler;
class SNAC;
class ProtocolNegotiation;
class BuddyPictureHandler;
class Roster;
class Feedbag;
class Md5Login;
class ClientPrivate;
class MD5LoginDataPrivate;
class OAuthLoginDataPrivate;

class IREEN_EXPORT MD5LoginData
{
public:
	MD5LoginData(const QString &password = QString());
	~MD5LoginData();
	MD5LoginData(const MD5LoginData &other);
	MD5LoginData &operator=(const MD5LoginData &other);
	QString password() const;
	void setPassword(const QString &password);
	QString loginServer() const;
	quint16 loginServerPort() const;
	void setLoginServer(const QString &server, quint16 port);
#if IREEN_SSL_SUPPORT
	void setSslMode(bool enableSsl = true);
	bool isSslEnabled() const;
#endif
private:
	QSharedDataPointer<MD5LoginDataPrivate> d;
};

#if IREEN_SSL_SUPPORT

class IREEN_EXPORT OAuthLoginData
{
public:
	OAuthLoginData();
	~OAuthLoginData();
	OAuthLoginData(const OAuthLoginData &other);
	OAuthLoginData &operator=(const OAuthLoginData &other);
	QString developerId() const;
	void setDeveloperId(const QString &developerId);
	QString clientName() const;
	void setClientName(const QString &clientName);
	QString distributionId() const;
	void setDistributionId(const QString &distributionId);
	QString password() const;
	void setPassword(const QString &password);
	void setSslMode(bool enableSsl = true);
	bool isSslEnabled() const;
	int versionMajor() const;
	int versionMinor() const;
	int versionSecMinor() const;
	int versionPatch() const;
	void setVersion(int major, int minor, int secMinor, int patch);
	QVariant lastToken() const;
	void setLastToken(const QVariant &tokenData);
private:
	QSharedDataPointer<OAuthLoginDataPrivate> d;
};

#endif

class IREEN_EXPORT Client: public AbstractConnection
{
	Q_OBJECT
	Q_DECLARE_PRIVATE(Client)
public:
	Client(const QString &uin, QObject *parent);
	void login(const QString &password);
	void login(const MD5LoginData &data);
#if IREEN_SSL_SUPPORT
	void login(const OAuthLoginData &data);
#endif
	void disconnectFromHost(bool force = false);
	QString uin() const;
	void sendStatus(Status status);
	QAbstractSocket::SocketState socketState() const;
	Status status() const;
	bool isConnected();
	void setCapability(const Capability &capability, const QString &type);
	bool removeCapability(const Capability &capability);
	bool removeCapability(const QString &type);
	bool containsCapability(const Capability &capability) const;
	bool containsCapability(const QString &type) const;
	QList<Capability> capabilities() const;
	Feedbag *feedbag() const;
	QTextCodec *asciiCodec() const;
	void setAsciiCodec(QTextCodec *codec);
	QTextCodec *detectCodec() const;
signals:
	void loginFinished();
	void loginTokenUpdated(const QVariant &token);
protected:
	void handleSNAC(AbstractConnection *conn, const SNAC &snac);
private slots:
	void onDisconnect();
	void onError(ConnectionError error);
	void authError(Ireen::AbstractConnection::ConnectionError error);
	void onCookieTimeout();
	void finishLogin();
private:
	void setIdle(bool allow);
	void processNewConnection();
	void processCloseConnection();
private:
	friend class Cookie;
	friend class Md5Login;
	friend class OscarAuth;
	friend class Feedbag;
};

} // namespace Ireen

#endif // IREEN_CONNECTION_H

