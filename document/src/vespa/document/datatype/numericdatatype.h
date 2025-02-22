// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
/**
 * \class document::NumericDataType
 * \ingroup datatype
 *
 * \brief Data type holding numbers of various types.
 *
 * Data type object allowing you to store a number. This is typically only
 * created when initializing the global primitive datatypes in the DataType
 * class.
 */
#pragma once

#include "primitivedatatype.h"

namespace document {

class NumericDataType : public PrimitiveDataType {
public:
    NumericDataType(Type type);

    NumericDataType* clone() const override { return new NumericDataType(*this); }
    void print(std::ostream&, bool verbose, const std::string& indent) const override;

    DECLARE_IDENTIFIABLE_ABSTRACT(NumericDataType);
};

}


