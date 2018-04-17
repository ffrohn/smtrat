/**
 * @file GBPP.cpp
 * @author YOUR NAME <YOUR EMAIL ADDRESS>
 *
 * @version 2018-03-09
 * Created on 2018-03-09.
 */

#include "GBPPModule.h"

namespace smtrat
{
	template<class Settings>
	GBPPModule<Settings>::GBPPModule(const ModuleInput* _formula, RuntimeSettings*, Conditionals& _conditionals, Manager* _manager):
		PModule( _formula, _conditionals, _manager )
#ifdef SMTRAT_DEVOPTION_Statistics
		, mStatistics(Settings::moduleName)
#endif
	{
		simplifyInequalityFunction = std::bind(&GBPPModule<Settings>::simplifyInequality, this, std::placeholders::_1);
	}
	
	template<class Settings>
	GBPPModule<Settings>::~GBPPModule()
	{}
	
	template<class Settings>
	void GBPPModule<Settings>::updateModel() const
	{
		mModel.clear();
		if( solverState() == Answer::SAT )
		{
			// Your code.
		}
	}
	
	template<class Settings>
	Answer GBPPModule<Settings>::checkCore()
	{
		mEqualities.clear();
		mEqualityComplexity = 0;
		mBasis.reset();
		
		// Extract top-level Equalities
		for (const auto& f: rReceivedFormula()) {
			if (f.formula().getType() == carl::FormulaType::CONSTRAINT) {
				if (f.formula().constraint().relation() == carl::Relation::EQ) {
					SMTRAT_LOG_DEBUG("smtrat.gbpp", "Found equality " << f.formula().constraint());
					mEqualities.emplace(f.formula().constraint());
					mEqualityComplexity += f.formula().constraint().lhs().complexity();
				}
			}
		}
		
		// Compute GBasis
		for (const auto& eq: mEqualities) {
			SMTRAT_LOG_DEBUG("smtrat.gbpp", "Adding to Gröbner Basis: " << gpoly(eq.constraint().lhs().normalize()));
			mBasis.addPolynomial(gpoly(eq.constraint().lhs().normalize()));
		}
		mBasis.calculate();
		SMTRAT_LOG_DEBUG("smtrat.gbpp", "Constructed Gröbner Basis:" << std::endl << mBasis.getIdeal());
		
		// Simplify all inequalities w.r.t. GBasis and forward to backend
		carl::FormulaVisitor<FormulaT> visitor;
		for (const auto& f: rReceivedFormula()) {
			if (mEqualities.find(f.formula()) != mEqualities.end()) continue;
			auto res = visitor.visitResult(f.formula(), simplifyInequalityFunction);
			
			if (res != f.formula()) {
				SMTRAT_LOG_INFO("smtrat.gbpp", "Reduced " << f.formula() << " to " << res);
			}
			if (!res.isTrue()) {
				addSubformulaToPassedFormula(res, f.formula());
			}
		}
		
		// Forward basis to backend
		std::size_t basisComplexity = 0;
		for (const auto& p: mBasis.getIdeal().getGenerators()) {
			basisComplexity += p.complexity();
		}
		if (basisComplexity >= mEqualityComplexity) {
			for (const auto& f: mEqualities) {
				addSubformulaToPassedFormula(f, f);
			}
		} else {
			for (const auto& p: mBasis.getIdeal().getGenerators()) {
				addSubformulaToPassedFormula(FormulaT(ConstraintT(Poly(p), carl::Relation::EQ)));
			}
		}
		
		Answer ans = runBackends();
		if (ans == UNSAT) {
			generateTrivialInfeasibleSubset();
		}
		return ans;
	}
	
	template<typename Settings>
	FormulaT GBPPModule<Settings>::simplifyInequality(const FormulaT& formula) const {
		if (formula.getType() != carl::FormulaType::CONSTRAINT) return formula;
		assert(mEqualities.find(formula) == mEqualities.end());
		const auto& c = formula.constraint();
		
		typename Settings::Reductor reductor(mBasis.getIdeal(), gpoly(c.lhs()));
		auto reduced = reductor.fullReduce();
		SMTRAT_LOG_DEBUG("smtrat.gbpp", "Reduced " << c.lhs() << " to " << reduced);
		
		if (reduced.nrTerms() >= c.lhs().nrTerms()) return formula;
		return FormulaT(ConstraintT(Poly(reduced), c.relation()));
	}
}

#include "Instantiation.h"
