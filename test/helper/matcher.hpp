#pragma once

#include <gmock/gmock.h>

#include <expected>
#include <optional>

#include "./printer.hpp"

MATCHER(ExpectedHasValue, "expected has value") {
	return arg.has_value();
}

MATCHER(ExpectedHasError, "expected has error") {
	return not arg.has_value();
}

MATCHER(OptionalHasValue, "optional has value") {
	return arg.has_value();
}

MATCHER(OptionalHasNoValue, "optional has no value") {
	return not arg.has_value();
}
