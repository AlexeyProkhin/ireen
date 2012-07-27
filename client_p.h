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

#ifndef IREEN_CLIENT_P_H
#define IREEN_CLIENT_P_H

#include "client.h"
#include "abstractconnection_p.h"
#include "capability.h"
#include "feedbag.h"
#include <k8json/k8json.h>

namespace Ireen {

class DetectCodec : public QTextCodec
{
public:
	inline DetectCodec(QTextCodec **asciiCodec) : m_asciiCodec(asciiCodec) {}
protected:
	QByteArray name() const { return "ireen-detect-codec"; }

	QString convertToUnicode(const char *chars, int len, ConverterState *state) const
	{
		if (K8JSON::isValidUtf8(reinterpret_cast<const uchar*>(chars), len, false))
			return Util::utf8Codec()->toUnicode(chars, len, state);
		else
			return (*m_asciiCodec)->toUnicode(chars, len, state);
	}

	QByteArray convertFromUnicode(const QChar *input, int number, ConverterState *state) const
	{
		return Util::utf8Codec()->fromUnicode(input, number, state);
	}

	int mibEnum() const { return 0; }
private:
	QTextCodec **m_asciiCodec;
};

class MD5LoginDataPrivate : public QSharedData
{
public:
#if IREEN_SSL_SUPPORT
	bool ssl;
#endif
	QString password;
	QString server;
	quint16 port;
};

#if IREEN_SSL_SUPPORT

class OAuthLoginDataPrivate : public QSharedData
{
public:
	bool ssl;
	int versionMajor;
	int versionMinor;
	int versionSecMinor;
	int versionPatch;
	QString password;
	QString devId;
	QString clienName;
	QString distId;
	QVariant lastToken;
};

#endif

class AbstractLoginMethod
{
public:
	virtual void login() = 0;
	virtual QAbstractSocket::SocketState socketState() = 0;
	virtual QObject *toObject() = 0;
	QString errorString() const { return m_errorString; }
protected:
	QString m_errorString;
};

class ClientPrivate : public AbstractConnectionPrivate
{
public:
	ClientPrivate(Client *q_ptr) : q(q_ptr) {}
	void setCapability(const Capability &capability, const QString &type);
	bool removeCapability(const Capability &capability);
	bool removeCapability(const QString &type);
	void finishLogin();
	void connectToBOSS(const QString &host, quint16 port, const QByteArray &cookie);
	void sendUserInfo(bool force = false);
	void setFeedbag(Feedbag *feedbag);
	void login(AbstractLoginMethod *auth);
	bool stopLogin();
public:
	bool isIdle;
	quint16 statusFlags;
	QString uin;
	QHash<quint64, Cookie> cookies;
	Status status;
	QByteArray auth_cookie;
	Capabilities caps;
	QHash<QString, Capability> typedCaps;
	Feedbag *feedbag;
	Client *q;
	QTextCodec *asciiCodec;
	DetectCodec *detectCodec;
	AbstractLoginMethod *auth;
};

} // namespace Ireen

#endif // IREEN_CLIENT_P_H
