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

#include "cookie.h"
#include "client_p.h"
#include <QDateTime>
#include <QTimer>

namespace Ireen {

class CookiePrivate: public QSharedData
{
public:
	CookiePrivate(quint64 _id = 0):
		id(_id), member(0)
	{
		timer.setSingleShot(true);
	}

	quint64 id;
	Client *client;
	QString uin;
	mutable QObject *receiver;
	mutable QLatin1String member;
	mutable QTimer timer;
};

Cookie::Cookie(bool generate):
	d_ptr(new CookiePrivate)
{
	Q_D(Cookie);
	if (generate)
		d->id = generateId();
	else
		d->id = 0;
	d->client = 0;
}

Cookie::Cookie(Client *client, quint64 id):
	d_ptr(new CookiePrivate(id))
{
	Q_D(Cookie);
	d->client = client;
}

Cookie::Cookie(Client *client, const QString &uin, quint64 id):
	d_ptr(new CookiePrivate(id))
{
	Q_D(Cookie);
	d->uin = uin;
	d->client = client;
}

Cookie::Cookie(const Cookie &cookie):
	d_ptr(cookie.d_ptr)
{

}

Cookie &Cookie::operator=(const Cookie &cookie)
{
	d_ptr = cookie.d_ptr;
	return *this;
}

Cookie::~Cookie()
{
}

void Cookie::lock(QObject *receiver, const char *member, int msec) const
{
	Q_D(const Cookie);
	Q_ASSERT(d->client);
	Q_ASSERT(!isEmpty());
	d->client->d_func()->cookies.insert(d->id, *this);
	d->timer.setProperty("cookieId", d->id);
	QObject::connect(&d->timer, SIGNAL(timeout()), d->client, SLOT(onCookieTimeout()));
	d->receiver = receiver;
	d->member = QLatin1String(member);
	d->timer.start(msec);
}

bool Cookie::unlock() const
{
	Q_D(const Cookie);
	Q_ASSERT(d->client);
	Cookie cookie = d->client->d_func()->cookies.take(d->id);
	if (!cookie.isEmpty()) {
		cookie.d_func()->timer.stop();
		d->receiver = 0;
		d->member = QLatin1String(0);
		return true;
	} else {
		return false;
	}
}

bool Cookie::isLocked() const
{
	Q_D(const Cookie);
	Q_ASSERT(d->client);
	return d->client->d_func()->cookies.contains(d->id);
}

bool Cookie::isEmpty() const
{
	return d_func()->id == 0;
}

quint64 Cookie::id() const
{
	return d_func()->id;
}

Client *Cookie::client()
{
	return d_func()->client;
}

void Cookie::setClient(Client *client)
{
	Q_D(Cookie);
	Q_ASSERT(!d->client || !isLocked());
	d->client = client;
}

QString Cookie::uin() const
{
	return d_func()->uin;
}

void Cookie::setUin(const QString &uin)
{
	d_func()->uin = uin;
}

QObject *Cookie::receiver()
{
	return d_func()->receiver;
}

const char *Cookie::member()
{
	return d_func()->member.latin1();
}

quint64 Cookie::generateId()
{
	static quint64 id = 10000;
	return ++id;
}

} // namespace Ireen

