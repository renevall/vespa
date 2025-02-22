// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
/**
 * \class document::ShortFieldValue
 * \ingroup fieldvalue
 *
 * \brief Wrapper for field values of datatype SHORT.
 */
#pragma once

#include "numericfieldvalue.h"
#include <vespa/document/datatype/numericdatatype.h>

namespace document {

class ShortFieldValue : public NumericFieldValue<int16_t> {
public:
    typedef std::unique_ptr<ShortFieldValue> UP;
    typedef int16_t Number;

    ShortFieldValue(Number value = 0)
        : NumericFieldValue<Number>(value) {}

    void accept(FieldValueVisitor &visitor) override { visitor.visit(*this); }
    void accept(ConstFieldValueVisitor &visitor) const override { visitor.visit(*this); }

    const DataType *getDataType() const override { return DataType::SHORT; }
    ShortFieldValue* clone() const override { return new ShortFieldValue(*this); }

    using NumericFieldValue<Number>::operator=;

    DECLARE_IDENTIFIABLE(ShortFieldValue);

};

} // document

