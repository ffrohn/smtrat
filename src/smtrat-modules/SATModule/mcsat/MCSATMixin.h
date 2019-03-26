#pragma once

#include "../SolverTypes.h"

#include "BaseBackend.h"
#include "../ClauseChecker.h"

#include "MCSATStatistics.h"

#include <carl/formula/model/Assignment.h>

#include <smtrat-mcsat/smtrat-mcsat.h>

#include <functional>
#include <map>
#include <set>
#include <vector>

namespace smtrat {
namespace mcsat {

using carl::operator<<;
    
struct InformationGetter {
	std::function<Minisat::lbool(Minisat::Var)> getVarValue;
	std::function<Minisat::lbool(Minisat::Lit)> getLitValue;
	std::function<Minisat::lbool(Minisat::Var)> getBoolVarValue;
	std::function<int(Minisat::Var)> getDecisionLevel;
	std::function<int(Minisat::Var)> getTrailIndex;
	std::function<Minisat::CRef(Minisat::Var)> getReason;
	std::function<const Minisat::Clause&(Minisat::CRef)> getClause;
	std::function<const Minisat::vec<Minisat::CRef>&()> getClauses;
	std::function<const Minisat::vec<Minisat::CRef>&()> getLearntClauses;
	std::function<bool(Minisat::Var)> isTheoryAbstraction;
	std::function<bool(const FormulaT&)> isAbstractedFormula;
	std::function<Minisat::Var(const FormulaT&)> abstractVariable;
	std::function<const FormulaT&(Minisat::Var)> reabstractVariable;
	std::function<const FormulaT&(Minisat::Lit)> reabstractLiteral;
	std::function<const Minisat::vec<Minisat::Watcher>&(Minisat::Lit)> getWatches;
	std::function<Minisat::Var()> newVar;
};
struct TheoryLevel {
	/// Theory variable for this level
	carl::Variable variable = carl::Variable::NO_VARIABLE;
	/// Literal that assigns this theory variable
	Minisat::Lit decisionLiteral = Minisat::lit_Undef;
	/// Boolean variables univariate in this theory variable
	std::vector<Minisat::Var> decidedVariables;
};

template<typename Settings>
class MCSATMixin {
  
private:
	InformationGetter mGetter;
	
	/**
	 * The first entry of the stack always contains an entry for the non-theory
	 * variables: the variable is set to NO_VARIABLE and the lists contain
	 * variables that do not contain any theory variables.
	 */
	using TheoryStackT = std::vector<TheoryLevel>;
	TheoryStackT mTheoryStack;

	/// Variables that are not univariate in any variable yet.
	std::vector<Minisat::Var> mUndecidedVariables;
	
	MCSATBackend<Settings> mBackend;

	struct VarMapping {
		std::unordered_map<Minisat::Var, carl::Variable> minisatToCarl;
    	std::unordered_map<carl::Variable, Minisat::Var> carlToMinisat;

		void insert(const carl::Variable& carlVar, Minisat::Var minisatVar) {
			minisatToCarl.emplace(minisatVar, carlVar);
			carlToMinisat.emplace(carlVar, minisatVar);
		}

		bool has(Minisat::Var v) const {
			return minisatToCarl.find(v) != minisatToCarl.end();
		}

		bool has(const carl::Variable& v) const {
			return carlToMinisat.find(v) != carlToMinisat.end();
		}

		const carl::Variable& carlVar(Minisat::Var v) const {
			return minisatToCarl.at(v);
		}

		Minisat::Var minisatVar(const carl::Variable& v) const {
			return carlToMinisat.at(v);
		}

		std::vector<Minisat::Var> minisatVars() const {
			std::vector<Minisat::Var> res;
			for(auto it = minisatToCarl.begin(); it != minisatToCarl.end(); ++it) {
				res.push_back(it->first);
			}
			return res;
		}
	};
	VarMapping mTheoryVarMapping;

	#ifdef SMTRAT_DEVOPTION_Statistics
	MCSATStatistics*& mpStatistics;
	#endif

	/*
	struct ModelAssignmentCache { // TODO DYNSCHED model assignment cache
		ModelValues mContent;
		Model mModel;
		const Model& mBaseModel;

		ModelAssignmentCache(const Model& baseModel) : mBaseModel(baseModel) {
			mModel = mBaseModel;
		}

		bool empty() const {
			return mContent.empty();
		}

		void clear() {
			mContent.clear();
			mModel = mBaseModel;
		}

		void cache(const ModelValues& val) {
			assert(empty());
			mModel = mBaseModel;
			mContent = val;
			for (const auto& assignment : content()) {
				mModel.emplace(assignment.first, assignment.second);
			}
		}

		const ModelValues& content() const {
			return mContent;
		}

		const Model& model() const {
			return mModel;
		}
	};
	/// Cache for the next model assignemt(s)
	ModelAssignmentCache mModelAssignmentCache;
	*/

	struct VarProperties {
		boost::optional<std::size_t> maxDegree = boost::none;
	};
	/// Cache for static information about variables
	std::vector<VarProperties> mVarPropertyCache;

private:
	// ***** private helper methods

	/// Updates lookup for the current level
	void updateCurrentLevel();
	/// Pop a theory decision
	void popTheoryDecision();

	std::size_t varid(Minisat::Var var) const {
		assert(var >= 0);
		return static_cast<std::size_t>(var);
	}
public:
	
	template<typename BaseModule>
	explicit MCSATMixin(BaseModule& baseModule):
		mGetter({
			[&baseModule](Minisat::Var v){ return baseModule.value(v); },
			[&baseModule](Minisat::Lit l){ return baseModule.value(l); },
			[&baseModule](Minisat::Var l){ return baseModule.bool_value(l); },
			[&baseModule](Minisat::Var v){ return baseModule.vardata[v].level; },
			[&baseModule](Minisat::Var v){ return baseModule.vardata[v].mTrailIndex; },
			[&baseModule](Minisat::Var v){ return baseModule.reason(v); },
			[&baseModule](Minisat::CRef c) -> const auto& { return baseModule.ca[c]; },
			[&baseModule]() -> const auto& { return baseModule.clauses; },
			[&baseModule]() -> const auto& { return baseModule.learnts; },
			[&baseModule](Minisat::Var v){ return (baseModule.mBooleanConstraintMap.size() > v) && (baseModule.mBooleanConstraintMap[v].first != nullptr); },
			[&baseModule](const FormulaT& f) { return baseModule.mConstraintLiteralMap.count(f) > 0; },
			[&baseModule](const FormulaT& f) { return var(baseModule.mConstraintLiteralMap.at(f).front()); },
			[&baseModule](Minisat::Var v) -> const auto& { return baseModule.mBooleanConstraintMap[v].first->reabstraction; },
			[&baseModule](Minisat::Lit l) -> const auto& { return sign(l) ? baseModule.mBooleanConstraintMap[var(l)].second->reabstraction : baseModule.mBooleanConstraintMap[var(l)].first->reabstraction; },
			[&baseModule](Minisat::Lit l) -> const auto& { return baseModule.watches[l]; },
			[&baseModule]() -> Minisat::Var { baseModule.mBooleanConstraintMap.push( std::make_pair( nullptr, nullptr ) ); return baseModule.newVar(true,true,0,false); }
		}),
		mTheoryStack(1, TheoryLevel())
		#ifdef SMTRAT_DEVOPTION_Statistics
		,mpStatistics(baseModule.mpMCSATStatistics)
		#endif
		// mModelAssignmentCache(model())
	{}
	
	std::size_t level() const {
		return mTheoryStack.size() - 1;
	}
	const Model& model() const {
		return mBackend.getModel();
	}
	const std::vector<Minisat::Var>& undecidedBooleanVariables() const {
		return mUndecidedVariables;
	}
	bool isAssignedTheoryVariable(const carl::Variable& var) const {
		return std::find(mBackend.assignedVariables().begin(), mBackend.assignedVariables().end(), var) != mBackend.assignedVariables().end();
	}
	bool theoryAssignmentComplete() const {
		return mBackend.assignedVariables().size() == mBackend.variables().size();
	}
	/// Returns the respective theory level
	const TheoryLevel& get(std::size_t level) const {
		assert(level < mTheoryStack.size());
		return mTheoryStack[level];
	}
	/// Returns the current theory level
	const TheoryLevel& current() const {
		return mTheoryStack.back();
	}
	TheoryLevel& current() {
		return mTheoryStack.back();
	}
	/// Retrieve already decided theory variables
	carl::Variable variable(std::size_t level) const {
		SMTRAT_LOG_TRACE("smtrat.sat.mcsat", "Obtaining variable " << level);
		assert(level < mTheoryStack.size());
		if (level == 0) return carl::Variable::NO_VARIABLE;
		return get(level).variable;
	}
	
	// ***** Modifier
	
	/**
	 * Add a new constraint.
	 */
	void doBooleanAssignment(Minisat::Lit lit) {
		SMTRAT_LOG_DEBUG("smtrat.sat.mcsat", "Assigned " << lit);
		if (!mGetter.isTheoryAbstraction(var(lit))) return;
		const auto& f = mGetter.reabstractLiteral(lit);
		if (f.getType() == carl::FormulaType::VARASSIGN) {
			SMTRAT_LOG_DEBUG("smtrat.sat.mcsat", "Skipping assignment.");
			return;
		}
		/*
		if (!mModelAssignmentCache.empty()) {
			auto res = carl::model::evaluate(f, mModelAssignmentCache.model());
			if (!res.isBool() || !res.asBool()) {
				mModelAssignmentCache.clear(); // clear model assignment cache
			}
		}
		*/
		mBackend.pushConstraint(f);
	}
	/**
	 * Remove the last constraint. f must be the same as the one passed to the last call of pushConstraint().
	 */
	void undoBooleanAssignment(Minisat::Lit lit) {
		SMTRAT_LOG_DEBUG("smtrat.sat.mcsat", "Unassigned " << lit);
		if (!mGetter.isTheoryAbstraction(var(lit))) return;
		const auto& f = mGetter.reabstractLiteral(lit);
		if (f.getType() == carl::FormulaType::VARASSIGN) {
			SMTRAT_LOG_DEBUG("smtrat.sat.mcsat", "Skipping assignment.");
			return;
		}
		mBackend.popConstraint(f);
	}
	
	/// Add a variable, return the level it was inserted on
	std::size_t addBooleanVariable(Minisat::Var variable);

	// ***** Auxiliary getter
	
	/// Checks whether the given formula is univariate on the given level
	bool isFormulaUnivariate(const FormulaT& formula, std::size_t level) const;
	
	/// Push a theory decision
	void pushTheoryDecision(const FormulaT& assignment, Minisat::Lit decisionLiteral);
	/// Backtracks to the theory decision represented by the given literal. 
	bool backtrackTo(Minisat::Lit literal);	
	
	// ***** Helper methods
	
	/// Evaluate a literal in the theory, set lastReason to last theory decision involved.
	Minisat::lbool evaluateLiteral(Minisat::Lit lit) const;
	
	std::pair<bool, boost::optional<Explanation>> isBooleanDecisionFeasible(Minisat::Lit lit);
	
	boost::optional<Explanation> isFeasible(const carl::Variable& var) {
		SMTRAT_LOG_DEBUG("smtrat.sat.mcsat", "Checking whether trail is feasible (w.r.t. " << var << ")");
		/*
		AssignmentOrConflict res;
		if (!mModelAssignmentCache.empty()) {
			#ifdef SMTRAT_DEVOPTION_Statistics
			mpStatistics->modelAssignmentCacheHit();
			#endif
			SMTRAT_LOG_DEBUG("smtrat.sat.mcsat", "Found cached assignment.");
			return boost::none;
		} else { */
			auto res = mBackend.findAssignment(var);
			if (carl::variant_is_type<ModelValues>(res)) {
				// mModelAssignmentCache.cache(boost::get<ModelValues>(res));
				return boost::none;
			} else {
				const auto& confl = boost::get<FormulasT>(res);
				SMTRAT_LOG_DEBUG("smtrat.sat.mcsat", "Explaining " << confl);
				return mBackend.explain(var, confl);
			}
		// }
	}
	
	boost::variant<Explanation,FormulasT> makeTheoryDecision(const carl::Variable& var) {
		SMTRAT_LOG_DEBUG("smtrat.sat.mcsat", "Obtaining assignment");
		SMTRAT_LOG_DEBUG("smtrat.sat.mcsat", mBackend);
		AssignmentOrConflict res;
		// if (mModelAssignmentCache.empty()) {
			res = mBackend.findAssignment(var);
		/*} else {
			SMTRAT_LOG_DEBUG("smtrat.sat.mcsat", "Found cached assignment.");
			#ifdef SMTRAT_DEVOPTION_Statistics
			mpStatistics->modelAssignmentCacheHit();
			#endif
			res = mModelAssignmentCache.content();
			mModelAssignmentCache.clear();
		}*/
		if (carl::variant_is_type<ModelValues>(res)) {
			const auto& values = boost::get<ModelValues>(res);
			SMTRAT_LOG_DEBUG("smtrat.sat.mcsat", "-> " << values);
			FormulasT reprs;
			for (const auto& value : values) {
				FormulaT repr = carl::representingFormula(value.first, value.second);
				reprs.push_back(std::move(repr));
			}
			return reprs;
		} else {
			const auto& confl = boost::get<FormulasT>(res);
			auto explanation = mBackend.explain(var, confl);
			SMTRAT_LOG_DEBUG("smtrat.sat.mcsat", "Got a conflict: " << explanation);
			return explanation;
		}
	}

	/**
	 * Checks if the trail is consistent after the assignment on the current level.
	 * The trail must be consistent on the previous level.
	 * 
	 * If only_univariate is set to true, only syntactically univariate constraints are checked for consistency. This may
	 * lead to an inconsistent trail when considering all variables, but the trail is still consistent up to the currently
	 * decided theory variables.
	 * 
	 * Note that if we check consistency for all constraints, including those being not yet syntactically univariate,
	 * the explanation backends need to be able to eliminate all unassigned variables, otherwise we run into termination problems.
	 * 
	 * Returns boost::none if consistent and otherwise a conflicting Boolean variable.
	 */
	boost::optional<FormulaT> checkConsistency(bool only_univariate = false) { // TODO DYNSCHED make more efficient
		const auto& trail = mBackend.getTrail();
		SMTRAT_LOG_DEBUG("smtrat.sat.mcsat", "Checking trail against " << trail.model());
		auto evaluator = [&trail](const auto& c){
			auto res = carl::model::evaluate(c, trail.model());
			SMTRAT_LOG_DEBUG("smtrat.sat.mcsat", c << " evaluates to " << res);
			if (res.isBool()) {
				if (!res.asBool()) return false;
			}
			return true;
		};

		for (const auto& c: trail.constraints()) {
			const Minisat::Var& var = mGetter.abstractVariable(c);
			if (std::find(current().decidedVariables.begin(), current().decidedVariables.end(), var) == current().decidedVariables.end()) {
				continue;
			}
			if (only_univariate) {
				carl::Variables tvars;
				c.arithmeticVars(tvars);
				for (const auto& v : mBackend.assignedVariables())
					tvars.erase(v);
				if (tvars.size() > 1)
					continue;
			}
			if (!evaluator(c)) return c;
		}
		for (const auto& b: trail.activeMvBounds()) {
			const Minisat::Var& var = mGetter.abstractVariable(b);
			if (std::find(current().decidedVariables.begin(), current().decidedVariables.end(), var) == current().decidedVariables.end()) {
				continue;
			}
			if (only_univariate) {
				carl::Variables tvars;
				b.arithmeticVars(tvars);
				for (const auto& v : mBackend.assignedVariables())
					tvars.erase(v);
				if (tvars.size() > 1)
					continue;
			}
			if (!evaluator(b)) return b;
		}
		return boost::none;
	}

	/**
	 * Checks if the trail is consistent after the assignment on the current level.
	 * The trail must be consistent on the previous level.
	 * 
	 * Returns boost::none if consistent and an explanation.
	 */
	boost::optional<Explanation> isStillConsistent() {
		auto core = checkConsistency();
		if (core == boost::none) {
			SMTRAT_LOG_DEBUG("smtrat.sat.mcsat", "Trail is still consistent");
			return boost::none;
		} else {
			const auto& confl = boost::get<FormulaT>(core);
			SMTRAT_LOG_DEBUG("smtrat.sat.mcsat", "Inconsistent: " << confl << " evaluates to false");
			// pick any unassigned variable in confl (it must exist, otherwise the AssignmentFinder is incorrect)
			carl::Variables vars;
			confl.arithmeticVars(vars);
			for (const auto& avar : mBackend.assignedVariables())
				vars.erase(avar);
			assert(vars.size() > 0);
			auto explanation = mBackend.explain(*(vars.begin()), {confl});
			SMTRAT_LOG_DEBUG("smtrat.sat.mcsat", "Got a conflict: " << explanation);
			return explanation;
		}
	}
	
	Explanation explainTheoryPropagation(Minisat::Lit literal) {
		SMTRAT_LOG_DEBUG("smtrat.sat.mcsat", "Current state: " << (*this));
		SMTRAT_LOG_DEBUG("smtrat.sat.mcsat", "Explaining " << literal << " under " << mBackend.getModel());
		const auto& f = mGetter.reabstractLiteral(literal);

		carl::Variables vars;
		f.arithmeticVars(vars);
		for (const auto& v : mBackend.assignedVariables())
			vars.erase(v);
		assert(vars.size() == 1);
		carl::Variable tvar = *(vars.begin());

		auto conflict = mBackend.isInfeasible(tvar, !f);
		assert(carl::variant_is_type<FormulasT>(conflict));
		auto& confl = boost::get<FormulasT>(conflict);
		assert( std::find(confl.begin(), confl.end(), !f) != confl.end() );
		SMTRAT_LOG_DEBUG("smtrat.sat.mcsat", "Explaining " << f << " from " << confl);
		auto res = mBackend.explain(tvar, !f, confl);
		SMTRAT_LOG_DEBUG("smtrat.sat.mcsat", "Explaining " << f << " by " << res);
		// f is part of the conflict, because the trail is feasible without f:
		if (carl::variant_is_type<FormulaT>(res)) {
			if (boost::get<FormulaT>(res).isFalse()) {
				SMTRAT_LOG_DEBUG("smtrat.sat.mcsat", "Explanation failed.");
			} else {
				assert(boost::get<FormulaT>(res).contains(f));
			}
		}
		else {
			assert(boost::get<ClauseChain>(res).chain().back().clause().contains(f));
		}
		return res;
	}
		
	template<typename Constraints>
	void initVariables(const Constraints& c) {
		mBackend.initVariables(c);
		for (const carl::Variable& theoryVar : mBackend.variables()) {
			if (!mTheoryVarMapping.has(theoryVar)) {
				Minisat::Var minisatVar = mGetter.newVar();
				mTheoryVarMapping.insert(theoryVar, minisatVar);
			}
		}
	}

	bool isTheoryVar(Minisat::Var v) const {
		return mTheoryVarMapping.has(v);
	}

	const carl::Variable& theoryVar(Minisat::Var v) const { // TODO REFACTOR rename
		return mTheoryVarMapping.carlVar(v);
	}

	Minisat::Var minisatVar(const carl::Variable& v) const { // TODO REFACTOR rename
		return mTheoryVarMapping.minisatVar(v);
	}

	std::vector<Minisat::Var> theoryVarAbstractions() const {
		return mTheoryVarMapping.minisatVars();
	}

	// ***** Auxliary getter
	
	std::size_t theoryLevel(Minisat::Var var) const {
		if (!mGetter.isTheoryAbstraction(var)) {
			return 0;
		}
		return theoryLevel(mGetter.reabstractVariable(var));
	}
	
	std::size_t theoryLevel(const FormulaT& f) const { // TODO DYNSCHED theory levels are stored in decidedVariables => lookup more efficient?
		SMTRAT_LOG_TRACE("smtrat.sat.mcsat", "Computing theory level for " << f);
		carl::Variables vars;
		f.arithmeticVars(vars);
		if (vars.empty()) {
			SMTRAT_LOG_TRACE("smtrat.sat.mcsat", f << " has no variable, thus on level 0");
			return 0;
		}
		
		//for (std::size_t lvl = level(); lvl > 0; lvl--) {
		//	if (variable(lvl) == carl::Variable::NO_VARIABLE) continue;
		//	if (vars.count(variable(lvl)) > 0) {
		//		SMTRAT_LOG_TRACE("smtrat.sat.mcsat", f << " is univariate in " << variable(lvl));
		//		return lvl;
		//	}
		//}
		//SMTRAT_LOG_TRACE("smtrat.sat.mcsat", f << " contains undecided variables.");
		//return std::numeric_limits<std::size_t>::max();
	
		Model m = model();
		if (!carl::model::evaluate(f, m).isBool()) {
			SMTRAT_LOG_TRACE("smtrat.sat.mcsat", f << " is undecided.");
			return std::numeric_limits<std::size_t>::max();
		}
		for (std::size_t lvl = level(); lvl > 0; lvl--) {
			if (variable(lvl) == carl::Variable::NO_VARIABLE) continue;
			m.erase(variable(lvl));
			if (vars.count(variable(lvl)) == 0) continue;
			if (!carl::model::evaluate(f, m).isBool()) {
				return lvl;
			}
		}
		assert(false);
		return 0;
		
		for (std::size_t level = 1; level < mTheoryStack.size(); level++) {
			vars.erase(variable(level));
			if (vars.empty()) {
				SMTRAT_LOG_TRACE("smtrat.sat.mcsat", f << " is univariate in " << variable(level));
				return level;
			}
		}
		SMTRAT_LOG_TRACE("smtrat.sat.mcsat", f << " is undecided.");
		return std::numeric_limits<std::size_t>::max();
	}
	
	Minisat::Lit getDecisionLiteral(Minisat::Var var) const {
		if (!mGetter.isTheoryAbstraction(var)) {
			return Minisat::lit_Undef;
		}
		return getDecisionLiteral(mGetter.reabstractVariable(var));
	}
	Minisat::Lit getDecisionLiteral(const FormulaT& f) const {
		std::size_t level = theoryLevel(f);
		SMTRAT_LOG_DEBUG("smtrat.sat.mcsat", "Theory level of " << f << " is " << level);
		if (level >= mTheoryStack.size()) return Minisat::lit_Undef;
		return get(level).decisionLiteral;
	}
	
	int assignedAtTrailIndex(Minisat::Var variable) const {
		auto lit = getDecisionLiteral(variable);
		if (lit == Minisat::lit_Undef) {
			SMTRAT_LOG_DEBUG("smtrat.sat.mcsat", variable << " was not assigned yet.");
			return std::numeric_limits<int>::max();
		}
		return mGetter.getTrailIndex(var(lit));
	}
	
	int decisionLevel(Minisat::Var var) const {
		if (!mGetter.isTheoryAbstraction(var)) {
			return std::numeric_limits<int>::max();
		}
		return decisionLevel(mGetter.reabstractVariable(var));
	}
	
	int decisionLevel(const FormulaT& f) const {
		auto lit = getDecisionLiteral(f);
		if (lit == Minisat::lit_Undef) {
			return std::numeric_limits<int>::max();
		}
		return mGetter.getDecisionLevel(var(lit));
	}
	
	bool trailIsConsistent() const {
		const auto& trail = mBackend.getTrail();
		SMTRAT_LOG_DEBUG("smtrat.sat.mcsat", "Checking trail against " << trail.model());
		auto evaluator = [&trail](const auto& c){
			auto res = carl::model::evaluate(c, trail.model());
			SMTRAT_LOG_DEBUG("smtrat.sat.mcsat", c << " evaluates to " << res);
			if (res.isBool()) {
				if (!res.asBool()) return false;
			}
			return true;
		};
		for (const auto& c: trail.constraints()) {
			//auto category = mcsat::constraint_type::categorize(c, model(), carl::Variable::NO_VARIABLE);
			//if (category != mcsat::ConstraintType::Assigned) continue;
			if (!evaluator(c)) return false;
		}
		for (const auto& b: trail.activeMvBounds()) {
			//auto category = mcsat::constraint_type::categorize(b, model(), carl::Variable::NO_VARIABLE);
			//if (category != mcsat::ConstraintType::Assigned) continue;
			if (!evaluator(b)) return false;
		}
		return true;
	}

	std::size_t maxDegree(const Minisat::Var& var) {
		std::size_t v = varid(var);
		assert(v < mVarPropertyCache.size());

		if (mVarPropertyCache[v].maxDegree == boost::none) {
			if (!mGetter.isTheoryAbstraction(var)) {
				mVarPropertyCache[v].maxDegree = 0;
			} else {
				const auto& reabstraction = mGetter.reabstractVariable(var);
				if (reabstraction.getType() == carl::FormulaType::CONSTRAINT) {
					const auto& constr = reabstraction.constraint();
					carl::Variables vars;
					reabstraction.arithmeticVars(vars);
					std::size_t maxDeg = 0;
					for (const auto& tvar : vars) {
						std::size_t deg = constr.lhs().degree(tvar);
						if (deg > maxDeg) maxDeg = deg;
					}
					mVarPropertyCache[v].maxDegree = maxDeg;
				} else if (reabstraction.getType() == carl::FormulaType::VARCOMPARE) {
					mVarPropertyCache[v].maxDegree = std::numeric_limits<std::size_t>::max();
				} else {
					assert(false);
				}
				
			}
		}
		assert(mVarPropertyCache[v].maxDegree != boost::none);

		return *mVarPropertyCache[v].maxDegree;
	}

	std::vector<Minisat::Var> theoryVarsIn(const Minisat::Var& v) { // TODO DYNSCHED cache?
		if (!mGetter.isTheoryAbstraction(v)) {
			return std::vector<Minisat::Var>();
		}
		const auto& reabstraction = mGetter.reabstractVariable(v);

		carl::Variables tvars;
		reabstraction.arithmeticVars(tvars);
		std::vector<Minisat::Var> vars;
		for (const auto& tvar : tvars) {
			vars.push_back(minisatVar(tvar));
		}
		return vars;
	}

	// ***** Output
	/// Prints a single clause
	void printClause(std::ostream& os, Minisat::CRef clause) const;
	template<typename Sett>
	friend std::ostream& operator<<(std::ostream& os, const MCSATMixin<Sett>& mcm);
};

}
}

#include "MCSATMixin.tpp"
