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

#include "snac.h"
#include "util.h"

namespace Ireen {

SNAC::SNAC(quint16 family, quint16 subtype) :
	m_receiver(0), m_member("")
{
	reset(family, subtype);
}

SNAC::~SNAC()
{
}

void SNAC::reset(quint16 family, quint16 subtype)
{
	m_family = family;
	m_subtype = subtype;
	m_flags = 0;
	m_data.clear();
}

SNAC SNAC::fromByteArray(const QByteArray &d)
{
	DataUnit data(d);
	quint16 family = data.read<quint16>();
	quint16 subtype = data.read<quint16>();
	SNAC snac(family, subtype);
	snac.m_flags = data.read<quint16>();
	snac.m_id = data.read<quint32>();
	snac.m_data = data.readAll();
	if (snac.m_flags & 0x8000) {
		// Some unknown data
		int offset = snac.read<quint16>() + 2; // sizeof(quint16)
		snac.m_data = QByteArray::fromRawData(snac.m_data.constData() + offset,
											  snac.m_data.size() - offset);
	}
	return snac;
}

QByteArray SNAC::toByteArray() const
{
	QByteArray data = header();
	data += m_data;
	return data;
}

QByteArray SNAC::header() const
{
	DataUnit data;
	data.append<quint16>(m_family);
	data.append<quint16>(m_subtype);
	data.append<quint16>(m_flags);
	data.append<quint32>(m_id);
	return data.data();
}

void SNAC::setCookie(const Cookie &cookie, QObject *receiver, const char *member, int msec)
{
	m_cookie = cookie;
	m_receiver = receiver;
	if (member)
		m_member = QLatin1String(member);
	m_msec = msec;
}

void SNAC::lock()
{
	if (!m_cookie.isEmpty()) {
		m_cookie.lock(m_receiver, m_member.latin1(), m_msec);
		m_member = QLatin1String("");
	}
}

} // namespace Ireen

