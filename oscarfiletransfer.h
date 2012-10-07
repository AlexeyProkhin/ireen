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

#ifndef IREEN_OSCARFILETRANSFER_H
#define IREEN_OSCARFILETRANSFER_H

#include "ireen_global.h"
#include "messagehandler.h"
#include <QDateTime>

class QHostAddress;

namespace Ireen {

class Client;
class OftManager;
class OftConnectionPrivate;
class OftManagerPrivate;

class OftConnection : public QObject
{
	Q_OBJECT
public:
	enum ErrorType { NetworkError, Canceled, NotSupported, IOError, NoError };
	enum State { Initiation, AwaitingIODevice, Started, Finished, Error };

	virtual ~OftConnection();
	QString uin() const;
	quint64 cookie() const;
	OftManager *manager() const;
	Client *client() const;
	quint16 localPort() const;
	quint16 remotePort() const;
	QHostAddress remoteAddress() const;

	QString title() const;
	quint32 totalSize() const;
	quint16 filesCount() const;
	QString currentFileName() const;
	quint32 currentFileSize() const;
	quint16 currentFileIndex() const;

	void send();
	void cancel();
	void accept();
	void resumeSending(QIODevice *outputDevice, const QString &name, quint32 size,
					   const QDateTime &lastModified = QDateTime());
	void resumeReceiving(QIODevice *inputDevice);
signals:
	void stateChanged(Ireen::OftConnection::State state);
	void error(Ireen::OftConnection::ErrorType error, const QString &desc);
	void progress(quint32 bytes);
	void currentFileChanged();
	void localPortChanged(quint16 port);
	void remotePortChanged(quint16 port);
	void remoteAddressChanged(const QHostAddress &addr);
private:
	Q_PRIVATE_SLOT(d, void close())
	Q_PRIVATE_SLOT(d, void startNextStage())
	Q_PRIVATE_SLOT(d, void sendFileRequest())
	Q_PRIVATE_SLOT(d, void connected())
	Q_PRIVATE_SLOT(d, void onError(QAbstractSocket::SocketError))
	Q_PRIVATE_SLOT(d, void onHeaderReaded())
	Q_PRIVATE_SLOT(d, void onNewData())
	Q_PRIVATE_SLOT(d, void onSendData())
	Q_PRIVATE_SLOT(d, void resumeSendingImpl(quint32 checksum))
	Q_PRIVATE_SLOT(d, void resumeReceivingImpl(quint32 checksum))
	Q_PRIVATE_SLOT(d, void resumeFileReceivingImpl(quint32 checksum))
private:
	OftConnection(const QString &uin, bool isIncoming,
						  quint64 cookie, OftManager *manager,
						  bool forceProxy, quint32 totalSize = 0,
						  quint16 filesCount = 1,
						  const QString &title = QString());

	friend class OftServer;
	friend class OftManager;
	friend class OftManagerPrivate;
	friend class OftConnectionPrivate;
	QScopedPointer<OftConnectionPrivate> d;
};

class OftManager : public QObject
{
	Q_OBJECT
public:
	explicit OftManager(MessageHandler *messageHandler);
	Client *client() const;
	OftConnection *send(const QString &uin, quint32 totalSize,
						quint16 filesCount = 1,
						const QString &title = QString());
	static void setAllowAnyServerPort(bool allowAnyServerPort);
	static void setServerPorts(const QList<quint16> &ports);
signals:
	void incomingConnection(Ireen::OftConnection *connection);
private:
	friend class OftConnection;
	friend class OftManagerPrivate;
	QScopedPointer<OftManagerPrivate> d;
};


} // namespace Ireen

#endif // IREEN_OSCARFILETRANSFER_H
