#include "privacylists.h"

namespace Ireen {

class PrivacyListsPrivate
{
public:
	Feedbag *feedbag;
};

static Visibility itemToVisibility(const FeedbagItem &item)
{
	if (item.isNull())
		return NoVisibility;
	Visibility visibility = (Visibility)item.field<quint8>(0x00CA);
	if (visibility >= NoVisibility && visibility <= AllowPermitList)
		return visibility;
	else
		return NoVisibility;
}

PrivacyLists::PrivacyLists(Feedbag *feedbag) :
	QObject(feedbag), d(new PrivacyListsPrivate)
{
	d->feedbag = feedbag;
	m_types << SsiPermit << SsiDeny << SsiIgnore << SsiVisibility;
	feedbag->registerHandler(this);
}

PrivacyLists::~PrivacyLists()
{
}

Visibility PrivacyLists::visibility()
{
	FeedbagItem item = d->feedbag->itemByType(SsiVisibility);
	return itemToVisibility(item);
}

void PrivacyLists::setVisibility(Visibility visibility)
{
	FeedbagItem item = d->feedbag->itemByType(SsiVisibility, Feedbag::CreateItem);
	TLV data = item.field(0x00CA);
	if (data.read<quint8>() != visibility) {
		item.setField<quint8>(0x00CA, visibility);
		item.updateOrAdd();
	}
}

bool PrivacyLists::isInList(const QString &uin, PrivacyListType list)
{
	Q_ASSERT(list == VisibleList || list == InvisibleList || list == IgnoreList);
	return d->feedbag->containsItem(list, uin);
}

void PrivacyLists::addToList(const QString &uin, PrivacyListType list)
{
	Q_ASSERT(list == VisibleList || list == InvisibleList || list == IgnoreList);
	FeedbagItem item = d->feedbag->item(list, uin, 0, Feedbag::CreateItem);
	if (!item.isInList())
		item.add();
}

void PrivacyLists::removeFromList(const QString &uin, PrivacyListType list)
{
	Q_ASSERT(list == VisibleList || list == InvisibleList || list == IgnoreList);
	FeedbagItem item = d->feedbag->item(list, uin);
	if (item.isInList())
		item.remove();
}

QString PrivacyLists::visibilityToString(Visibility visibility)
{
	switch (visibility) {
	case AllowAllUsers:
		return QT_TRANSLATE_NOOP("Privacy", "Visible for all");
	case BlockAllUsers:
		return QT_TRANSLATE_NOOP("Privacy", "Invisible for all");
	case AllowPermitList:
		return QT_TRANSLATE_NOOP("Privacy", "Visible only for visible list");
	case BlockDenyList:
		return QT_TRANSLATE_NOOP("Privacy", "Invisible only for invisible list");
	case AllowContactList:
		return QT_TRANSLATE_NOOP("Privacy", "Visible only for contact list");
	default:
		return QT_TRANSLATE_NOOP("Privacy", "Unknown privacy");
	}
}

bool PrivacyLists::handleFeedbagItem(Feedbag *feedbag, const FeedbagItem &item, Feedbag::ModifyType type, FeedbagError error)
{
	Q_UNUSED(feedbag);
	if (error != FeedbagError::NoError)
		return false;

	bool isItemAdded = type != Feedbag::Remove;
	PrivacyListType listType;
	switch (item.type()) {
	case SsiVisibility: {
		Visibility newVisibility = isItemAdded
				? itemToVisibility(item)
				: NoVisibility;
		emit visibilityChanged(newVisibility);
		return true;
	}
	case SsiPermit: {
		if (isItemAdded)
			debug() << item.name() << "has been added to visible list";
		else
			debug() << item.name() << "has been removed from visible list";
		listType = VisibleList;
		break;
	}
	case SsiDeny: {
		if (isItemAdded)
			debug() << item.name() << "has been added to invisible list";
		else
			debug() << item.name() << "has been removed from invisible list";
		listType = InvisibleList;
		break;
	}
	case SsiIgnore: {
		if (isItemAdded)
			debug() << item.name() << "has been added to ignore list";
		else
			debug() << item.name() << "has been removed from ignore list";
		listType = IgnoreList;
		break;
	default:
		return false;
	}
	}

	if (isItemAdded)
		emit contactAdded(item.name(), listType);
	else
		emit contactRemoved(item.name(), listType);
	return true;
}

} // namespace Ireen
