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

#ifndef IREEN_METAFIELDS_H
#define IREEN_METAFIELDS_H

#include "metafield.h"

namespace Ireen {

const FieldNamesList &countries();
const FieldNamesList &interests();
const FieldNamesList &languages();
const FieldNamesList &pasts();
const FieldNamesList &genders();
const FieldNamesList &study_levels();
const FieldNamesList &industries();
const FieldNamesList &occupations();
const FieldNamesList &affilations();
const AgesList &ages();
const FieldNamesList &fields();
const FieldNamesList &fields_names();

} // namespace Ireen

#endif // IREEN_METAFIELDS_H

