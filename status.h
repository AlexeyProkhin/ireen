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

#ifndef IREEN_STATUS_H
#define IREEN_STATUS_H

#include "capability.h"

namespace Ireen {

typedef QHash<QString, Capability> CapabilityHash;

class IREEN_EXPORT Status
{
public:
	enum StatusEnum
	{
		// Status FLAGS (used to determine status of other users)
		Online         = 0x0000,
		Away           = 0x0001,
		DND            = 0x0002,
		NA             = 0x0004,
		Occupied       = 0x0010,
		Invisible      = 0x0100,
		// Ireen Status Flags
		Offline        = 0x8000
	};

	Status(quint16 id = Online, const QString &text = QString()) :
		m_id(id),
		m_text(text)
	{}
	quint16 id() const { return m_id; }
	void setId(quint16 id) { m_id = id; }
	QString text() const { return m_text; }
	void setText(const QString &text) { m_text = text; }
	void addCapability(const QString &name, const Capability &cap) { m_caps.insert(name, cap); }
	void removeCapability(const QString &name) { m_caps.remove(name); }
	bool hasCapability(const QString &name) { return m_caps.contains(name); }
	CapabilityHash capabilities() const { return m_caps; }
	bool operator==(quint16 id) { return m_id == id; }
	void operator=(quint16 id) { m_caps.clear(); m_text.clear(); m_id = id; }
	static QSet<QString> allSupportedCapabilityTypes();
	static void registerCapabilityType(const QString &type);
private:
	quint16 m_id;
	QString m_text;
	CapabilityHash m_caps;
};

} // namespace Ireen

#endif // IREEN_STATUS_H
