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

#ifndef IREEN_FILETRANSFER_H
#define IREEN_FILETRANSFER_H

#include "oscarfiletransfer.h"
#include "messagehandler.h"
#include "client.h"

#include <QTcpSocket>
#include <QFile>
#include <QFileInfo>
#include <QTcpServer>
#include <QWeakPointer>
#include <QHostInfo>
#include <QThread>

namespace Ireen {

const int FILETRANSFER_WAITING_TIMEOUT = 15000;

enum OftPacketType
{
	OftPrompt = 0x0101,
	OftAcknowledge = 0x0202,
	OftDone = 0x0204,
	OftReceiverResume = 0x0205,
	OftSenderResume = 0x0106,
	OftResumeAcknowledge = 0x0207,
	OftSender = 0x0100,
	OftReceiver = 0x0200
};

struct OftHeader
{
public:
	OftHeader();
	void readData(QIODevice *dev, QTextCodec *asciiCodec);
	void writeData(QIODevice *dev);
	bool isFinished() { return m_state == Finished; }
public:
	OftPacketType type;
	quint64 cookie;
	bool encrypt;
	bool compress;
	quint16 totalFiles;
	quint16 filesLeft;
	quint16 totalParts;
	quint16 partsLeft;
	quint32 totalSize;
	quint32 size;
	quint32 modTime;
	quint32 checksum;
	quint32 receivedResourceForkChecksum;
	quint32 resourceForkSize;
	quint32 creationTime;
	quint32 resourceForkChecksum;
	quint32 bytesReceived;
	quint32 receivedChecksum;
	QString identification;
	quint8 flags;
	QByteArray macFileInfo;
	QString fileName;
private:
	enum State {
		ReadHeader,
		ReadData,
		Finished
	} m_state;
	quint16 m_length;
	QByteArray m_data;
};

class OftSocket : public QTcpSocket
{
	Q_OBJECT
public:
	enum ReadingState {
		ProxyInit = 0x1001,
		ProxyReceive = 0x1002,
		Proxy = 0x1000,
		ReadHeader = 0x0003,
		ReadData = 0x0004
	};
	explicit OftSocket(Client *client, QObject *parent = 0);
	OftSocket(Client *client, int socketDescriptor, QObject *parent = 0);
	const OftHeader &lastHeader() const { return m_lastHeader; }
	OftHeader &lastHeader() { return m_lastHeader; }
	ReadingState readingState() const { return m_state; }
	void directConnect(const QHostAddress &addr, quint16 port);
	void proxyConnect();
	void proxyConnect(QHostAddress addr, quint16 port);
	const QHostAddress &proxyIP() { return m_proxyIP; }
	quint16 proxyPort() { return m_proxyPort; }
	quint16 clientPort() { return m_clientPort; }
	void setCookie(quint64 cookie) { m_cookie = cookie; }
signals:
	void proxyInitialized();
	void initialized();
	void headerReaded(const OftHeader &header);
	void newData();
	void timeout();
public slots:
	void dataReaded();
protected:
	void connectToProxy(const QHostAddress &address, quint16 port);
private slots:
	void onReadyRead();
	void connected();
	void disconnected();
	void onTimeout();
	void proxyFound(const QHostInfo &proxyInfo);
private:
	void init();
	ReadingState m_state;
	OftHeader m_lastHeader;
	Client *m_client;
	quint64 m_cookie;
	QHostAddress m_proxyIP;
	quint16 m_proxyPort;
	quint16 m_len;
	int m_hostReqId;
	QTimer m_timer;
	quint16 m_clientPort;
};

class OftServer : public QTcpServer
{
	Q_OBJECT
public:
	explicit OftServer(quint16 port);
	void listen();
	void close();
	void setConnection(OftConnection *conn);
	OftConnection *conn() { return m_conn; }
signals:
	void closed(OftConnection *conn);
	void timeout(OftConnection *conn);
protected:
	void incomingConnection(int socketDescriptor);
private slots:
	void onTimeout();
private:
	OftConnection *m_conn;
	quint16 m_port;
	QTimer m_timer;
};

class OftChecksumThread : public QThread
{
	Q_OBJECT
public:
	OftChecksumThread(QIODevice *file, int bytes = 0);
	static quint32 chunkChecksum(const char *buffer, int len, quint32 checksum, int offset);
protected:
	void run();
signals:
	void done(quint32 checksum);
private:
	QIODevice *file;
	int bytes;
};

class OftConnectionPrivate
{
public:
	void close(bool error);
	void handleRendezvous(quint16 reqType, const TLVMap &tlvs);
	void setSocket(OftSocket *socket);
	void waitIODevice();
	void startFileSending();
	void resumeReceivingImpl(bool resume);
	void setError(OftConnection::ErrorType error, const QString &desc = QString());
	void setState(OftConnection::State state);
// slots:
	void close() { close(true); }
	void startNextStage();
	void sendFileRequest();
	void connected();
	void onError(QAbstractSocket::SocketError);
	void onHeaderReaded();
	void onNewData();
	void onSendData();
	void resumeSendingImpl(quint32 checksum);
	void resumeReceivingImpl(quint32 checksum);
	void resumeFileReceivingImpl(quint32 checksum);
public:
	bool isIncoming;
	bool isAccepted;
	bool proxy;
	bool connInited;
	quint16 filesCount;
	quint16 currentFileIndex;
	quint16 stage;
	quint32 totalSize;
	quint32 currentFileSize;
	quint64 cookie;
	OftConnection *q;
	OftConnection::State state;
	OftConnection::ErrorType error;
	QString errorDesc;
	QString title;
	QString currentFileName;
	QString uin;
	QDateTime lastModified;
	Pointer<OftSocket> socket;
	Pointer<OftServer> server;
	QScopedPointer<QIODevice> data;
	OftManager *manager;
	OftHeader header;
	QHostAddress clientVerifiedIP;
};

class OftManagerPrivate : public MessagePlugin
{
public:
	virtual void processMessage(const QString &uin, const Capability &guid,
								const QByteArray &data, quint16 reqType,
								const Cookie &cookie);
	OftConnection *connection(quint64 cookie);
	void addConnection(OftConnection *connection);
	void removeConnection(OftConnection *connection);
public:
	static OftServer *getFreeServer();
	static void deleteOftServer(OftServer *server);
	static void setAllowAnyServerPort(bool allowAnyServerPort);
	static void setServerPorts(const QList<quint16> &ports);
public:
	bool forceProxy;
	OftManager *q;
	Client *client;
	QHash<quint64, OftConnection*> connections;
	static QHash<quint16, OftServer*> servers;
	static bool allowAnyPort;
};

} // namespace Ireen

#endif // IREEN_FILETRANSFER_H

