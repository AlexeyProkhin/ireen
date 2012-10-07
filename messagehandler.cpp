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

#include "messagehandler.h"
#include "messages.h"
#include "client.h"
#include "buddycaps.h"
#include <QColor>

namespace Ireen {

class MessageHandlerPrivate : public SNACHandler
{
public:
	MessageHandlerPrivate(MessageHandler *q_ptr, Client *client);
	void handleSNAC(AbstractConnection *client, const SNAC &snac);
	void handleMessage(const SNAC &snac);
	void handleResponse(const SNAC &snac);
	QString handleChannel1Message(const QString &uin, const TLVMap &tlvs);
	QString handleChannel2Message(const QString &uin, const TLVMap &tlvs, const Cookie &msgCookie);
	QString handleChannel4Message(const QString &uin, const TLVMap &tlvs);
	QString handleTlv2711(const DataUnit &data, const QString &uin, quint16 ack, const Cookie &msgCookie);
	void sendMetaInfoRequest(quint16 type);
public:
	bool detectCodec;
	QMultiHash<Capability, MessagePlugin *> msg_plugins;
	QMultiHash<Tlv2711Type, Tlv2711Plugin *> tlvs2711Plugins;
	Client *client;
	MessageHandler *q;
};

MessageHandlerPrivate::MessageHandlerPrivate(MessageHandler *q_ptr, Client *client) :
	q(q_ptr)
{
	m_infos << SNACInfo(ServiceFamily, ServiceServerAsksServices)
			<< SNACInfo(MessageFamily, MessageSrvReplyIcbm)
			<< SNACInfo(MessageFamily, MessageResponse)
			<< SNACInfo(MessageFamily, MessageSrvRecv)
			<< SNACInfo(MessageFamily, MessageSrvAck)
			<< SNACInfo(MessageFamily, MessageMtn)
			<< SNACInfo(MessageFamily, MessageSrvError)
			<< SNACInfo(ExtensionsFamily, ExtensionsMetaSrvReply);

	this->client = client;
	detectCodec = true;
	client->registerInitializationSnac(MessageFamily, MessageCliReqIcbm);
	client->registerInitializationSnac(MessageFamily, MessageCliSetParams);
	client->registerHandler(this);

	q->connect(client, SIGNAL(loginFinished()), SLOT(loginFinished()));
}

MessagePlugin::~MessagePlugin()
{
}


Tlv2711Plugin::~Tlv2711Plugin()
{
}

MessageHandler::MessageHandler(Client *client) :
	d(new MessageHandlerPrivate(this, client))
{
}

MessageHandler::~MessageHandler()
{
}

Client *MessageHandler::client() const
{
	return d->client;
}

void MessageHandler::registerHandler(const Capability &capability, MessagePlugin *handler)
{
	d->msg_plugins.insert(capability, handler);
}

void MessageHandler::registerHandler(const Capability &type, quint16 id, Tlv2711Plugin *handler)
{
	d->tlvs2711Plugins.insert(Tlv2711Type(type, id), handler);
}

void MessageHandler::setDetectCodec(bool detectCodec)
{
	d->detectCodec = detectCodec;
}

void MessageHandlerPrivate::handleSNAC(AbstractConnection *conn, const SNAC &sn)
{
	Q_ASSERT(conn == conn);
	switch ((sn.family() << 16) | sn.subtype()) {
	case ServiceFamily << 16 | ServiceServerAsksServices: {
		// Sending CLI_REQICBM
		SNAC snac(MessageFamily, MessageCliReqIcbm);
		conn->send(snac);
		break;
	}
	// Server sends ICBM service parameters to client
	case MessageFamily << 16 | MessageSrvReplyIcbm: {
		quint32 dw_flags = 0x00000303;
		// TODO: Find description
#ifdef DBG_CAPHTML
		dw_flags |= 0x00000400;
#endif
		dw_flags |= 0x00000008; // typing notifications
		// Set message parameters for all channels (imitate ICQ 6)
		SNAC snac(MessageFamily, MessageCliSetParams);
		snac.append<quint16>(0x0000); // Channel
		snac.append<quint32>(dw_flags); // Flags
		snac.append<quint16>(max_message_snac_size); // Max message snac size
		snac.append<quint16>(0x03E7); // Max sender warning level
		snac.append<quint16>(0x03E7); // Max receiver warning level
		snac.append<quint16>(client_rate_limit); // Minimum message interval in seconds
		snac.append<quint16>(0x0000); // Unknown
		conn->send(snac);
		break;
	}
	case MessageFamily << 16 | MessageSrvRecv:
		handleMessage(sn);
		break;
	case MessageFamily << 16 | MessageResponse:
		handleResponse(sn);
		break;
	case MessageFamily << 16 | MessageSrvAck: {
		Cookie cookie = sn.read<Cookie>();
		quint16 channel = sn.read<quint16>();
		QString uin = sn.read<QString, qint8>();
		cookie.setClient(this->client);
		cookie.setUin(uin);
		debug() << QString("Server accepted message for delivery to %1 on channel %2").arg(uin).arg(channel);
		emit q->messageAccepted(cookie, uin, channel);
		break;
	}
	// Typing notifications
	case MessageFamily << 16 | MessageMtn: {
		Cookie cookie = sn.read<Cookie>();
		Q_UNUSED(cookie);
		quint16 channel = sn.read<quint16>();
		Q_UNUSED(channel);
		QString uin = sn.read<QString, qint8>();
		MTN type = MTN(sn.read<quint16>());
		if (type != MtnFinished && type != MtnTyped && type != MtnBegun && type != MtnGone)
			type = MtnUnknown;
		emit q->typingNotification(uin, type);
		break;
	}
	case ExtensionsFamily << 16 | ExtensionsMetaSrvReply: {
		TLVMap tlvs = sn.read<TLVMap>();
		if (tlvs.contains(0x01)) {
			DataUnit data(tlvs.value(0x01));
			data.skipData(6); // skip length field + my uin
			quint16 metaType = data.read<quint16>(LittleEndian);
			switch (metaType) {
			case (0x0041):
				// Offline message.
				// It seems it's not used anymore.
				break;
			case (0x0042):
				// Delete offline messages from the server.
				sendMetaInfoRequest(0x003E);
				break;
			}
		}
		break;
	}
	case MessageFamily << 16 | MessageSrvError: {
		ProtocolError error(sn);
		debug() << QString("Error (%1, %2): %3")
				.arg(error.code(), 2, 16)
				.arg(error.subcode(), 2, 16)
				.arg(error.errorString());
		break;
	}
	}
}

void MessageHandler::loginFinished()
{
	// Offline messages request
	d->sendMetaInfoRequest(0x003C);
}

void MessageHandlerPrivate::handleMessage(const SNAC &snac)
{
	Cookie cookie = snac.read<Cookie>();
	quint16 channel = snac.read<quint16>();
	QString uin = snac.read<QString, quint8>();
	if (uin.isEmpty()) {
		debug() << "Received a broken message packet";
		debug(DebugVeryVerbose) << "The packet:" << snac.data().toHex();
		return;
	}

	cookie.setClient(client);
	cookie.setUin(uin);

	quint16 warning = snac.read<quint16>();
	Q_UNUSED(warning);
	snac.skipData(2); // unused number of tlvs
	TLVMap tlvs = snac.read<TLVMap>();
	QString message;
	switch (channel) {
	case 0x0001: // message
		message = handleChannel1Message(uin, tlvs);
		break;
	case 0x0002: // rendezvous
		message = handleChannel2Message(uin, tlvs, cookie);
		break;
	case 0x0004:
		message = handleChannel4Message(uin, tlvs);
		break;
	default:
		Ireen::warning() << "Unknown message channel:" << channel;
	}

	if (!message.isEmpty()) {
		QDateTime time;
		if (tlvs.contains(0x0016))
			time = QDateTime::fromTime_t(tlvs.value(0x0016).read<quint32>());
		else
			time = QDateTime::currentDateTime();
		emit q->messageReceived(uin, message, time, cookie, channel);
	}
}

void MessageHandlerPrivate::handleResponse(const SNAC &snac)
{
	Cookie cookie = snac.read<Cookie>();
	quint16 format = snac.read<quint16>();
	if (format != 2) {
		debug() << "Unknown response format" << format;
		return;
	}

	QString uin = snac.read<QString, quint8>();
	cookie.setClient(client);
	cookie.setUin(uin);
	snac.skipData(2); //quint16 reason = snac.read<quint16>();
	handleTlv2711(snac, uin, 2, cookie);
}

QString MessageHandlerPrivate::handleChannel1Message(const QString &uin, const TLVMap &tlvs)
{
	QString message;
	if (tlvs.contains(0x0002)) {
		DataUnit data(tlvs.value(0x0002));
		TLVMap msgTlvs = data.read<TLVMap>();
		if (msgTlvs.contains(0x0501))
			debug(DebugVerbose) << "Message has" << msgTlvs.value(0x0501).data().toHex().constData() << "caps";
		foreach(const TLV &tlv, msgTlvs.values(0x0101))
		{
			DataUnit msg_data(tlv);
			quint16 charset = msg_data.read<quint16>();
			quint16 codepage = msg_data.read<quint16>();
			Q_UNUSED(codepage);
			QByteArray data = msg_data.readAll();
			QTextCodec *codec = 0;
			if (charset == CodecUtf16Be)
				codec = Util::utf16Codec();
			else if (detectCodec)
				codec = client->detectCodec();
			else
				codec = client->asciiCodec();
			message += codec->toUnicode(data);
		}
	} else {
		debug() << "Incorrect message on channel 1 from" << uin << ": SNAC should contain TLV 2";
	}
	debug(DebugVerbose) << "New message has been received on channel 1:" << message;
	return message;
}

QString MessageHandlerPrivate::handleChannel2Message(const QString &uin, const TLVMap &tlvs, const Cookie &msgCookie)
{
	if (tlvs.contains(0x0005)) {
		DataUnit data(tlvs.value(0x0005));
		quint16 type = data.read<quint16>();
		data.skipData(8); // again cookie
		Capability guid = data.read<Capability>();
		if (guid.isEmpty()) {
			debug() << "Incorrect message on channel 2 from" << uin << ": guid is not found";
			return QString();
		}
		if (guid == ICQ_CAPABILITY_SRVxRELAY) {
			if (type == 1) {
				debug() << "Abort messages on channel 2 is ignored";
				return QString();
			}
			TLVMap tlvs = data.read<TLVMap>();
			quint16 ack = tlvs.value(0x0A).read<quint16>();
			if (tlvs.contains(0x2711)) {
				DataUnit data(tlvs.value(0x2711));
				return handleTlv2711(data, uin, ack, msgCookie);
			} else
				debug() << "Message on channel 2 should contain TLV 2711";
		} else {
			QList<MessagePlugin *> plugins = msg_plugins.values(guid);
			if (!plugins.isEmpty()) {
				QByteArray plugin_data = data.readAll();
				for (int i = 0; i < plugins.size(); i++)
					plugins.at(i)->processMessage(uin, guid, plugin_data, type, msgCookie);
			} else {
				debug() << IMPLEMENT_ME
						<< QString("Message (channel 2) from %1 with type %2 and guid %3 is not processed.")
						.arg(uin)
						.arg(type)
						.arg(guid.toString());
			}
		}
	} else
		debug() << "Incorrect message on channel 2 from" << uin << ": SNAC should contain TLV 5";
	return QString();
}

QString MessageHandlerPrivate::handleChannel4Message(const QString &uin, const TLVMap &tlvs)
{
	// TODO: Understand this holy shit
	if (tlvs.contains(0x0005)) {
		DataUnit data(tlvs.value(0x0005));
		quint32 uin_2 = data.read<quint32>(LittleEndian);
		if (QString::number(uin_2) != uin)
			return QString();
		quint8 type = data.read<quint8>();
		quint8 flags = data.read<quint8>();
		QByteArray msg_data = data.read<QByteArray, quint16>(LittleEndian);
		Q_UNUSED(flags);
		Q_UNUSED(msg_data);
		debug() << IMPLEMENT_ME << QString("Message (channel 3) from %1 with type %2 is not processed.").arg(uin).arg(type);
	} else
		debug() << "Incorrect message on channel 4 from" << uin << ": SNAC should contain TLV 5";
	return QString();
}

QString MessageHandlerPrivate::handleTlv2711(const DataUnit &data, const QString &uin, quint16 ack, const Cookie &msgCookie)
{
	if (ack == 2 && !msgCookie.unlock()) {
		debug().nospace() << "Skipped unexpected response message with cookie " << msgCookie.id();
		return QString();
	}
	quint16 id = data.read<quint16>(LittleEndian);
	if (id != 0x1B) {
		debug() << "Unknown message id in TLV 2711";
		return QString();
	}
	quint16 version = data.read<quint16>(LittleEndian);
	Q_UNUSED(version); // TODO: what is it?

	Capability guid = data.read<Capability>();
	data.skipData(9);
	id = data.read<quint16>(LittleEndian);
	quint16 cookie = data.read<quint16>(LittleEndian);
	Q_UNUSED(cookie);
	if (guid == ICQ_CAPABILITY_PSIG_MESSAGE) {
		data.skipData(12);
		quint8 type = data.read<quint8>();
		quint8 flags = data.read<quint8>();
		Q_UNUSED(flags);
		quint16 status = data.read<quint16>(LittleEndian);
		quint16 priority = data.read<quint16>(LittleEndian);
		Q_UNUSED(status);
		Q_UNUSED(priority);

		if (type == MsgPlain && ack != 2) // Plain message
		{
			QByteArray message_data = data.read<QByteArray, quint16>(LittleEndian);
			message_data.resize(message_data.size() - 1);
			QColor foreground(data.read<quint8>(),
							  data.read<quint8>(),
							  data.read<quint8>(),
							  data.read<quint8>());
			QColor background(data.read<quint8>(),
							  data.read<quint8>(),
							  data.read<quint8>(),
							  data.read<quint8>());
			Q_UNUSED(foreground);
			Q_UNUSED(background);
			QTextCodec *codec = NULL;
			while (data.dataSize() > 0) {
				QString guid = data.read<QString, quint32>(LittleEndian);
				if (!detectCodec) {
					if (guid.compare(ICQ_CAPABILITY_UTF8.toString(), Qt::CaseInsensitive) == 0) {
						codec = Util::utf8Codec();
					}
				}
				if (guid.compare(ICQ_CAPABILITY_RTFxMSGS.toString(), Qt::CaseInsensitive) == 0) {
					debug() << "RTF is not supported";
					return QString();
				}
			}
			if (codec == NULL) {
				if (detectCodec)
					codec = client->detectCodec();
				else
					codec = client->asciiCodec();
			}
			QString message = codec->toUnicode(message_data);
			debug(DebugVerbose) << "New message has been received on channel 2:" << message;
			return message;
		} else if (MsgPlugin) {
			data.skipData(3);
			DataUnit info = data.read<DataUnit, quint16>(LittleEndian);
			Capability pluginType = info.read<Capability>();
			quint16 pluginId = info.read<quint16>(LittleEndian);
			QString pluginName = info.read<QString, quint32>(LittleEndian);
			DataUnit pluginData = data.read<DataUnit, quint32>(LittleEndian);
			if (pluginType.isNull()) {
				if (ack == 2) {
					debug() << "Message with id" << msgCookie.id() << "has been delivered";
					emit q->messageDelivered(msgCookie, uin);
				}
			} else {
				bool found = false;
				foreach (Tlv2711Plugin *plugin, tlvs2711Plugins.values(Tlv2711Type(pluginType, pluginId))) {
					plugin->processTlvs2711(uin, pluginType, pluginId, pluginData, msgCookie);
					found = true;
				}
				if (!found) {
					debug() << "Unhandled plugin message" << pluginType.toString()
							<< pluginId << pluginName << pluginData.data().toHex();
				}
			}
		} else
			debug() << "Unhandled TLV 2711 message with type" << hex << type;
	} else {
		debug() << "Unknown format of TLV 2711";
	}
	return QString();
}

void MessageHandlerPrivate::sendMetaInfoRequest(quint16 type)
{
	SNAC snac(ExtensionsFamily, ExtensionsMetaCliRequest);
	DataUnit data;
	data.append<quint16>(8, LittleEndian); // data chunk size
	data.append<quint32>(client->uin().toUInt(), LittleEndian);
	data.append<quint16>(type, LittleEndian); // message request cmd
	data.append<quint16>(snac.id()); // request sequence number
	snac.appendTLV(0x01, data);
	client->send(snac);
}

} // namespace Ireen
