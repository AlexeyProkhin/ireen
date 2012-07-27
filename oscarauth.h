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

#ifndef IREEN_OSCARAUTH_H
#define IREEN_OSCARAUTH_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QObjectCleanupHandler>

#include "client_p.h"

class QUrl;

namespace Ireen {

class OscarAuth : public QObject, public AbstractLoginMethod
{
	Q_OBJECT
public:
	enum State
	{
		Invalid,
		AtPasswordRequest,
		AtLogin,
		AtSessionStart,
		AtError
	};

	explicit OscarAuth(Client *client, const OAuthLoginData &loginData);
	virtual ~OscarAuth();
	State state() const { return m_state; }
	QAbstractSocket::SocketState socketState();
	QObject *toObject() { return this; }
public slots:
	void setProxy(const QNetworkProxy &proxy);
	void login();
protected:
	void clientLogin(bool longTerm);
	void startSession(const QByteArray &token, const QByteArray &sessionKey);
signals:
	void stateChanged(Ireen::OscarAuth::State);
	void error(Ireen::AbstractConnection::ConnectionError error);
private slots:
	void onClientLoginFinished();
	void onStartSessionFinished();
	void onSslErrors(const QList<QSslError> &errors);
private:
	QString generateLanguage();
	int version() const;
	QByteArray generateSignature(const QByteArray &method, const QByteArray &sessionSecret, const QUrl &url);
private:
	Client *m_client;
	State m_state;
	OAuthLoginData m_loginData;
	QVariantMap m_tokenData;
	QNetworkAccessManager m_manager;
	QObjectCleanupHandler m_cleanupHandler;
};

} // Ireen

#endif // IREEN_OSCARAUTH_H

