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

#include "md5login.h"
#include "util.h"
#include "client_p.h"
#include <QCryptographicHash>
#include <QUrl>
#include <QNetworkProxy>

namespace Ireen {

Md5Login::Md5Login(Client *client, const MD5LoginData &loginData) :
	AbstractConnection(client),
	m_bossPort(0),
	m_loginData(loginData),
	m_client(client),
	m_hostReqId(0)
{
	m_infos.clear();
	m_infos << SNACInfo(AuthorizationFamily, SignonLoginReply)
			<< SNACInfo(AuthorizationFamily, SignonAuthKey);
	registerHandler(this);
	setSeqNum(generateFlapSequence());
	socket()->setProxy(m_client->socket()->proxy());
	QList<SNACInfo> outgoingSnacs;
	outgoingSnacs << SNACInfo(AuthorizationFamily, SignonLoginRequest)
				<< SNACInfo(AuthorizationFamily, SignonAuthRequest);
	registerInitializationSnacs(outgoingSnacs, false);
}

Md5Login::~Md5Login()
{
	if (m_hostReqId)
		QHostInfo::abortHostLookup(m_hostReqId);
}

void Md5Login::login()
{
#if IREEN_SSL_SUPPORT
	if (m_loginData.isSslEnabled()) {
		m_bossAddr.clear();
		m_cookie.clear();
		// Connecting to login server
		Socket *s = socket();
		if (s->state() != QAbstractSocket::UnconnectedState)
			s->abort();
		QString loginServer = m_loginData.loginServer();
		quint16 loginPort   = m_loginData.loginServerPort();
		if (loginServer.isEmpty())
			loginServer = QLatin1String("slogin.icq.com");
		if (loginPort == 0)
			loginPort = 443;
		socket()->connectToHostEncrypted(loginServer, loginPort);
	} else
#endif
	{
		m_bossAddr.clear();
		m_cookie.clear();
		// Connecting to login server
		Socket *s = socket();
		if (s->state() != QAbstractSocket::UnconnectedState)
			s->abort();
		QString loginServer = m_loginData.loginServer();
		if (loginServer.isEmpty())
			loginServer = QLatin1String("login.icq.com");
		m_hostReqId = QHostInfo::lookupHost(loginServer, this, SLOT(hostFound(QHostInfo)));
	}
}

#if IREEN_SSL_SUPPORT
void Md5Login::sslLogin()
{

}
#endif

void Md5Login::hostFound(const QHostInfo &host)
{
	m_hostReqId = 0;
	if (!host.addresses().isEmpty()) {
		quint16 loginPort = m_loginData.loginServerPort();
		if (loginPort == 0)
			loginPort = 5190;
		socket()->connectToHost(host.addresses().at(qrand() % host.addresses().size()), loginPort);
	} else {
		setError(HostNotFound, tr("No IP addresses were found for the host '%1'").arg(m_loginData.loginServer()));
	}
}

void Md5Login::processNewConnection()
{
	AbstractConnection::processNewConnection();
	FLAP flap(0x01);
	flap.append<quint32>(0x00000001);
	// It's some strange unknown shit, but ICQ 6.5 sends it
	flap.appendTLV<quint32>(0x8003, 0x00100000);
	send(flap);

	SNAC snac(AuthorizationFamily, 0x0006);
	snac.appendTLV<QByteArray>(0x0001, m_client->uin().toLatin1());
	send(snac);
}

void Md5Login::processCloseConnection()
{
	AbstractConnection::processCloseConnection();
	if (!m_bossAddr.isEmpty())
		m_client->d_func()->connectToBOSS(m_bossAddr, m_bossPort, m_cookie);
	else
		m_client->disconnectFromHost();
}

void Md5Login::handleSNAC(AbstractConnection *conn, const SNAC &sn)
{
	Q_UNUSED(conn);
	Q_ASSERT(conn == this);
	if (sn.subtype() == SignonAuthKey) {
		const ClientInfo &client = m_client->clientInfo();
		SNAC snac(AuthorizationFamily, SignonLoginRequest);
		snac.setId(qrand());
		snac.appendTLV<QByteArray>(0x0001, m_client->uin().toUtf8());
		{
			quint32 length = qFromBigEndian<quint32>((uchar *) sn.data().constData());
			QByteArray password = m_client->asciiCodec()->fromUnicode(m_loginData.password());
			QByteArray key = sn.data().mid(2, length);
			key += QCryptographicHash::hash(password, QCryptographicHash::Md5);
			key += "AOL Instant Messenger (SM)";
			snac.appendTLV(0x0025, QCryptographicHash::hash(key, QCryptographicHash::Md5));
		}
		// Flag for "new" md5 authorization
		snac.appendTLV(0x004c);
		snac.appendTLV<QByteArray>(0x0003, client.id_string);
		snac.appendTLV<quint16>(0x0017, client.major_version);
		snac.appendTLV<quint16>(0x0018, client.minor_version);
		snac.appendTLV<quint16>(0x0019, client.lesser_version);
		snac.appendTLV<quint16>(0x001a, client.build_number);
		snac.appendTLV<quint16>(0x0016, client.id_number);
		snac.appendTLV<quint32>(0x0014, client.distribution_number);
		snac.appendTLV<QByteArray>(0x000f, client.language);
		snac.appendTLV<QByteArray>(0x000e, client.country);
		// Unknown shit
		snac.appendTLV<quint8>(0x0094, 0x00);
		send(snac);
	} else if (sn.subtype() == SignonLoginReply) {
		TLVMap tlvs = sn.read<TLVMap>();
		if (tlvs.contains(0x01) && tlvs.contains(0x05) && tlvs.contains(0x06)) {
			QList<QByteArray> list = tlvs.value(0x05).data().split(':');
			m_bossAddr = list.at(0);
			m_bossPort = list.size() > 1 ? atoi(list.at(1).constData()) : 5190;
			m_cookie = tlvs.value(0x06).data();
		} else {
			DataUnit data(tlvs.value(0x0008));
			setError(static_cast<AbstractConnection::ConnectionError>(data.read<quint16>()));
		}
	}
}

} // namespace Ireen
