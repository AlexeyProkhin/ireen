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

#ifndef IREEN_METAFIELD_H
#define IREEN_METAFIELD_H

#include <QHash>
#include "ireen_global.h"

namespace Ireen {

struct Category
{
	QString category;
	QString keyword;
};
typedef QList<Category> CategoryList;
typedef QHash<quint16, QString> FieldNamesList;
typedef QHash<quint32, QString> AgesList;

enum GenderEnum
{
	Unknown,
	Female = 1,
	Male = 2,
	Female2 = 'F',
	Male2 = 'M'
};

enum MetaFieldEnum {
	Uin,
	// Basic info
	Nick,
	FirstMetaField = Nick,
	FirstName,
	LastName,
	Email,
	HomeCity,
	HomeState,
	HomePhone,
	HomeFax,
	HomeAddress,
	CellPhone,
	HomeZipCode,
	HomeCountry,
	GMT,
	AuthFlag,
	WebawareFlag,
	DirectConnectionFlag,
	PublishPrimaryEmailFlag,
	// More info
	Age,
	Gender,
	Homepage,
	Birthday,
	Languages,
	OriginalCity,
	OriginalState,
	OriginalCountry,
	// Work info
	WorkCity,
	WorkState,
	WorkPhone,
	WorkFax,
	WorkAddress,
	WorkZip,
	WorkCountry,
	WorkCompany,
	WorkDepartment,
	WorkPosition,
	WorkOccupation,
	WorkWebpage,
	// Other
	Emails,
	Notes,
	Interests,
	Pasts,
	Affilations,
	LastMetaField = Affilations,

	AgeRange,
	Whitepages,
	OnlineFlag
};


namespace Country {
enum Enum {
	OtherCountry = 9999,
	Afghanistan = 93,
	Albania = 355,
	Algeria = 213,
	Andorra = 376,
	Angola = 244,
	Anguilla = 101,
	AntiguaAndBarbuda = 1021,
	Antilles = 5902,
	Argentina = 54,
	Armenia = 374,
	Aruba = 297,
	AscensionIsland = 247,
	Australia = 61,
	Austria = 43,
	Azerbaijan = 994,
	Bahamas = 103,
	Bahrain = 973,
	Bangladesh = 880,
	Barbados = 104,
	Barbuda = 120,
	Belarus = 375,
	Belgium = 32,
	Belize = 501,
	Benin = 229,
	Bermuda = 105,
	Bhutan = 975,
	Bolivia = 591,
	BosniaandHerzegovina = 387,
	Botswana = 267,
	Brazil = 55,
	BritishVirginIslands = 106,
	Brunei = 673,
	Bulgaria = 359,
	BurkinaFaso = 226,
	Burundi = 257,
	Cambodia = 855,
	Cameroon = 237,
	Canada = 107,
	CanaryIslands = 178,
	CapeVerdeIslands = 238,
	CaymanIslands = 108,
	CentralAfricanRepublic = 236,
	Chad = 235,
	Chile = 56,
	China = 86,
	ChristmasIsland = 672,
	CocosIslands = 6102,
	Colombia = 57,
	Comoros = 2691,
	Congo = 242,
	CookIslands = 682,
	CostaRica = 506,
	CoteDIvoire = 225,
	Croatia = 385,
	Cuba = 53,
	Cyprus = 357,
	CzechRepublic = 42,
	Denmark = 45,
	DiegoGarcia = 246,
	Djibouti = 253,
	Dominica = 109,
	DominicanRepublic = 110,
	Ecuador = 593,
	Egypt = 20,
	ElSalvador = 503,
	EquatorialGuinea = 240,
	Eritrea = 291,
	Estonia = 372,
	Ethiopia = 251,
	FaeroeIslands = 298,
	FalklandIslands = 500,
	Fiji = 679,
	Finland = 358,
	France = 33,
	FrenchAntilles = 5901,
	FrenchGuiana = 594,
	FrenchPolynesia = 689,
	Gabon = 241,
	Gambia = 220,
	Georgia = 995,
	Germany = 49,
	Ghana = 233,
	Gibraltar = 350,
	Greece = 30,
	Greenland = 299,
	Grenada = 111,
	Guadeloupe = 590,
	Guam = 671,
	Guatemala = 502,
	Guinea = 224,
	Guinea_Bissau = 245,
	Guyana = 592,
	Haiti = 509,
	Honduras = 504,
	HongKong = 852,
	Hungary = 36,
	Iceland = 354,
	India = 91,
	Indonesia = 62,
	Iran = 98,
	Iraq = 964,
	Ireland = 353,
	Israel = 972,
	Italy = 39,
	Jamaica = 112,
	Japan = 81,
	Jordan = 962,
	Kazakhstan = 705,
	Kenya = 254,
	Kiribati = 686,
	Kuwait = 965,
	Kyrgyzstan = 706,
	Laos = 856,
	Latvia = 371,
	Lebanon = 961,
	Lesotho = 266,
	Liberia = 231,
	LibyanArabJamahiriya = 218,
	Liechtenstein = 4101,
	Lithuania = 370,
	Luxembourg = 352,
	Macau = 853,
	Macedonia = 389,
	Madagascar = 261,
	Malawi = 265,
	Malaysia = 60,
	Maldives = 960,
	Mali = 223,
	Malta = 356,
	MarshallIslands = 692,
	Martinique = 596,
	Mauritania = 222,
	Mauritius = 230,
	MayotteIsland = 269,
	Mexico = 52,
	Micronesia = 691,
	Moldova = 373,
	Monaco = 377,
	Mongolia = 976,
	Montserrat = 113,
	Morocco = 212,
	Mozambique = 258,
	Myanmar = 95,
	Namibia = 264,
	Nauru = 674,
	Nepal = 977,
	Netherlands = 31,
	NetherlandsAntilles = 599,
	Nevis = 114,
	NewCaledonia = 687,
	NewZealand = 64,
	Nicaragua = 505,
	Niger = 227,
	Nigeria = 234,
	Niue = 683,
	NorfolkIsland = 6722,
	NorthKorea = 850,
	Norway = 47,
	Oman = 968,
	Pakistan = 92,
	Palau = 680,
	Panama = 507,
	PapuaNewGuinea = 675,
	Paraguay = 595,
	Peru = 51,
	Philippines = 63,
	Poland = 48,
	Portugal = 351,
	PuertoRico = 121,
	Qatar = 974,
	ReunionIsland = 262,
	Romania = 40,
	RotaIsland = 6701,
	Russia = 7,
	Rwanda = 250,
	SaintHelena = 290,
	SaintKitts = 115,
	SaintKittsandNevis = 1141,
	SaintLucia = 122,
	SaintPierreandMiquelon = 508,
	SaintVincentandtheGrenadines = 116,
	SaipanIsland = 670,
	Samoa = 684,
	SanMarino = 378,
	SaoTomeAndPrincipe = 239,
	SaudiArabia = 966,
	Scotland = 442,
	Senegal = 221,
	Seychelles = 248,
	SierraLeone = 232,
	Singapore = 65,
	Slovakia = 4201,
	Slovenia = 386,
	SolomonIslands = 677,
	Somalia = 252,
	SouthAfrica = 27,
	SouthKorea = 82,
	Spain = 34,
	SriLanka = 94,
	Sudan = 249,
	Suriname = 597,
	Swaziland = 268,
	Sweden = 46,
	Switzerland = 41,
	SyrianArabRepublic = 963,
	Taiwan = 886,
	Tajikistan = 708,
	Tanzania = 255,
	Thailand = 66,
	TinianIsland = 6702,
	Togo = 228,
	Tokelau = 690,
	Tonga = 676,
	TrinidadandTobago = 117,
	Tunisia = 216,
	Turkey = 90,
	Turkmenistan = 709,
	TurksandCaicosIslands = 118,
	Tuvalu = 688,
	Uganda = 256,
	Ukraine = 380,
	UnitedArabEmirates = 971,
	UnitedKingdom = 44,
	Uruguay = 598,
	USA = 1,
	Uzbekistan = 711,
	Vanuatu = 678,
	VaticanCity = 379,
	Venezuela = 58,
	VietNam = 84,
	VirginIslandsOfUnitedStates = 123,
	Wales = 441,
	WallisAndFutunaIslands = 681,
	WesternSamoa = 685,
	Yemen = 967,
	Yugoslavia = 381,
	Yugoslavia_Montenegro = 382,
	Yugoslavia_Serbia = 3811,
	Zaire = 243,
	Zambia = 260,
	Zimbabwe = 263
}; }

namespace Language {
enum Enum {
	Afrikaans = 55,
	Albanian = 58,
	Arabic = 1,
	Armenian = 59,
	Azerbaijani = 68,
	Belorussian = 72,
	Bhojpuri = 2,
	Bosnian = 56,
	Bulgarian = 3,
	Burmese = 4,
	Cantonese = 5,
	Catalan = 6,
	Chamorro = 61,
	Chinese = 7,
	Croatian = 8,
	Czech = 9,
	Danish = 10,
	Dutch = 11,
	English = 12,
	Esperanto = 13,
	Estonian = 14,
	Farsi = 15,
	Finnish = 16,
	French = 17,
	Gaelic = 18,
	German = 19,
	Greek = 20,
	Gujarati = 70,
	Hebrew = 21,
	Hindi = 22,
	Hungarian = 23,
	Icelandic = 24,
	Indonesian = 25,
	Italian = 26,
	Japanese = 27,
	Khmer = 28,
	Korean = 29,
	Kurdish = 69,
	Lao = 30,
	Latvian = 31,
	Lithuanian = 32,
	Macedonian = 65,
	Malay = 33,
	Mandarin = 63,
	Mongolian = 62,
	Norwegian = 34,
	Persian = 57,
	Polish = 35,
	Portuguese = 36,
	Punjabi = 60,
	Romanian = 37,
	Russian = 38,
	Serbian = 39,
	Sindhi = 66,
	Slovak = 40,
	Slovenian = 41,
	Somali = 42,
	Spanish = 43,
	Swahili = 44,
	Swedish = 45,
	Tagalog = 46,
	Taiwanese = 64,
	Tamil = 71,
	Tatar = 47,
	Thai = 48,
	Turkish = 49,
	Ukrainian = 50,
	Urdu = 51,
	Vietnamese = 52,
	Welsh = 67,
	Yiddish = 53,
	Yoruba = 54
}; }

namespace Interest {
enum Enum {
	Art = 100,
	Astronomy = 128,
	AudioandVisual = 147,
	Business = 125,
	BusinessServices = 146,
	Cars = 101,
	CelebrityFans = 102,
	Clothing = 130,
	Collections = 103,
	Computers = 104,
	Culture = 105,
	Ecology = 122,
	Eighties = 136,
	Entertainment = 139,
	Fifties = 137,
	FinanceandCorporate = 138,
	Fitness = 106,
	HealthandBeauty = 142,
	Hobbies = 108,
	HomeAutomation = 150,
	HouseholdProducts = 144,
	Games = 107,
	Government = 124,
	ICQ_Help = 109,
	Internet = 110,
	Lifestyle = 111,
	MailOrderCatalog = 145,
	Media = 143,
	MoviesandTV = 112,
	Music = 113,
	Mystics = 126,
	NewsandMedia = 123,
	Outdoors = 114,
	Parenting = 115,
	Parties = 131,
	PetsandAnimals = 116,
	Publishing = 149,
	Religion = 117,
	RetailStores = 141,
	Science = 118,
	Seventies = 135,
	Sixties = 134,
	Skills = 119,
	Socialscience = 133,
	Space = 129,
	SportingandAthletic = 148,
	Sports = 120,
	Travel = 127,
	WebDesign = 121,
	Women = 132
}; }

namespace Past {
enum Enum {
	ElementarySchool = 300,
	HighSchool = 301,
	College = 302,
	University = 303,
	Military = 304,
	PastWorkPlace = 305,
	PastOrganization = 306,
	OtherPast = 399
}; }

namespace StudyLevel {
enum Enum {
	AssociatedDegree = 4,
	BachelorsDegree = 5,
	ElementaryDegree = 1,
	HighSchoolDegree = 2,
	MastersDegree = 6,
	PhD = 7,
	PostDoctoral = 8,
	UniversityOrCollegeDegree = 3
}; }

namespace Industry {
enum Enum {
	Agriculture = 2,
	Arts = 3,
	Construction = 4,
	ConsumerGoods = 5,
	CorporateServices = 6,
	Education = 7,
	Finance = 8,
	Government = 9,
	HighTech = 10,
	Legal = 11,
	Manufacturing = 12,
	Media = 13,
	MedicalAndHealthCare = 14,
	NonProfitOrganizationManagement = 15,
	OtherIndustry = 19,
	RecreationTravelAndEntertainment = 16,
	ServiceIndustry = 17,
	Transportation = 18
}; }

namespace Occupation {
enum Enum {
	Academic = 1,
	Administrative = 2,
	ArtAndEntertainment = 3,
	CollegeStudent = 4,
	Computers = 5,
	CommunityAndSocial = 6,
	Education = 7,
	Engineering = 8,
	FinancialServices = 9,
	Government = 10,
	HighSchoolStudent = 11,
	Home = 12,
	ICQ_ProvidingHelp = 13,
	Law = 14,
	Managerial = 15,
	Manufacturing = 16,
	MedicalAndHealth = 17,
	Military = 18,
	Non_GovernmentOrganization = 19,
	Professional = 20,
	Retail = 21,
	Retired = 22,
	ScienceAndResearch = 23,
	Sports = 24,
	Technical = 25,
	UniversityStudent = 26,
	WebBuilding = 27,
	OtherServices = 99
}; }

namespace Affiliation {
enum Enum {
	AlumniOrg = 200,
	CharityOrg = 201,
	ClubAndSocialOrg = 202,
	CommunityOrg = 203,
	CulturalOrg = 204,
	FanClubs = 205,
	FraternityAndSorority = 206,
	HobbyistsOrg = 207,
	InternationalOrg = 208,
	NatureandEnvironmentOrg = 209,
	ProfessionalOrg = 210,
	ScientifiAndTechnicalOrg = 211,
	SelfImprovementGroup = 212,
	SpiritualAndReligiousOrg = 213,
	SportsOrg = 214,
	SupportOrg = 215,
	TradeandBusinessOrg = 216,
	Union = 217,
	VolunteerOrg = 218,
	OtherAffiliation = 299
}; }

namespace MaritalStatus {
enum Enum {
	Single = 10,
	CloseRelationships = 11,
	Engaged = 12,
	Married = 20,
	Divorced = 30,
	Separated = 31,
	Widowed = 40,
	OpenRelationship = 50,
	OtherMaritalStatus = 255
}; }

class IREEN_EXPORT MetaFieldKey
{
public:
	MetaFieldKey(const QString &name);
	MetaFieldKey(int value);
	MetaFieldKey(const MetaFieldKey &field);
	QString name() const;
	MetaFieldEnum value() const { return m_value; };
	QString toString() const;
	operator QString() const { return toString(); }
	MetaFieldKey &operator=(int value) { m_value = static_cast<MetaFieldEnum>(value); return *this; }
	bool operator==(int value) const { return m_value == value; }
	bool operator!=(int value) const { return m_value != value; }
	bool operator==(const MetaFieldKey &value) const { return m_value == value.m_value; }
	bool operator!=(const MetaFieldKey &value) const { return m_value != value.m_value; }
private:
	mutable QString m_name;
	MetaFieldEnum m_value;
};
typedef QHash<MetaFieldKey, QVariant> MetaInfoValuesHash;

QDebug operator<<(QDebug dbg, const Category &cat);

} // namespace Ireen

inline uint qHash(const Ireen::MetaFieldKey &key)
{
	return qHash(static_cast<int>(key.value()));
}

Q_DECLARE_METATYPE(Ireen::Category);
Q_DECLARE_METATYPE(Ireen::CategoryList);


#endif // IREEN_METAFIELD_H

