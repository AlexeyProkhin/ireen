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

#include "abstractconnection_p.h"
#include "capability.h"
#include "feedbag.h"

#if IREEN_USE_MD5_LOGIN
#include "md5login.h"
#else
#include "oscarauth.h"
#endif

namespace Ireen {

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
#if IREEN_USE_MD5_LOGIN
	Pointer<Md5Login> auth;
	QString loginServer;
	quint16 loginServerPort;
#else
	Pointer<OscarAuth> auth;
#endif
};

} // namespace Ireen

#endif // IREEN_CLIENT_P_H
