/****************************************************************************
**
** Ireen — cross-platform OSCAR protocol library
**
** Copyright © 2012 Alexey Prokhin <alexey.prokhin@yandex.ru>
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

#include "oscarfiletransfer_p.h"
#include "buddycaps.h"
#include "tlv.h"
#include "client.h"
#include <QHostAddress>
#include <QDir>
#include <QTimer>
#include <QApplication>

namespace Ireen {

QHash<quint16, OftServer*> OftManagerPrivate::servers;
bool OftManagerPrivate::allowAnyPort = true;

const int BUFFER_SIZE = 4096;
using namespace Util;

OftHeader::OftHeader() :
	encrypt(false),
	compress(false),
	totalFiles(1),
	filesLeft(1),
	totalParts(1),
	partsLeft(1),
	totalSize(0),
	checksum(0xFFFF0000),
	receivedResourceForkChecksum(0xFFFF0000),
	resourceForkChecksum(0xFFFF0000),
	bytesReceived(0),
	receivedChecksum(0xFFFF0000),
	identification("Cool FileXfer"),
	flags(0x20),
	m_state(ReadHeader)
{
}

void OftHeader::readData(QIODevice *dev, QTextCodec *asciiCodec)
{
	if (m_state == ReadHeader) {
		DataUnit data(dev->read(6));
		/*quint32 protVersion = */data.read<quint32>();  // TODO: test it
		m_length = data.read<quint16>() - 6;
		m_state = ReadData;
		m_data.resize(m_length);
	}
	if (m_state == ReadData) {
		char *data = m_data.data() + m_data.size() - m_length;
		int readed = dev->read(data, m_length);
		m_length -= readed;
	}
	if (m_length == 0) {
		DataUnit data(m_data);
		type = static_cast<OftPacketType>(data.read<quint16>());
		cookie = data.read<quint64>();
		encrypt = data.read<quint16>();
		compress = data.read<quint16>();
		totalFiles = data.read<quint16>();
		filesLeft = data.read<quint16>();
		totalParts = data.read<quint16>();
		partsLeft = data.read<quint16>();
		totalSize = data.read<quint32>();
		size = data.read<quint32>();
		modTime = data.read<quint32>();
		checksum = data.read<quint32>();
		receivedResourceForkChecksum = data.read<quint32>();
		resourceForkSize = data.read<quint32>();
		creationTime = data.read<quint32>();
		resourceForkChecksum = data.read<quint32>();
		bytesReceived = data.read<quint32>();
		receivedChecksum = data.read<quint32>();
		identification = QString::fromLatin1(data.readData(32));
		flags = data.read<quint8>();
		data.skipData(71); // skipped List Name Offset, List Size Offset and "Dummy" block
		macFileInfo = data.readData(16);
		quint16 encoding = data.read<quint16>();
		//quint16 encodingSubcode = data.read<quint16>();
		data.skipData(2);
		QTextCodec *codec = 0;
		if (encoding == CodecUtf16Be)
			codec = utf16Codec();
		else
			codec = asciiCodec;
		QByteArray name = data.readAll();
		if (name.size() == 64) {
			uint c = qstrnlen(name.constData(), name.size());
			name.resize(c);
		} else if (encoding == CodecUtf16Be) {
			name.chop(2);
		} else {
			name.chop(1);
		}
		fileName = codec->toUnicode(name);
		m_state = Finished;
	}
}

static void resizeArray(QByteArray &data, int size)
{
	int oldSize = data.size();
	data.resize(size);
	if (oldSize < size)
		memset(data.data()+oldSize, 0, size-oldSize);
}

void OftHeader::writeData(QIODevice *dev)
{
	DataUnit data;
	debug() << "Outgoing oft message with type" << hex << type;
	data.append<quint16>(type);
	data.append<quint64>(cookie);
	data.append<quint16>(encrypt);
	data.append<quint16>(compress);
	data.append<quint16>(totalFiles);
	data.append<quint16>(filesLeft);
	data.append<quint16>(totalParts);
	data.append<quint16>(partsLeft);
	data.append<quint32>(totalSize);
	data.append<quint32>(size);
	data.append<quint32>(modTime);
	data.append<quint32>(checksum);
	data.append<quint32>(receivedResourceForkChecksum);
	data.append<quint32>(resourceForkSize);
	data.append<quint32>(creationTime);
	data.append<quint32>(resourceForkChecksum);
	data.append<quint32>(bytesReceived);
	data.append<quint32>(receivedChecksum);
	{
		QByteArray ident = identification.toLatin1();
		resizeArray(ident, 32);
		data.append(ident);
	}
	data.append<quint8>(flags);
	data.append<quint8>(0x1C);
	data.append<quint8>(0x11);
	{
		QByteArray dummy;
		resizeArray(dummy, 69);
		data.append(dummy);
	}
	resizeArray(macFileInfo, 16);
	data.append(macFileInfo);
	data.append<quint16>(CodecUtf16Be);
	data.append<quint16>(0);
	{
		QByteArray name = utf16Codec()->fromUnicode(fileName);
		name = name.mid(2);
		if (name.size() < 64)
			resizeArray(name, 64);
		else
			name.append("\0\0");
		data.append(name);
	}
	DataUnit header;
	header.append<quint32>(0x4F465432); // Protocol version: "OFT2"
	header.append<quint16>(data.dataSize() + 6);
	header.append(data);
	dev->write(header.data());
}

OftSocket::OftSocket(Client *client, QObject *parent) :
	QTcpSocket(parent), m_client(client)
{
	init();
}

OftSocket::OftSocket(Client *client, int socketDescriptor, QObject *parent) :
	QTcpSocket(parent), m_client(client)
{
	setSocketDescriptor(socketDescriptor);
	init();
}

void OftSocket::init()
{
	connect(this, SIGNAL(readyRead()), SLOT(onReadyRead()));
	connect(this, SIGNAL(connected()), this, SLOT(connected()));
	m_state = ReadHeader;
	m_len = 0;
	m_hostReqId = 0;
	m_timer.setInterval(FILETRANSFER_WAITING_TIMEOUT);
	m_timer.setSingleShot(true);
	connect(&m_timer, SIGNAL(timeout()), SLOT(onTimeout()));
}

void OftSocket::directConnect(const QHostAddress &addr, quint16 port)
{
	m_state = ReadHeader;
	connectToHost(addr, port);
	m_clientPort = port;
	m_timer.start();
	debug().nospace() << "Trying to establish a direct connection to "
			<< addr.toString().toLocal8Bit().constData()
			<< ":" << port;
}

void OftSocket::proxyConnect()
{
	m_state = ProxyInit;
	m_lastHeader = OftHeader();
	m_len = 0;
	m_proxyPort = 0;
	m_hostReqId = QHostInfo::lookupHost("ars.oscar.aol.com", this, SLOT(proxyFound(QHostInfo)));
}

void OftSocket::proxyFound(const QHostInfo &proxyInfo)
{
	m_hostReqId = 0;
	QList<QHostAddress> addresses = proxyInfo.addresses();
	if (!addresses.isEmpty()) {
		connectToProxy(addresses.at(qrand() % addresses.size()), 5190);
	} else {
		setErrorString(tr("The file transfer proxy server is not available"));
		emit error(QAbstractSocket::ProxyNotFoundError);
	}
}

void OftSocket::proxyConnect(QHostAddress addr, quint16 port)
{
	m_state = ProxyReceive;
	m_lastHeader = OftHeader();
	m_len = 0;
	m_proxyPort = port;
	connectToProxy(addr, port);
}

void OftSocket::connectToProxy(const QHostAddress &addr, quint16 port)
{
	connectToHost(addr, port);
	debug().nospace() << "Trying to connect to the proxy "
			<< addr.toString().toLocal8Bit().constData()
			<< ":" << port;
	m_timer.start();
}

void OftSocket::dataReaded()
{
	m_state = ReadHeader;
	m_lastHeader = OftHeader();
	if (bytesAvailable() > 0)
		onReadyRead();
}

void OftSocket::onReadyRead()
{
	if (m_state & Proxy) {
		DataUnit data;
		if (m_len == 0) {
			if (bytesAvailable() <= 4)
				return;
			data.setData(read(4));
			m_len = data.read<quint16>() - 2;
			if (data.read<quint16>() != 0x044A)
				debug() << "Unknown proxy protocol version";
		}
		if (bytesAvailable() <= m_len) {
			data.setData(read(m_len));
			m_len = 0;
		} else {
			return;
		}
		quint16 type = data.read<quint16>();
		data.skipData(4); // unknown
		quint16 flags = data.read<quint16>();
		Q_UNUSED(flags);
		debug() << "Rendezvous proxy packet. Type" << type;
		switch (type) {
		case 0x0001 : { // error
			quint16 code = data.read<quint16>();
			QString str;
			if (code == 0x000d || code == 0x000e)
				str = "Bad Request";
			else if (code == 0x0010)
				str = "Initial Request Timed Out";
			else if (code == 0x001a)
				str = "Accept Period Timed Out";
			else
				str = QString("Unknown rendezvous proxy error: %1").arg(code);
			debug() << "Rendezvous proxy error:" << str;
			setSocketError(QAbstractSocket::ProxyProtocolError);
			setErrorString(str);
			emit error(QAbstractSocket::ProxyProtocolError);
			break;
		}
		case 0x0003 : { // Acknowledge
			if (m_state != ProxyInit) {
				setSocketError(QAbstractSocket::ProxyProtocolError);
				setErrorString("Rendezvous proxy acknowledge packets are forbidden");
				emit error(QAbstractSocket::ProxyProtocolError);
				break;
			}
			m_proxyPort = data.read<quint16>();
			m_proxyIP.setAddress(data.read<quint32>());
			emit proxyInitialized();
			break;
		}
		case 0x0005 : { // Ready
			m_state = ReadHeader;
			emit initialized();
			break;
		}
		default:
			setSocketError(QAbstractSocket::ProxyProtocolError);
			setErrorString(QString("Unknown rendezvous proxy request").arg(type));
			emit error(QAbstractSocket::ProxyProtocolError);
		}
	} else {
		if (m_state == ReadHeader) {
			m_lastHeader.readData(this, m_client->asciiCodec());
			if (m_lastHeader.isFinished()) {
				m_state = ReadData;
				emit headerReaded(m_lastHeader);
			}
		}
		if (m_state == ReadData && bytesAvailable() > 0)
			emit newData();
	}
}

void OftSocket::connected()
{
	m_timer.stop();
	if (m_state & Proxy) {
		DataUnit data;
		data.append<quint8>(m_client->uin(), m_client->asciiCodec()); // uin or screen name
		if (m_state == ProxyReceive)
			data.append<quint16>(m_proxyPort);
		data.append<quint64>(m_cookie);
		data.appendTLV(0x0001, ICQ_CAPABILITY_AIMSENDFILE); // capability
		DataUnit header;
		header.append<quint16>(10 + data.dataSize());
		header.append<quint16>(0x044A); // proto version
		header.append<quint16>(m_state == ProxyInit ? 0x0002 : 0x0004); // request cmd
		header.append<quint32>(0); // unknown
		header.append<quint16>(0); // flags
		header.append(data.data());
		write(header.data());
		flush();
	} else {
		emit initialized();
	}
}

void OftSocket::disconnected()
{
}

void OftSocket::onTimeout()
{
	if (m_hostReqId != 0)
		QHostInfo::abortHostLookup(m_hostReqId);
	emit timeout();
	close();
}

OftServer::OftServer(quint16 port) :
	m_port(port)
{
	m_timer.setInterval(FILETRANSFER_WAITING_TIMEOUT);
	m_timer.setSingleShot(true);
	connect(&m_timer, SIGNAL(timeout()), SLOT(onTimeout()));
}

void OftServer::listen()
{
	m_timer.start();
	QTcpServer::listen();
	debug() << "Started listening for incoming connections on port" << serverPort();
	//emit m_conn->localPortChanged(serverPort());
}

void OftServer::close()
{
	m_conn = 0;
	m_timer.stop();
	QTcpServer::close();
}

void OftServer::setConnection(OftConnection *conn)
{
	m_conn = conn;
}

void OftServer::incomingConnection(int socketDescriptor)
{
	OftSocket *socket = new OftSocket(m_conn->client(), socketDescriptor);
	debug().nospace() << "Incoming oscar transfer connection from "
			<< socket->peerAddress().toString().toLatin1().constData()
			<< ":" << socket->peerPort();
	m_conn->d->setSocket(socket);
	emit closed(m_conn);
	m_conn->d->connected();
	close();
	m_timer.stop();
}

void OftServer::onTimeout()
{
	emit timeout(m_conn);
	close();
}

OftChecksumThread::OftChecksumThread(QIODevice *f, int b) :
	file(f), bytes(b)
{
}

quint32 OftChecksumThread::chunkChecksum(const char *buffer, int len, quint32 oldChecksum, int offset)
{
	// code adapted from miranda's oft_calc_checksum
	quint32 checksum = (oldChecksum >> 16) & 0xffff;
	for (int i = 0; i < len; i++)
	{
		quint16 val = buffer[i];
		//quint32 oldchecksum = checksum;
		if (((i + offset) & 1) == 0)
			val = val << 8;
		if (checksum < val)
			checksum -= val + 1;
		else // simulate carry
			checksum -= val;
	}
	checksum = ((checksum & 0x0000ffff) + (checksum >> 16));
	checksum = ((checksum & 0x0000ffff) + (checksum >> 16));
	return (quint32)checksum << 16;
}

void OftChecksumThread::run()
{
	quint32 checksum = 0xFFFF0000;
	QByteArray data;
	data.reserve(BUFFER_SIZE);
	int totalRead = 0;
	if (bytes <= 0)
		bytes = file->size();
	bool isOpen = file->isOpen();
	if (!isOpen)
		file->open(QIODevice::ReadOnly);
	while (totalRead < bytes) {
		data = file->read(qMin(BUFFER_SIZE, bytes - totalRead));
		checksum = chunkChecksum(data.constData(), data.size(), checksum, totalRead);
		totalRead += data.size();
		//QApplication::processEvents(); // This call causes crashes
	}
	if (!isOpen)
		file->close();
	emit done(checksum);
}

OftConnection::OftConnection(const QString &uin, bool isIncoming, quint64 cookie,
							 OftManager *manager, bool forceProxy, quint32 totalSize,
							 quint16 filesCount, const QString &title) :
	d(new OftConnectionPrivate)
{
	d->q = this;
	d->isIncoming = isIncoming;
	d->isAccepted = !isIncoming;
	d->proxy = forceProxy;
	d->connInited = false;
	d->filesCount = 0;
	d->currentFileIndex = 0;
	d->totalSize = 0;
	d->currentFileSize = 0;
	d->cookie = cookie;
	d->manager = manager;
	d->uin = uin;
	d->totalSize = totalSize;
	d->filesCount = filesCount;
	d->title = title;

	d->manager->d->addConnection(this);
}

OftConnection::~OftConnection()
{
	d->manager->d->removeConnection(this);
}

QString OftConnection::uin() const
{
	return d->uin;
}

quint64 OftConnection::cookie() const
{
	return d->cookie;
}

OftManager *OftConnection::manager() const
{
	return d->manager;
}

Client *OftConnection::client() const
{
	return d->manager->client();
}

quint16 OftConnection::localPort() const
{
	if (d->socket && d->socket->isOpen())
		return d->socket->localPort();
	if (d->server && d->server->isListening())
		return d->server->serverPort();
	return 0;
}

quint16 OftConnection::remotePort() const
{
	if (d->socket && d->socket->isOpen())
		return d->socket->peerPort();
	return 0;
}

QHostAddress OftConnection::remoteAddress() const
{
	if (d->socket && d->socket->isOpen())
		return d->socket->peerAddress();
	return QHostAddress();
}

QString OftConnection::title() const
{
	return d->title;
}

quint32 OftConnection::totalSize() const
{
	return d->totalSize;
}

quint16 OftConnection::filesCount() const
{
	return d->filesCount;
}

QString OftConnection::currentFileName() const
{
	return d->currentFileName;
}

quint32 OftConnection::currentFileSize() const
{
	return d->currentFileSize;
}

quint16 OftConnection::currentFileIndex() const
{
	return d->currentFileIndex;
}

void OftConnection::send()
{
	d->stage = 1;
	if (!d->proxy) {
		d->sendFileRequest();
	} else {
		d->setSocket(new OftSocket(client(), this));
		d->socket->proxyConnect();
	}
}

void OftConnection::cancel()
{
	Channel2BasicMessageData data(MsgCancel, ICQ_CAPABILITY_AIMSENDFILE, d->cookie);
	ServerMessage message(d->uin, data);
	client()->send(message);
    d->close(false);
}

void OftConnection::accept()
{
	d->isAccepted = true;
	if (d->state == Initiation && d->connInited && d->isIncoming)
		d->waitIODevice();
}

void OftConnection::resumeSending(QIODevice *outputDevice, const QString &name, quint32 size, const QDateTime &lastModified)
{
	Q_ASSERT(d->state == AwaitingIODevice && !d->isIncoming);
	d->data.reset(outputDevice);
	if (!d->data) {
		d->setState(Error);
		d->setError(IOError);
		d->close(false);
		return;
	}

	d->currentFileName = name;
	d->currentFileSize = size;
	d->lastModified = lastModified;
	emit currentFileChanged();

	OftChecksumThread *checksum = new OftChecksumThread(d->data.data(), d->header.size);
	connect(checksum, SIGNAL(done(quint32)), SLOT(resumeSendingImpl(quint32)));
	checksum->start();
}

void OftConnection::resumeReceiving(QIODevice *inputDevice)
{
	Q_ASSERT(d->state == AwaitingIODevice && d->isIncoming);
	d->data.reset(inputDevice);
	if (!d->data) {
		d->setState(Error);
		d->setError(IOError);
		d->close(false);
		return;
	}

	QFile *file = qobject_cast<QFile*>(d->data.data());
	bool exist = file && file->exists() && file->size() <= d->header.size;
	if (exist) {
		d->header.bytesReceived = file->size();
		d->header.type = d->header.bytesReceived == d->header.size ? OftDone : OftReceiverResume;
		OftChecksumThread *checksum = new OftChecksumThread(d->data.data(), d->header.size);
		connect(checksum, SIGNAL(done(quint32)), SLOT(resumeReceivingImpl(quint32)));
		checksum->start();
	} else {
		if (!d->data->open(QIODevice::WriteOnly)) {
			d->close(false);
			d->setState(Error);
			d->setError(IOError, file ? tr("Could not open %1").arg(file->fileName()) : QString());
			return;
		}
		d->header.type = OftAcknowledge;
		d->onNewData();
		d->resumeReceivingImpl(false);
	}
}

void OftConnectionPrivate::close(bool error)
{
	if (socket) {
		if (!error)
			socket->close();
		socket->deleteLater();
	}
	if (data)
		data.reset();
	if (error) {
		Channel2BasicMessageData data(MsgCancel, ICQ_CAPABILITY_AIMSENDFILE, cookie);
		ServerMessage message(uin, data);
		manager->client()->send(message);
		setState(OftConnection::Error);
		setError(OftConnection::NetworkError);
	}
}

void OftConnectionPrivate::handleRendezvous(quint16 reqType, const TLVMap &tlvs)
{
	if (reqType == MsgRequest) {
		debug() << uin << "has sent file transfer request";
		stage = tlvs.value<quint16>(0x000A);
		QHostAddress proxyIP(tlvs.value<quint32>(0x0002));
		QHostAddress clientIP(tlvs.value<quint32>(0x0003));
		clientVerifiedIP = QHostAddress(tlvs.value<quint32>(0x0004));
		quint16 port = tlvs.value<quint16>(0x0005);
		bool forceProxy = proxy;
		proxy = tlvs.contains(0x0010);
		DataUnit tlv2711(tlvs.value(0x2711));
		bool multipleFiles = tlv2711.read<quint16>() > 1;
		quint16 filesCount = tlv2711.read<quint16>();
		quint32 totalSize = tlv2711.read<quint32>();
		Q_UNUSED(multipleFiles);
		QTextCodec *codec = 0;
		{
			QByteArray encoding = tlvs.value(0x2722);
			if (!encoding.isEmpty())
				codec = QTextCodec::codecForName(encoding.constData());
			if (!codec)
				codec = defaultCodec();
		}
		QString title = codec->toUnicode(tlv2711.readAll());
		title.chop(1);

		QString errorStr;
		if (stage == 1) {
			if (isIncoming) {
				this->filesCount = filesCount;
				this->totalSize = totalSize;
				this->title = title;
			} else {
				errorStr = "Stage 1 oscar file transfer request is forbidden during file sending";
			}
		} else if (stage == 2) {
			if (!isIncoming) {
				if (server)
					server->close();
				if (socket) {
					debug() << "Sender has sent the request for reverse connection (stage 2)"
							<< "but the connection already initialized at stage 1";
					return;
				}
			} else {
				errorStr = "Stage 2 oscar file transfer request is forbidden during file receiving";
			}
		} else if (stage == 3) {
			if (isIncoming)
				proxy = true;
			else
				errorStr = "Stage 3 oscar file transfer request is forbidden during file sending";
		} else {
			errorStr = QString("Unknown file transfer request at stage %1").arg(stage);
		}

		if (!errorStr.isEmpty()) {
			debug() << errorStr;
			close();
			return;
		}

		if (!proxy && clientIP.isNull()) {
			// The conntact has not sent us its IP, so skip that stage
			startNextStage();
			return;
		}
		setSocket(new OftSocket(manager->client(), q));
		if (!proxy && forceProxy) {
			// The contact wants a direct connection which
			// is forbidden by the user
			proxy = true;
			clientVerifiedIP = QHostAddress::Null;
			startNextStage();
			return;
		}
		if (!proxy)
			socket->directConnect(clientIP, port);
		else
			socket->proxyConnect(proxyIP, port);
		q->connect(socket, SIGNAL(timeout()), SLOT(startNextStage()));
	} else if (reqType == MsgAccept) {
		debug() << uin << "accepted file transfing";
	} else if (reqType == MsgCancel) {
		debug() << uin << "canceled file transfing";
		close(false);
		setState(OftConnection::Error);
		setError(OftConnection::Canceled);
	}
}

void OftConnectionPrivate::setSocket(OftSocket *newSocket)
{
	if (!newSocket) {
		socket = newSocket;
		socket->setParent(q);
		socket->setCookie(cookie);
		q->connect(socket, SIGNAL(proxyInitialized()), SLOT(sendFileRequest()));
		q->connect(socket, SIGNAL(initialized()), SLOT(connected()));
		q->connect(socket, SIGNAL(error(QAbstractSocket::SocketError)),
				SLOT(onError(QAbstractSocket::SocketError)));
		q->connect(socket, SIGNAL(headerReaded(OftHeader)), SLOT(onHeaderReaded()));
		q->connect(socket, SIGNAL(disconnected()), socket, SLOT(deleteLater()));
		if (socket->readingState() == OftSocket::ReadData) {
			onHeaderReaded();
			if (socket->bytesAvailable() > 0)
				onNewData();
		}
		emit q->localPortChanged(socket->localPort());
	} else {
		newSocket->deleteLater();
		debug() << "Cannot change socket in an initialized oscar file transfer connection";
	}
}

void OftConnectionPrivate::waitIODevice()
{
	if (!isIncoming && currentFileIndex >= filesCount) {
		close(false);
		setState(OftConnection::Finished);
		return;
	}
	setState(OftConnection::AwaitingIODevice);
}

void OftConnectionPrivate::sendFileRequest()
{
	Channel2BasicMessageData data(MsgRequest, ICQ_CAPABILITY_AIMSENDFILE, cookie);
	quint32 clientAddr = 0;
	quint32 proxyAddr;
	quint16 port;
	if (proxy) {
		proxyAddr = socket->proxyIP().toIPv4Address();
		port = socket->proxyPort();
	} else {
		server = OftManagerPrivate::getFreeServer();
		if (server) {
			server->setConnection(q);
			server->listen();
			// That does not work well with all clients
			// connect(server, SIGNAL(timeout(OftConnection*)), SLOT(close()));
			clientAddr = manager->client()->socket()->localAddress().toIPv4Address();
			port = server->serverPort();
			emit q->localPortChanged(server->serverPort());
		} else {
			clientAddr = 0;
			port = 0;
		}
		proxyAddr = clientAddr;
	}
	data.appendTLV<quint16>(0x000A, stage);
	data.appendTLV<quint32>(0x0002, proxyAddr);
	data.appendTLV<quint32>(0x0016, proxyAddr ^ 0x0FFFFFFFF);
	data.appendTLV<quint32>(0x0003, clientAddr);
	data.appendTLV<quint16>(0x0005, port);
	data.appendTLV<quint16>(0x0017, port ^ 0x0FFFF);
	if (proxy)
		data.appendTLV(0x0010);
	if (stage == 1) {
		{
			// file info
			const int count = filesCount;
			DataUnit tlv2711;
			tlv2711.append<quint16>(count >= 2 ? 2 : 1);
			tlv2711.append<quint16>(count); // count
			tlv2711.append<quint32>(totalSize);
			tlv2711.append(title, defaultCodec()); // ???
			tlv2711.append<quint8>(0);
			data.appendTLV(0x2711, tlv2711);
		}
		{
			// file name encoding
			DataUnit tlv2722;
			tlv2722.append(defaultCodec()->name());
			data.appendTLV(0x2722, tlv2722);
		}
	}
	ServerMessage message(uin, data);
	manager->client()->send(message);
	debug() << "A stage" << stage << "file transfer request has been sent";
}

void OftConnectionPrivate::connected()
{
	emit q->remoteAddressChanged(socket->peerAddress());
	emit q->remotePortChanged(socket->peerPort());

	if (isIncoming) {
		Channel2BasicMessageData data(MsgAccept, ICQ_CAPABILITY_AIMSENDFILE, cookie);
		ServerMessage message(uin, data);
		manager->client()->send(message);
	} else {
		++currentFileIndex;
		waitIODevice();
	}
}

void OftConnectionPrivate::onError(QAbstractSocket::SocketError error)
{
	bool connClosed = error == QAbstractSocket::RemoteHostClosedError;
	if (stage == 1 && isIncoming && !connClosed) {
		startNextStage();
	} else if (stage == 2 && !isIncoming && !connClosed) {
		startNextStage();
	} else {
		if (connClosed && header.bytesReceived == header.size && header.filesLeft <= 1) {
			debug() << "File transfer connection closed";
			setState(OftConnection::Finished);
			close(false);
		} else {
			debug() << "File transfer connection error" << socket->errorString();
			close();
		}
	}
}

void OftConnectionPrivate::onNewData()
{
	if (!data) {
		debug() << "File transfer data has been received when the output file is not initialized";
		return;
	}
	if (socket->bytesAvailable() <= 0)
		return;
	QByteArray buf = socket->read(header.size - header.bytesReceived);
	header.receivedChecksum =
			OftChecksumThread::chunkChecksum(buf.constData(), buf.size(),
											 header.receivedChecksum,
											 header.bytesReceived);
	header.bytesReceived += buf.size();
	data.data()->write(buf);
	emit q->progress(header.bytesReceived);
	if (header.bytesReceived == header.size) {
		q->disconnect(socket, SIGNAL(newData()), q, SLOT(onNewData()));
		data.reset();
		header.type = OftDone;
		--header.filesLeft;
		header.writeData(socket);
		socket->dataReaded();
		if (header.filesLeft == 0)
			setState(OftConnection::Finished);
	}
}

void OftConnectionPrivate::onSendData()
{
	if (!data && socket->bytesToWrite())
		return;
	QByteArray buf = data.data()->read(BUFFER_SIZE);
	header.receivedChecksum =
			OftChecksumThread::chunkChecksum(buf.constData(), buf.size(),
											 header.receivedChecksum,
											 header.bytesReceived);
	header.bytesReceived += buf.size();
	socket->write(buf);
	emit q->progress(header.bytesReceived);
	if (header.bytesReceived == header.size) {
		q->disconnect(socket, SIGNAL(bytesWritten(qint64)), q, SLOT(onSendData()));
		data.reset();
	}
}

void OftConnectionPrivate::resumeSendingImpl(quint32 checksum)
{
	q->sender()->deleteLater();
	header.type = OftPrompt;
	header.cookie = cookie;
	header.modTime = lastModified.toTime_t();
	header.size = currentFileSize;
	header.fileName = currentFileName;
	header.checksum = checksum;
	header.receivedChecksum = 0xFFFF0000;
	header.bytesReceived = 0;
	header.totalSize = totalSize;
	header.writeData(socket);
	header.filesLeft = filesCount - currentFileIndex;
	setState(OftConnection::Started);
}

void OftConnectionPrivate::resumeReceivingImpl(quint32 checksum)
{
	q->sender()->deleteLater();
	header.receivedChecksum = checksum;
	resumeReceivingImpl(true);
}

void OftConnectionPrivate::resumeReceivingImpl(bool resume)
{
	header.cookie = cookie;
	header.writeData(socket);
	if (resume)
		socket->dataReaded();
	setState(OftConnection::Started);
	q->connect(socket, SIGNAL(newData()), SLOT(onNewData()));
}

void OftConnectionPrivate::setError(OftConnection::ErrorType newError, const QString &desc)
{
	if (error != newError) {
		error = newError;
		emit q->error(error, desc);
	}
}

void OftConnectionPrivate::setState(OftConnection::State newState)
{
	if (state != newState) {
		state = newState;
		emit q->stateChanged(state);
	}
}

void OftConnectionPrivate::resumeFileReceivingImpl(quint32 checksum)
{
	if (checksum != header.receivedChecksum) { // receiver's file is corrupt
		header.receivedChecksum = 0xffff0000;
		header.bytesReceived = 0;
	}
	header.type = OftSenderResume;
	header.writeData(socket);
}

void OftConnectionPrivate::startNextStage()
{
	if (stage == 1) {
		if (!proxy && !clientVerifiedIP.isNull()) {
			socket->close();
			socket->directConnect(clientVerifiedIP, socket->clientPort());
			clientVerifiedIP = QHostAddress::Null;
		} else {
			stage = 2;
			if (proxy) {
				socket->close();
				socket->proxyConnect();
			} else {
				socket->deleteLater();
				sendFileRequest();
			}
		}
	} else if (stage == 2) {
		if (!proxy && !clientVerifiedIP.isNull()) {
			socket->close();
			socket->directConnect(clientVerifiedIP, socket->clientPort());
			clientVerifiedIP = QHostAddress::Null;
		} else {
			stage = 3;
			proxy = true;
			socket->close();
			socket->proxyConnect();
		}
	} else {
		close();
	}
}

void OftConnectionPrivate::onHeaderReaded()
{
	if (socket->lastHeader().isFinished()) {
		header = socket->lastHeader();
		QString error;
		if (isIncoming) {
			if (header.type & OftReceiver)
				error = QString("Oft message type %1 is not allowed during receiving");
		} else {
			if (header.type & OftSender)
				error = QString("Oft message type %1 is not allowed during sending");
		}
		if (!error.isEmpty()) {
			debug() << error.arg(header.type);
			close();
			return;
		}
		debug() << "Incoming oft message with type" << hex << header.type;
		switch (header.type) {
		case OftPrompt: { // Sender has sent us info about file transfer
			if (data) {
				debug() << "Prompt messages are not allowed during resuming receiving";
				return;
			}

			connInited = true;

			currentFileIndex = header.totalFiles - header.filesLeft;
			if (currentFileIndex >= filesCount) {
				debug() << "Sender sent wrong OftPrompt filetransfer request";
				close();
				break;
			}

			currentFileSize = header.size;
			currentFileName = header.fileName;
			emit q->currentFileChanged();

			if (isAccepted)
				waitIODevice();
			break;
		}
		case OftDone: { // Receiver has informed us about received file
			++currentFileIndex;
			waitIODevice();
			break;
		}
		case OftReceiverResume: { // Receiver wants to resume old file transfer
			if (!data) {
				debug() << "Sender sent OftReceiverResume filetransfer request before OftPrompt";
				close();
				return;
			}

			OftChecksumThread *checksum = new OftChecksumThread(data.data(), header.bytesReceived);
			q->connect(checksum, SIGNAL(done(quint32)), SLOT(resumeFileReceivingImpl(quint32)));
			checksum->start();
			break;
		}
		case OftSenderResume: { // Sender responded at our resuming request
			if (!data) {
				debug() << "The sender had sent OftReceiverResume filetransfer request"
						<< "before the receiver sent OftPromt";
				close();
				return;
			}
			QIODevice::OpenMode flags;
			header.type = OftResumeAcknowledge;
			if (header.bytesReceived) { // ok. resume receiving
				flags = QIODevice::WriteOnly | QIODevice::Append;
				debug() << "Receiving of file" << header.fileName << "will be resumed";
			} else { // sender said that our local file is corrupt
				flags = QIODevice::WriteOnly;
				header.receivedChecksum = 0xffff0000;
				header.bytesReceived = 0;
				debug() << "File" << header.fileName << "will be rewritten";
			}
			if (data.data()->open(flags)) {
				header.writeData(socket);
			} else {
				close();
			}
			break;
		}
		case OftResumeAcknowledge:
		case OftAcknowledge: {	// receiver are waiting file
			socket->dataReaded();
			if (data.data()->open(QFile::ReadOnly)) {
				q->connect(socket, SIGNAL(bytesWritten(qint64)), q, SLOT(onSendData()));
				setState(OftConnection::Started);
				onSendData();
			} else {
				close();
			}
			break;
		}
		default:
			debug() << "Unknown oft message type" << hex << header.type;
			socket->dataReaded();
		}
	}
}

void OftManagerPrivate::processMessage(const QString &uin, const Capability &guid,
											const QByteArray &data, quint16 reqType,
											const Cookie &cookie)
{
	Q_UNUSED(guid);
	TLVMap tlvs = DataUnit(data).read<TLVMap>();
	OftConnection *conn = connection(cookie.id());
	if (conn && conn->uin() != uin) {
		debug() << "Cannot create two oscar file transfer with the same cookie" << cookie.id();
		return;
	}
	bool newRequest = reqType == MsgRequest && !conn;
	if (newRequest)
		conn = new OftConnection(uin, true, cookie.id(), q, forceProxy);
	if (conn) {
		conn->d->handleRendezvous(reqType, tlvs);
		if (conn->title().isNull())
			// We were waiting stage 1 message, but the contact had sent us something else.
			conn->deleteLater();
		else
			q->incomingConnection(conn);
	} else {
		debug() << "Skipped oscar file transfer request with unknown cookie";
	}
}

OftConnection *OftManagerPrivate::connection(quint64 cookie)
{
	return connections.value(cookie);
}

void OftManagerPrivate::addConnection(OftConnection *connection)
{
	connections.insert(connection->cookie(), connection);
}

void OftManagerPrivate::removeConnection(OftConnection *connection)
{
	connections.remove(connection->cookie());
}

OftServer *OftManagerPrivate::getFreeServer()
{
	if (allowAnyPort) {
		OftServer *server = new OftServer(0);
		QObject::connect(server, SIGNAL(closed(OftConnection*)),
						 server, SLOT(deleteLater()));
		return server;
	} else {
		foreach (OftServer *server, servers) {
			if (!server->isListening())
				return server;
		}
	}
	return 0;
}

void OftManagerPrivate::deleteOftServer(OftServer *server)
{
	if (server->isListening())
		QObject::connect(server, SIGNAL(closed(OftConnection*)),
						 server, SLOT(deleteLater()));
	else
		server->deleteLater();
}


void OftManagerPrivate::setAllowAnyServerPort(bool _allowAnyPort)
{
	allowAnyPort = _allowAnyPort;
	if (allowAnyPort) {
		std::for_each(servers.constBegin(), servers.constEnd(),
					  &deleteOftServer);
		servers.clear();
	}
}

void OftManagerPrivate::setServerPorts(const QList<quint16> &ports)
{
	QSet<quint16> oldServers = servers.keys().toSet();

	// Update servers list
	foreach (quint16 port, ports) {
		if (!servers.contains(port))
			servers.insert(port, new OftServer(port));
		oldServers.remove(port);
	}

	// Remove disabled servers
	foreach (quint16 port, oldServers) {
		OftServer *server = servers.take(port);
		deleteOftServer(server);
	}
}

OftManager::OftManager(MessageHandler *messageHandler) :
	d(new OftManagerPrivate)
{
	messageHandler->registerHandler(ICQ_CAPABILITY_AIMSENDFILE, d.data());
	d->q = this;
	d->client = messageHandler->client();
}

Client *OftManager::client() const
{
	return d->client;
}

OftConnection *OftManager::send(const QString &uin, quint32 totalSize,
								quint16 filesCount, const QString &title)
{
	return new OftConnection(uin, false, Cookie::generateId(),
							 this, d->forceProxy, totalSize,
							 filesCount, title);
}

void OftManager::setAllowAnyServerPort(bool allowAnyServerPort)
{
	OftManagerPrivate::setAllowAnyServerPort(allowAnyServerPort);
}

void OftManager::setServerPorts(const QList<quint16> &ports)
{
	OftManagerPrivate::setServerPorts(ports);
}

} // namespace Ireen

#include "moc_oscarfiletransfer.cpp"
