#pragma once

#include <smtrat-modules/CoveringNGModule/CoveringNGModule.h>
#include <smtrat-modules/FPPModule/FPPModule.h>
#include <smtrat-solver/Manager.h>

namespace smtrat {
class CoveringNG_PP: public Manager {
public:
	CoveringNG_PP() : Manager() {
		setStrategy(
			addBackend<FPPModule<FPPSettings1>>({
                addBackend<CoveringNGModule<CoveringNGSettingsDefault>>()
            })
        );
	}
};
} // namespace smtrat
