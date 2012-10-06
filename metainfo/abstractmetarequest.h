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

#ifndef IREEN_ABSTRACTMETAREQUEST_H
#define IREEN_ABSTRACTMETAREQUEST_H

#include "metafield.h"
#include "../core/dataunit.h"
#include "../ireen_global.h"

namespace Ireen {

class AbstractMetaRequestPrivate;
class MetaInfo;
class Client;

class IREEN_EXPORT AbstractMetaRequest : public QObject
{
	Q_OBJECT
	Q_DECLARE_PRIVATE(AbstractMetaRequest)
	Q_DISABLE_COPY(AbstractMetaRequest)
public:
	enum ErrorType {
		NoError,
		Canceled,
		Timeout,
		ProtocolError
	};

	AbstractMetaRequest();
	virtual ~AbstractMetaRequest();
	quint16 id() const;
	Client *client() const;
	bool isDone() const;
	void setTimeout(int msec);
	ErrorType errorType();
	QString errorString();
signals:
	void done(bool ok);
public slots:
	virtual void send() const = 0;
	void cancel();
private slots:
	void timeout();
protected:
	friend class MetaInfo;
	AbstractMetaRequest(MetaInfo *metaInfo, AbstractMetaRequestPrivate *d);
	virtual bool handleData(quint16 type, const DataUnit &data) = 0;
	void sendRequest(quint16 type, const DataUnit &data) const;
	void close(bool ok, ErrorType error = NoError, const QString &errorString = QString());
protected:
	QScopedPointer<AbstractMetaRequestPrivate> d_ptr;
};

QString readSString(const DataUnit &data, Client *client);

} // namespace Ireen

#endif // IREEN_ABSTRACTMETAREQUEST_H

