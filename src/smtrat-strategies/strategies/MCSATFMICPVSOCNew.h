#pragma once

#include <smtrat-solver/Manager.h>

//#include "../modules/FPPModule/FPPModule.h"
#include <smtrat-modules/SATModule/SATModule.h>

namespace smtrat
{
	class MCSATFMICPVSOCNew: public Manager
	{
		public:
			MCSATFMICPVSOCNew(): Manager() { // TODO change to PP version later
				setStrategy(
						addBackend<SATModule<SATSettingsMCSATFMICPVSOCNew>>()
				);
			}
	};
}	// namespace smtrat
