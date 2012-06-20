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

#ifndef IREEN_PRIVACYLISTS_H
#define IREEN_PRIVACYLISTS_H

#include "feedbag.h"

namespace Ireen {

class PrivacyListsPrivate;

enum Visibility
{
	NoVisibility     = 0,
	AllowAllUsers    = 1,
	BlockAllUsers    = 2,
	AllowPermitList  = 3,
	BlockDenyList    = 4,
	AllowContactList = 5
};

enum PrivacyListType
{
	InvalidType,
	VisibleList   = SsiPermit,
	InvisibleList = SsiDeny,
	IgnoreList    = SsiIgnore
};

class IREEN_EXPORT PrivacyLists : public QObject, public FeedbagItemHandler
{
	Q_OBJECT
	Q_DISABLE_COPY(PrivacyLists)
public:
	PrivacyLists(Feedbag *feedbag);
	virtual ~PrivacyLists();
	Visibility visibility();
	void setVisibility(Visibility visibility);
	bool isInList(const QString &uin, Ireen::PrivacyListType list);
	void addToList(const QString &uin, Ireen::PrivacyListType list);
	void removeFromList(const QString &uin, Ireen::PrivacyListType list);
	static QString visibilityToString(Visibility visibility);
signals:
	void contactAdded(const QString &uin, Ireen::PrivacyListType list);
	void contactRemoved(const QString &uin, Ireen::PrivacyListType list);
	void visibilityChanged(Ireen::Visibility visibility);
protected:
	bool handleFeedbagItem(Feedbag *feedbag, const FeedbagItem &item, Feedbag::ModifyType type, FeedbagError error);
private:
	QScopedPointer<PrivacyListsPrivate> d;
};

} // namespace Ireen

#endif // IREEN_PRIVACYLISTS_H
