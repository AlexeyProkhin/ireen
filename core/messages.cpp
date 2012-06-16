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

#include "messages.h"
#include "buddycaps.h"

namespace Ireen {

using namespace Util;

Channel1MessageData::Channel1MessageData(const QString &message, Channel1Codec charset)
{
	init(fromUnicode(message, charset), charset);
}

Channel1MessageData::Channel1MessageData(const QByteArray &message, Channel1Codec charset)
{
	init(message, charset);
}

QByteArray Channel1MessageData::fromUnicode(const QString &message, Channel1Codec charset)
{
	QTextCodec *codec = 0;
	if (charset == CodecUtf16Be)
		codec = utf16Codec();
	else
		codec = asciiCodec();
	QByteArray data = codec->fromUnicode(message);
	if (charset == CodecUtf16Be)
		data = data.mid(2); // Remove BOM which are shown by some clients as an unknown symbol
	return data;
}

void Channel1MessageData::init(const QByteArray &message, Channel1Codec charset)
{
	DataUnit msgData;
	msgData.append<quint16>(charset);
	msgData.append<quint16>(0);
	msgData.append(message);

	appendTLV(0x0501, (quint32) 0x0106);
	appendTLV(0x0101, msgData.data());
}

Tlv2711::Tlv2711(quint8 msgType, quint8 msgFlags, quint16 X1, quint16 X2, const Cookie &cookie):
	m_cookie(cookie)
{
	setMaxSize(0x1f18);
	append<quint16>(0x1B, LittleEndian);
	append<quint16>(protocol_version, LittleEndian);
	append(ICQ_CAPABILITY_PSIG_MESSAGE);
	append<quint8>(0); // not sure
	append<quint16>(client_features);
	append<quint32>(dc_type);
	append<quint16>(m_cookie.id(), LittleEndian);
	append<quint16>(0x0E, LittleEndian);
	append<quint16>(m_cookie.id(), LittleEndian);
	append<quint64>(0); // Unknown 12 bytes
	append<quint32>(0);
	append<quint8>(msgType);
	append<quint8>(msgFlags);
	append<quint16>(X1, LittleEndian);
	append<quint16>(X2, LittleEndian);
}

void Tlv2711::appendEmptyPacket()
{
	append<quint16>(1, LittleEndian);
	append<quint8>(0);
}

void Tlv2711::appendColors()
{
	append<quint32>(0x00000000); // foreground
	append<quint32>(0x00FFFFFF, LittleEndian); // background
}

Channel2BasicMessageData::Channel2BasicMessageData(quint16 command, const Capability &guid, const Cookie &cookie):
	m_cookie(cookie)
{
	append(command);
	append(m_cookie);
	append(guid);
}

Channel2MessageData::Channel2MessageData(quint16 ackType, const Tlv2711 &data) :
	Channel2BasicMessageData(0, ICQ_CAPABILITY_SRVxRELAY, data.cookie())
{
	init(ackType, data);
}

Channel2MessageData::Channel2MessageData(const QByteArray &message, bool utf8, const Cookie &cookie) :
	Channel2BasicMessageData(0, ICQ_CAPABILITY_SRVxRELAY, cookie)
{
	init(message, utf8, cookie);
}

Channel2MessageData::Channel2MessageData(const QString &message, bool utf8, const Cookie &cookie) :
	Channel2BasicMessageData(0, ICQ_CAPABILITY_SRVxRELAY, cookie)
{
	QTextCodec *codec;
	if (utf8)
		codec = utf8Codec();
	else
		codec = asciiCodec();
	Q_ASSERT(codec);
	init(codec->fromUnicode(message), utf8, cookie);
}

void Channel2MessageData::init(quint16 ackType, const Tlv2711 &data)
{
	appendTLV(0x000A, ackType);
	/*if (includeDcInfo)
	{
		data.appendTLV(0x03, my_ip);
		data.appendTLV(0x05, my_port);
	}*/
	appendTLV(0x000F);
	appendTLV(0x2711, data.data());
}

void Channel2MessageData::init(const QByteArray &message, bool utf8, const Cookie &cookie)
{
	Tlv2711 tlv(0x01, 0, 0 /* contact status ? */, 1, cookie);
	tlv.append<quint16>(message, LittleEndian);
	tlv.appendColors();
	if (utf8)
		tlv.append<quint32>(ICQ_CAPABILITY_UTF8.toString().toUpper(), LittleEndian);
	init(0, tlv);
}

ServerMessage::ServerMessage() :
	SNAC(MessageFamily, MessageSrvSend)
{

}

ServerMessage::ServerMessage(const QString &uin, const Channel1MessageData &data, const Cookie &cookie, bool storeMessage) :
	SNAC(MessageFamily, MessageSrvSend)
{
	init(uin, 1, cookie);
	appendTLV(0x0002, data.data());
	if (storeMessage) {
		// empty TLV(6) store message if recipient offline.
		appendTLV(0x0006);
	}
}

ServerMessage::ServerMessage(const QString &uin, const Channel2BasicMessageData &data) :
	SNAC(MessageFamily, MessageSrvSend)
{
	init(uin, 2, data.cookie());
	appendTLV(0x05, data.data());
}

void ServerMessage::init(const QString &uin, qint16 channel, const Cookie &cookie)
{
	append(cookie); // cookie
	append<quint16>(channel); // message channel
	append<quint8>(uin); // uid or screenname
}

ServerResponseMessage::ServerResponseMessage(const QString &uin, quint16 format,
											 quint16 reason, const Cookie &cookie) :
	SNAC(MessageFamily, MessageResponse)
{
	append(cookie);
	append<quint16>(format);
	append<quint8>(uin);
	append<quint16>(reason);
}

Channel2ResponseMessage::Channel2ResponseMessage(const QString &uin, quint8 type,
												 quint8 flags, const Cookie &cookie) :
	ServerResponseMessage(uin, 2, 3, cookie)
{
	init(type, flags);
}

Channel2ResponseMessage::Channel2ResponseMessage(const QString &uin, const Cookie &cookie) :
	ServerResponseMessage(uin, 2, 3, cookie)
{
	init(MsgPlain, 0);
}

void Channel2ResponseMessage::init(quint8 type, quint8 flags)
{
	Tlv2711 responseTlv(type, flags, 0, 0);
	responseTlv.appendEmptyPacket();
	responseTlv.appendColors();
	append(responseTlv.data());
}

} // namespace Ireen

