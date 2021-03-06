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

#ifndef IREEN_MD5LOGIN_H
#define IREEN_MD5LOGIN_H

#include "client_p.h"
#include <QHostInfo>

namespace Ireen {

class Md5Login : public AbstractConnection, public AbstractLoginMethod
{
	Q_OBJECT
public:
	Md5Login(Client *client, const MD5LoginData &loginData);
	virtual ~Md5Login();
	QAbstractSocket::SocketState socketState() { return AbstractConnection::socket()->state(); }
	QObject *toObject() { return this; }
public slots:
	void login();
#if IREEN_SSL_SUPPORT
	void sslLogin();
#endif
protected:
	virtual void processNewConnection();
	virtual void processCloseConnection();
	virtual void handleSNAC(AbstractConnection *conn, const SNAC &snac);
private slots:
	void hostFound(const QHostInfo &host);
private:
#if IREEN_SSL_SUPPORT
	bool useSsl;
#endif
	quint16 m_bossPort;
	MD5LoginData m_loginData;
	QByteArray m_bossAddr;
	QByteArray m_cookie;
	Client *m_client;
	int m_hostReqId;
};

} // namespace Ireen

#endif // IREEN_MD5LOGIN_H

