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

#ifndef IREEN_MESSAGEHANDLER_H
#define IREEN_MESSAGEHANDLER_H

#include "core/snachandler.h"
#include "core/messages.h"

#include "core/capability.h"

namespace Ireen {

class Client;
class MessageHandlerPrivate;

class IREEN_EXPORT MessagePlugin
{
public:
	virtual ~MessagePlugin();
	QSet<Capability> capabilities() { return m_capabilities; }
	virtual void processMessage(const QString &uin, const Capability &guid, const QByteArray &data,
								quint16 type, const Cookie &cookie) = 0;
protected:
	QSet<Capability> m_capabilities;
};

typedef QPair<Capability, quint16> Tlv2711Type;
class IREEN_EXPORT Tlv2711Plugin
{
public:
	virtual ~Tlv2711Plugin();
	QSet<Tlv2711Type> tlv2711Types() { return m_tlvs2711Types; }
	virtual void processTlvs2711(const QString &uin, Capability guid, quint16 type,
								 const DataUnit &data, const Cookie &cookie) = 0;
protected:
	QSet<Tlv2711Type> m_tlvs2711Types;
};

class IREEN_EXPORT MessageHandler : public QObject
{
	Q_OBJECT
public:
	MessageHandler(Client *client);
	virtual ~MessageHandler();
	Client *client() const;
	void registerHandler(const Capability &capability, MessagePlugin *handler);
	void registerHandler(const Capability &type, quint16 id, Tlv2711Plugin *handler);
	void setDetectCodec(bool detectCodec = true);
signals:
	// The message identified by cookie has been received by the server and
	// it agreed to pass it to the destination.
	void messageAccepted(const Ireen::Cookie &cookie, const QString &uin, quint16 channel);
	// Received typing notification.
	void typingNotification(const QString &uin, Ireen::MTN newState);
	// The message identified by cookie has been delivered.
	void messageDelivered(const Ireen::Cookie &cookie, const QString &uin);
	// A new message has been received.
	void messageReceived(const QString &uin, const QString &message, const QDateTime &time,
						 const Ireen::Cookie &cookie, quint16 channel);
private slots:
	void loginFinished();
private:
	friend class MessageHandlerPrivate;
	QScopedPointer<MessageHandlerPrivate> d;
};

} // namespace Ireen

Q_DECLARE_INTERFACE(Ireen::Tlv2711Plugin, "org.qutim.ireen.Tlvs2711Plugin")
Q_DECLARE_INTERFACE(Ireen::MessagePlugin, "org.qutim.ireen.MessagePlugin")

#endif // IREEN_MESSAGEHANDLER_H
