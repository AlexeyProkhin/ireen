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

#include "client_p.h"
#include "util.h"
#include "feedbag.h"
#include "buddycaps.h"
#include "messages.h"
#include "sessiondataitem.h"

#include "md5login.h"
#include "oscarauth.h"

#include <QHostInfo>
#include <QBuffer>
#include <QTimer>
#include <QNetworkProxy>
#include <QMetaMethod>

namespace Ireen {

MD5LoginData::MD5LoginData(const QString &password) :
	d(new MD5LoginDataPrivate)
{
	d->password = password;
}

MD5LoginData::~MD5LoginData()
{
}

MD5LoginData::MD5LoginData(const MD5LoginData &other) :
	d(other.d)
{

}

MD5LoginData &MD5LoginData::operator=(const MD5LoginData &other)
{
	d = other.d;
	return *this;
}

QString MD5LoginData::password() const
{
	return d->password;
}

void MD5LoginData::setPassword(const QString &password)
{
	d->password = password;
}

QString MD5LoginData::loginServer() const
{
	return d->server;
}

quint16 MD5LoginData::loginServerPort() const
{
	return d->port;
}

void MD5LoginData::setLoginServer(const QString &server, quint16 port)
{
	d->server = server;
	d->port = port;
}

#if IREEN_SSL_SUPPORT

void MD5LoginData::setSslMode(bool enableSsl)
{
	d->ssl = enableSsl;
}

bool MD5LoginData::isSslEnabled() const
{
	return d->ssl;
}

OAuthLoginData::OAuthLoginData() :
	d(new OAuthLoginDataPrivate)
{
}

OAuthLoginData::~OAuthLoginData()
{
}

OAuthLoginData::OAuthLoginData(const OAuthLoginData &other) :
	d(other.d)
{
}

OAuthLoginData &OAuthLoginData::operator=(const OAuthLoginData &other)
{
	d = other.d;
	return *this;
}

QString OAuthLoginData::developerId() const
{
	return d->devId;
}

void OAuthLoginData::setDeveloperId(const QString &developerId)
{
	d->devId = developerId;
}

QString OAuthLoginData::clientName() const
{
	return d->clienName;
}

void OAuthLoginData::setClientName(const QString &clientName)
{
	d->clienName = clientName;
}

QString OAuthLoginData::distributionId() const
{
	return d->distId;
}

void OAuthLoginData::setDistributionId(const QString &distributionId)
{
	d->distId = distributionId;
}

QString OAuthLoginData::password() const
{
	return d->password;
}

void OAuthLoginData::setPassword(const QString &password)
{
	d->password = password;
}

void OAuthLoginData::setSslMode(bool enableSsl)
{
	d->ssl = enableSsl;
}

bool OAuthLoginData::isSslEnabled() const
{
	return d->ssl;
}

int OAuthLoginData::versionMajor() const
{
	return d->versionMajor;
}

int OAuthLoginData::versionMinor() const
{
	return d->versionMinor;
}

int OAuthLoginData::versionSecMinor() const
{
	return d->versionSecMinor;
}

int OAuthLoginData::versionPatch() const
{
	return d->versionPatch;
}

void OAuthLoginData::setVersion(int major, int minor, int secMinor, int patch)
{
	d->versionMajor = major;
	d->versionMinor = minor;
	d->versionSecMinor = secMinor;
	d->versionPatch = patch;
}

QVariant OAuthLoginData::lastToken() const
{
	return d->lastToken;
}

void OAuthLoginData::setLastToken(const QVariant &tokenData)
{
	d->lastToken = tokenData;
}

#endif

void ClientPrivate::sendUserInfo(bool force)
{
	if (!force && !q->isConnected())
		return;
	SNAC snac(LocationFamily, MessageFamily);
	TLV caps(0x05);
	foreach (const Capability &cap, this->caps)
		caps.append(cap);
	foreach (const Capability &cap, typedCaps)
		caps.append(cap);
	snac.append(caps);
	q->send(snac);
}

void ClientPrivate::connectToBOSS(const QString &host, quint16 port, const QByteArray &cookie)
{
	auth_cookie = cookie;
	if (socket->state() != QAbstractSocket::UnconnectedState)
		socket->abort();
	socket->connectToHost(host, port);
	stopLogin();
}

void ClientPrivate::setFeedbag(Feedbag *feedbag_)
{
	Q_ASSERT(!feedbag);
	q->registerHandler(feedbag = feedbag_);
	q->connect(feedbag, SIGNAL(loaded()), SLOT(finishLogin()));
}

void ClientPrivate::login(AbstractLoginMethod *newAuth)
{
	stopLogin();
	q->setError(Client::NoError);
	QObject *obj = newAuth->toObject();
	q->connect(obj, SIGNAL(error(Ireen::AbstractConnection::ConnectionError)),
			   q, SLOT(authError(Ireen::AbstractConnection::ConnectionError)));
	// Start connecting after the status has been updated.
	QTimer::singleShot(0, obj, SLOT(login()));
	auth = newAuth;
}

bool ClientPrivate::stopLogin()
{
	if (auth) {
		auth->toObject()->deleteLater();
		auth = 0;
		return true;
	}
	return false;
}

Client::Client(const QString &uin, QObject *parent) :
	AbstractConnection(new ClientPrivate(this), parent)
{
	Q_D(Client);
	m_infos << SNACInfo(LocationFamily, LocationRightsReply)
			<< SNACInfo(BosFamily, PrivacyRightsReply);

	d->uin = uin;
	d->statusFlags = 0x0000;
	d->isIdle = false;
	d->feedbag = 0;
	d->asciiCodec = QTextCodec::codecForLocale();
	d->detectCodec = new DetectCodec(&d->asciiCodec);

	registerHandler(this);

	registerInitializationSnac(LocationFamily, LocationCliReqRights);
	registerInitializationSnac(BosFamily, PrivacyReqRights);

	// ICQ UTF8 Support
	d->caps.append(ICQ_CAPABILITY_UTF8);
	// Buddy Icon
	d->caps.append(ICQ_CAPABILITY_AIMICON);
	// RTF messages
	//d->caps.append(ICQ_CAPABILITY_RTFxMSGS);
	// qutIM some shit
	d->caps.append(Capability(0x69716d75, 0x61746769, 0x656d0000, 0x00000000));
	d->caps.append(Capability(0x09461343, 0x4c7f11d1, 0x82224445, 0x53540000));
	// HTML messages
	d->caps.append(ICQ_CAPABILITY_HTMLMSGS);
	// ICQ typing
	d->caps.append(ICQ_CAPABILITY_TYPING);
	// Xtraz
	d->caps.append(ICQ_CAPABILITY_XTRAZ);
	// Messages on channel 2
	d->caps.append(ICQ_CAPABILITY_SRVxRELAY);
	// Short capability support
	d->caps.append(ICQ_CAPABILITY_SHORTCAPS);
}

void Client::login(const QString &password)
{
	login(MD5LoginData(password));
}

void Client::login(const MD5LoginData &data)
{
	d_func()->login(new Md5Login(this, data));
}

#if IREEN_SSL_SUPPORT

void Client::login(const OAuthLoginData &data)
{
	d_func()->login(new OscarAuth(this, data));
}

#endif

QString Client::uin() const
{
	return d_func()->uin;
}

void Client::disconnectFromHost(bool force)
{
	Q_D(Client);
	d->status = Status::Offline;
	if (!d->stopLogin())
		AbstractConnection::disconnectFromHost(force);
}

void Client::processNewConnection()
{
	Q_D(Client);
	AbstractConnection::processNewConnection();
	const ClientInfo &info = clientInfo();
	FLAP flap(0x01);
	flap.append<quint32>(0x01);
	flap.appendTLV<QByteArray>(0x0006, d->auth_cookie);
	flap.appendTLV<QByteArray>(0x0003, info.id_string);
	flap.appendTLV<quint16>(0x0017, info.major_version);
	flap.appendTLV<quint16>(0x0018, info.minor_version);
	flap.appendTLV<quint16>(0x0019, info.lesser_version);
	flap.appendTLV<quint16>(0x001a, info.build_number);
	flap.appendTLV<quint16>(0x0016, info.id_number);
	flap.appendTLV<quint32>(0x0014, info.distribution_number);
	flap.appendTLV<QByteArray>(0x000f, info.language);
	flap.appendTLV<QByteArray>(0x000e, info.country);
	// Unknown shit
	flap.appendTLV<quint8>(0x0094, 0x00);
	flap.appendTLV<quint32>(0x8003, 0x00100000);
	send(flap);
}

void Client::processCloseConnection()
{
	TLVMap tlvs = flap().read<TLVMap>();
	if (tlvs.contains(0x0009)) {
		setError(AnotherClientLogined);
	} else if (tlvs.contains(0x0008)) {
		DataUnit data(tlvs.value(0x0008));
		setError(static_cast<ConnectionError>(data.read<quint16>()));
	}
	//AbstractConnection::processCloseConnection();
}

QAbstractSocket::SocketState Client::socketState() const
{
	Q_D(const Client);
	if (d->auth)
		return d->auth->socketState();
	return socket()->state();
}

Status Client::status() const
{
	return d_func()->status;
}

bool Client::isConnected()
{
	return state() == AbstractConnection::Connected;
}

void ClientPrivate::setCapability(const Capability &capability, const QString &type)
{
	if (type.isEmpty()) {
		if (!capability.isNull())
			caps.push_back(capability);
	} else {
		if (!capability.isNull())
			typedCaps.insert(type, capability);
		else
			typedCaps.remove(type);
	}
}

bool ClientPrivate::removeCapability(const Capability &capability)
{
	return caps.removeOne(capability);
}

bool ClientPrivate::removeCapability(const QString &type)
{
	bool r = typedCaps.remove(type) > 0;
	return r;
}

void Client::setCapability(const Capability &capability, const QString &type)
{
	Q_D(Client);
	d->setCapability(capability, type);
	d->sendUserInfo();
}

bool Client::removeCapability(const Capability &capability)
{
	Q_D(Client);
	bool r = d->removeCapability(capability);
	d->sendUserInfo();
	return r;
}

bool Client::removeCapability(const QString &type)
{
	Q_D(Client);
	bool r = d->removeCapability(type);
	d->sendUserInfo();
	return r;
}

bool Client::containsCapability(const Capability &capability) const
{
	Q_D(const Client);
	if (d->caps.contains(capability))
		return true;
	foreach (const Capability &cap, d->typedCaps) {
		if (cap == capability)
			return true;
	}
	return false;
}

bool Client::containsCapability(const QString &type) const
{
	Q_D(const Client);
	return d->typedCaps.contains(type);
}

QList<Capability> Client::capabilities() const
{
	Q_D(const Client);
	QList<Capability> caps = d->caps;
	foreach (const Capability &cap, d->typedCaps)
		caps << cap;
	return caps;
}

Feedbag *Client::feedbag() const
{
	return d_func()->feedbag;
}

QTextCodec *Client::asciiCodec() const
{
	return d_func()->asciiCodec;
}

void Client::setAsciiCodec(QTextCodec *codec)
{
	d_func()->asciiCodec = codec ? codec : QTextCodec::codecForLocale();;
}

QTextCodec *Client::detectCodec() const
{
	return d_func()->detectCodec;
}

void Client::onDisconnect()
{
	Q_D(Client);
	d->status = Status::Offline;
	AbstractConnection::onDisconnect();
}

void Client::onError(ConnectionError error)
{
	AbstractConnection::onError(error);
}

void Client::authError(ConnectionError e)
{
	Q_D(Client);
	setError(e, d->auth->errorString());
	d->stopLogin();
	onDisconnect();
}

void Client::onCookieTimeout()
{
	Q_D(Client);
	Q_ASSERT(qobject_cast<QTimer*>(sender()));
	QTimer *timer = static_cast<QTimer*>(sender());
	quint64 id = timer->property("cookieId").value<quint64>();
	Q_ASSERT(id != 0);
	Cookie cookie = d->cookies.take(id);
	Q_ASSERT(!cookie.isEmpty());
	QObject *receiver = cookie.receiver();
	const char *member = cookie.member();
	if (receiver && member) {
		const QMetaObject *meta = receiver->metaObject();
		const char type = member[0];
		QByteArray tmp = QMetaObject::normalizedSignature(&member[1]);
		member = tmp.constData();

		int index = -1;
		switch (type) {
		case '0': index = meta->indexOfMethod(member); break;
		case '1': index = meta->indexOfSlot(member);   break;
		case '2': index = meta->indexOfSignal(member); break;
		default:  break;
		}

		if (index != -1) {
			meta->method(index).invoke(
						receiver,
						Qt::AutoConnection,
						Q_ARG(Cookie, cookie),
						Q_ARG(QString, cookie.uin()));
		}
	}
	// cookie.unlock(); // Commented out as this cookie is already unlocked
}

void Client::sendStatus(Status status)
{
	Q_D(Client);
	SNAC snac(ServiceFamily, ServiceClientSetStatus);
	snac.appendTLV<quint32>(0x06, (d->statusFlags << 16) | status.id()); // Status mode and security flags
	snac.appendTLV<quint16>(0x08, 0x0000); // Error code
	// Status item
	DataUnit statusData;
	{
		SessionDataItem statusNote(0x02, 0x04);
		QByteArray text = Util::utf8Codec()->fromUnicode(status.text());
		if (text.size() > 251)
			text.resize(251);
		statusNote.append<quint16>(text);
		statusNote.append<quint16>(0); // endcoding: utf8 by default
		statusData.append(statusNote);
	}
	snac.appendTLV(0x1D, statusData);
	snac.appendTLV<quint16>(0x1f, 0x00); // unknown
	send(snac);

	bool updateCaps = false;
	QSet<QString> types = Status::allSupportedCapabilityTypes();
	CapabilityHash caps = status.capabilities();
	CapabilityHash::const_iterator itr = caps.constBegin();
	CapabilityHash::const_iterator endItr = caps.constEnd();
	while (itr != endItr) {
		if (itr->isNull())
			continue;
		types.remove(itr.key());
		d->setCapability(itr.value(), itr.key());
		updateCaps = true;
		++itr;
	}
	foreach (const QString &type, types) {
		d->removeCapability(type);
		updateCaps = true;
	}
	if (updateCaps)
		d->sendUserInfo();
}

void Client::handleSNAC(AbstractConnection *conn, const SNAC &sn)
{
	Q_D(Client);
	Q_UNUSED(conn);
	Q_ASSERT(this == conn);
	AbstractConnection::handleSNAC(this, sn);
	sn.resetState();
	switch ((sn.family() << 16) | sn.subtype()) {
	// Server sends rate limits information
	case ServiceFamily << 16 | ServiceServerAsksServices: {
		// Requesting Location rights
		SNAC snac(LocationFamily, LocationCliReqRights);
		send(snac);

		// Sending CLI_REQBOS
		snac.reset(BosFamily, PrivacyReqRights);
		send(snac);
		break;
	}
	// Server replies via location service limitations
	case LocationFamily << 16 | LocationRightsReply: {
		// TODO: Implement, it's important
		break;
	}
	// Server sends PRM service limitations to client
	case BosFamily << 16 | PrivacyRightsReply: {
		if (!d->feedbag)
			finishLogin();
		break;
	}
	}
}

void Client::finishLogin()
{
	Q_D(Client);
	setState(Connected);
	d->sendUserInfo(true);
	d->isIdle = true;
	setIdle(false);
	SNAC snac(ServiceFamily, ServiceClientReady);
	// imitate ICQ 6 behaviour
	snac.append(QByteArray::fromHex(
		"0022 0001 0110 164f"
		"0001 0004 0110 164f"
		"0013 0004 0110 164f"
		"0002 0001 0110 164f"
		"0003 0001 0110 164f"
		"0015 0001 0110 164f"
		"0004 0001 0110 164f"
		"0006 0001 0110 164f"
		"0009 0001 0110 164f"
		"000a 0001 0110 164f"
		"000b 0001 0110 164f"));
	send(snac);
	d->status = Status::Online;
	emit loginFinished();
}

void Client::setIdle(bool allow)
{
	Q_D(Client);
	if (d->isIdle == allow)
		return;
	SNAC snac(ServiceFamily, 0x0011);
	snac.append<quint32>(allow ? 0x0000003C : 0x00000000);
	send(snac);
}

} // namespace Ireen
