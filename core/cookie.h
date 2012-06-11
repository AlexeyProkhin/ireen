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

#ifndef IREEN_COOKIE_H
#define IREEN_COOKIE_H

#include <QObject>
#include <QSharedPointer>
#include "dataunit.h"

namespace Ireen {

class Client;
class CookiePrivate;

class IREEN_EXPORT Cookie
{
	Q_DECLARE_PRIVATE(Cookie)
public:
	Cookie(bool generate = false);
	Cookie(Client *client, quint64 id);
	Cookie(Client *client, const QString &uin, quint64 id = 0);
	Cookie(const Cookie &cookie);
	Cookie &operator=(const Cookie &cookie);
	virtual ~Cookie();
	void lock(QObject *receiver = 0, const char *member = 0, int msec = 30000) const;
	bool unlock() const;
	bool isLocked() const;
	bool isEmpty() const;
	quint64 id() const;
	Client *client();
	void setClient(Client *client);
	QString uin() const;
	void setUin(const QString &uin);
	QObject *receiver();
	const char *member();
	static quint64 generateId();
private:
	QExplicitlySharedDataPointer<CookiePrivate> d_ptr;
};

template<>
struct toDataUnitHelper<Cookie>
{
	static inline QByteArray toByteArray(const Cookie &data)
	{
		return toDataUnitHelper<quint64>::toByteArray(data.id());
	}
};

template<>
struct fromDataUnitHelper<Cookie>
{
	static inline Cookie fromByteArray(const DataUnit &d)
	{
		return Cookie(d.read<quint64>());
	}
};

} // namespace Ireen

#endif // IREEN_COOKIE_H

