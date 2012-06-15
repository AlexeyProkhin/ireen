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

#ifndef IREEN_CONNECTION_P_H
#define IREEN_CONNECTION_P_H

#include "abstractconnection.h"
#include "snac.h"
#include <QTimer>
#include <QDateTime>
#include <QQueue>

namespace Ireen {

#define MINIMIZE_RATE_MEMORY_USAGE 1

class ConnectionRate: public QObject
{
	Q_OBJECT
public:
	ConnectionRate(const SNAC &sn, AbstractConnection *conn);
	virtual ~ConnectionRate() {}
	void update(const SNAC &sn);
	quint16 groupId() { return m_groupId; }
	void send(const SNAC &snac, bool priority);
	bool isEmpty() { return m_windowSize <= 1; }
	bool testRate(bool priority);
	bool startTimeout();
protected:
	void timerEvent(QTimerEvent *event);
private:
	void sendNextPackets();
	quint32 getTimeDiff(const QDateTime &dateTime);
private:
	quint16 m_groupId;
	quint32 m_windowSize;
	quint32 m_clearLevel;
	quint32 m_currentLevel;
	quint32 m_maxLevel;
	quint32 m_lastTimeDiff;
#if !MINIMIZE_RATE_MEMORY_USAGE
	quint32 m_alertLevel;
	quint32 m_limitLevel;
	quint32 m_disconnectLevel;
	quint8 m_currentState;
#endif
	QDateTime m_time;
	QQueue<SNAC> m_lowPriorityQueue;
	QQueue<SNAC> m_highPriorityQueue;
	QBasicTimer m_timer;
	quint32 m_defaultPriority;
	AbstractConnection *m_conn;
};

class AbstractConnectionPrivate
{
public:
	inline quint16 seqNum() { return seqnum++; }
	inline quint32 nextId() { return id++; }
	bool sslMode;
	Socket *socket;
	FLAP flap;
	QMultiMap<quint32, SNACHandler*> handlers;
	quint16 seqnum;
	quint32 id;
	ClientInfo clientInfo;
	QHostAddress ext_ip;
	QList<quint16> services;
	QHash<quint16, ConnectionRate*> rates;
	QHash<quint32, ConnectionRate*> ratesHash;
	AbstractConnection::ConnectionError error;
	QString errorStr;
	AbstractConnection::State state;
	QSet<SNACInfo> initSnacs; // Snacs that are allowed when initializing connection
	QTimer aliveTimer;
private:
	friend class AbstractConnection;
	void init(AbstractConnection *q);
};

} // namespace Ireen

#endif //IREEN_CONNECTION_P_H

