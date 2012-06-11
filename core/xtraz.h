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

#ifndef IREEN_XTRAZ_H
#define IREEN_XTRAZ_H

#include "ireen_global.h"
#include "messages.h"

namespace Ireen {

class XtrazPrivate;
class XtrazRequestPrivate;
class XtrazResponsePrivate;

enum XtrazType
{
	xtrazInvitation = 0x0001,
	xtrazData       = 0x0002,
	xtrazUserRemove = 0x0004,
	xtrazNotify     = 0x0008
};

const Capability MSG_XSTRAZ_SCRIPT(0x3b60b3ef, 0xd82a6c45, 0xa4e09c5a, 0x5e67e865);

class IREEN_EXPORT XtrazRequest
{
public:
	XtrazRequest(const QString &serviceId = QString(), const QString &pluginId = QString());
	XtrazRequest(const XtrazRequest &data);
	~XtrazRequest();
	XtrazRequest &operator=(const XtrazRequest &rhs);
	QString pluginId() const;
	void setPluginId(const QString &pluginId);
	QString serviceId() const;
	void setServiceId(const QString &serviceId);
	QString value(const QString &name, const QString &def = QString()) const;
	bool contains(const QString &name) const;
	void setValue(const QString &name, const QString &value);
	SNAC snac(const QString &uin) const;
private:
	friend class Xtraz;
	QSharedDataPointer<XtrazRequestPrivate> d;
};

class IREEN_EXPORT XtrazResponse
{
public:
	XtrazResponse(const QString &serviceId = QString(), const QString &event = QString());
	XtrazResponse(const XtrazResponse &data);
	~XtrazResponse();
	XtrazResponse &operator=(const XtrazResponse &rhs);
	QString event() const;
	void setEvent(const QString &event);
	QString serviceId() const;
	void setServiceId(const QString &serviceId);
	QString value(const QString &name, const QString &def = QString()) const;
	bool contains(const QString &name) const;
	void setValue(const QString &name, const QString &value);
	SNAC snac(const QString &uin, quint64 cookie) const;
private:
	friend class Xtraz;
	QSharedDataPointer<XtrazResponsePrivate> d;
};

class IREEN_EXPORT Xtraz
{
public:
	enum Type
	{
		Response,
		Request,
		Invalid
	};
	Xtraz(const QString &message);
	~Xtraz();
	Xtraz &operator=(const Xtraz &rhs);
	Type type();
	XtrazRequest request();
	XtrazResponse response();
private:
	Xtraz();
	Xtraz(const Xtraz &xtraz);
	QSharedDataPointer<XtrazPrivate> d;
};

} // namespace Ireen

#endif // IREEN_XTRAZ_H

