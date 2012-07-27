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

#ifndef IREEN_ABSTRACTCONNECTION_H
#define IREEN_ABSTRACTCONNECTION_H

#if IREEN_SSL_SUPPORT
#include <QSslSocket>
#else
#include <QTcpSocket>
#endif

#include <QMap>
#include <QHostAddress>
#include "core/snachandler.h"
#include "core/flap.h"

namespace Ireen {

class ConnectionRate;
class AbstractConnectionPrivate;

struct IREEN_EXPORT ProtocolError
{
public:
	ProtocolError(const SNAC &snac);
	qint16 code() { return m_code; }
	qint16 subcode() { return m_subcode; }
	QString errorString();
	TLVMap tlvs() const { return m_tlvs; }
protected:
	qint16 m_code;
	qint16 m_subcode;
	TLVMap m_tlvs;
};

struct IREEN_EXPORT ClientInfo
{
	QByteArray id_string;
	quint16 id_number;
	quint16 major_version;
	quint16 minor_version;
	quint16 lesser_version;
	quint16 build_number;
	quint32 distribution_number;
	QByteArray language;
	QByteArray country;
};

struct IREEN_EXPORT DirectConnectionInfo
{
	QHostAddress internal_ip;
	QHostAddress external_ip;
	quint32 port;
	quint8 dc_type;
	quint16 protocol_version;
	quint32 auth_cookie;
	quint32 web_front_port;
	quint32 client_features;
	quint32 info_utime; // last info update time (unix time_t)
	quint32 extinfo_utime; // last ext info update time (i.e. icqphone status)
	quint32 extstatus_utime; // last ext status update time (i.e. phonebook)
};

#if IREEN_SSL_SUPPORT
typedef QSslSocket Socket;
#else
typedef QTcpSocket Socket;
#endif


class IREEN_EXPORT AbstractConnection : public QObject, public SNACHandler
{
	Q_OBJECT
	Q_INTERFACES(Ireen::SNACHandler)
	Q_DECLARE_PRIVATE(AbstractConnection)
public:
	enum ConnectionError
	{
		NoError = 0x00,
		InvalidNickOrPassword = 0x01,
		ServiceUnaivalable = 0x02,
		IncorrectNickOrPassword = 0x04,
		MismatchNickOrPassword = 0x05,
		InternalClientError = 0x06,
		InvalidAccount = 0x07,
		DeletedAccount = 0x08,
		ExpiredAccount = 0x09,
		NoAccessToDatabase = 0x0a,
		NoAccessToResolver = 0x0b,
		InvalidDatabaseFields = 0x0c,
		BadDatabaseStatus = 0x0D,
		BadResolverStatus = 0x0E,
		InternalError = 0x0F,
		ServiceOffline = 0x10,
		SuspendedAccount = 0x11,
		DBSendError = 0x12,
		DBLinkError = 0x13,
		ReservationMapError = 0x14,
		ReservationLinkError = 0x15,
		ConnectionLimitExceeded = 0x16,
		ConnectionLimitExceededReservation = 0x17,
		RateLimitExceededReservation = 0x18,
		UserHeavilyWarned = 0x19,
		ReservationTimeout = 0x1a,
		ClientUpgradeRequired = 0x1b,
		ClientUpgradeRecommended = 0x1c,
		RateLimitExceeded = 0x1d,
		IcqNetworkError = 0x1e,
		InvalidSecirID = 0x20,
		AgeLimit = 0x22,
		AnotherClientLogined = 0x80,
		SocketError = 0x81,
		HostNotFound = 0x82
	};
	enum State
	{
		Unconnected,
		Connecting,
		Connected
	};

public:
	explicit AbstractConnection(QObject *parent = 0);
	virtual ~AbstractConnection();
	void registerHandler(SNACHandler *handler);
	void send(SNAC &snac, bool priority = true);
	void sendSnac(quint16 family, quint16 subtype, bool priority = true);
	bool testRate(quint16 family, quint16 subtype, bool priority = true);
	virtual void disconnectFromHost(bool force = false);
	const QHostAddress &externalIP() const;
	const QList<quint16> &servicesList();
	Socket *socket(); // FIXME: Make it private
	const Socket *socket() const;
	QNetworkProxy proxy() const;
	ConnectionError error();
	QString errorString();
	const ClientInfo &clientInfo();
	State state() const;
	void registerInitializationSnacs(const QList<SNACInfo> &snacs, bool append = true);
	void registerInitializationSnac(quint16 family, quint16 subtype);
public slots:
	void setProxy(const QNetworkProxy &proxy);
signals:
	void error(Ireen::AbstractConnection::ConnectionError error);
	void disconnected();
	void proxyUpdated(const QNetworkProxy &proxy);
protected:
	AbstractConnection(AbstractConnectionPrivate *d, QObject *parent);
	const FLAP &flap();
	void send(FLAP &flap);
	quint32 sendSnac(SNAC &snac);
	void setSeqNum(quint16 seqnum);
	virtual void processNewConnection();
	virtual void processCloseConnection();
	virtual void onDisconnect();
	virtual void onError(ConnectionError error);
	void setError(ConnectionError error, const QString &errorStr = QString());
	virtual void handleSNAC(AbstractConnection *conn, const SNAC &snac);
	void setState(AbstractConnection::State state);
	static quint16 generateFlapSequence();
private slots:
	void processSnac();
	void readData();
	void stateChanged(QAbstractSocket::SocketState);
	void error(QAbstractSocket::SocketError);
	void sendAlivePacket();
protected:
	friend class ConnectionRate;
	QScopedPointer<AbstractConnectionPrivate> d_ptr;
};

} // namespace Ireen

#endif // IREEN_ABSTRACTCONNECTION_H

