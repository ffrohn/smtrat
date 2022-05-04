#pragma once

#include "ValidationPoint.h"
#include "ValidationCollector.h"
#include <carl-io/SMTLIBStream.h>

#include <fstream>
#include <iostream>
#include <iomanip>

namespace smtrat {
namespace validation {

enum class ValidationOutputFormat {
	SMTLIB
};

template<ValidationOutputFormat SOF>
struct ValidationPrinter {};

template<ValidationOutputFormat SOF>
std::ostream& operator<<(std::ostream& os, ValidationPrinter<SOF>);

template<>
std::ostream& operator<<(std::ostream& os, ValidationPrinter<ValidationOutputFormat::SMTLIB>) {
	carl::io::SMTLIBStream sls;
	sls.setInfo("smt-lib-version", "2.0");
	for (const auto& s: ValidationCollector::getInstance().points()) {
		if (s->formulas().empty()) continue;
		int id = 0;
		for (const auto& kv: s->formulas()) {
			sls.reset();
			sls.comment(s->identifier() + " #" + std::to_string(id));
			sls.echo(s->identifier() + " #" + std::to_string(id));
			sls.setOption("interactive-mode", "true");
			sls.setInfo("status", (kv.second ? "sat" : "unsat"));
			#ifndef VALIDATION_STORE_STRINGS
			sls.declare(kv.first.logic());
			sls.declare(carl::variables(kv.first));
			sls.assertFormula(kv.first);
			#else
			sls << kv.first;
			#endif
			sls.getAssertions();
			sls.checkSat();
			id++;
		}
	}
	sls.exit();
	os << sls;
	return os;
}

auto validation_formulas_as_smtlib() {
	return ValidationPrinter<ValidationOutputFormat::SMTLIB>();
}

void validation_formulas_to_smtlib_file(const std::string& filename) {
	std::ofstream file;
	file.open(filename, std::ios::out);
	file << validation_formulas_as_smtlib();
	file.close();
}


}
}