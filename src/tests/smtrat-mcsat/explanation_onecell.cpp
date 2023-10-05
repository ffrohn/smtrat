#include <gtest/gtest.h>

#include <smtrat-common/logging.h>
#include <carl-logging/logging.h>
#include <carl-logging/logging-internals.h>
#include <smtrat-mcsat/explanations/onecell/onecell.h>
#include <smtrat-mcsat/explanations/onecell/Explanation.h>

TEST(smtrat_mcsat, onecell)
{
	if (!carl::logging::logger().has("stdout")) {
		carl::logging::logger().configure("stdout", std::cout);
	}
	carl::logging::logger().filter("stdout")
	 	("smtrat.cadcells", carl::logging::LogLevel::LVL_TRACE)
	 	("smtrat.mcsat.onecell", carl::logging::LogLevel::LVL_TRACE)
	;

	using P = smtrat::cadcells::Polynomial;

	auto var_s = carl::fresh_real_variable("s");
	auto var_p = carl::fresh_real_variable("p");
	auto var_d = carl::fresh_real_variable("d");
	auto var_b = carl::fresh_real_variable("b");
	smtrat::cadcells::VariableOrdering vrs({ var_s, var_p, var_d, var_b });
	smtrat::cadcells::Polynomial::ContextType ctx(vrs);

	smtrat::cadcells::Assignment ass;
	ass.emplace(var_s,0);
	ass.emplace(var_p,0);
	ass.emplace(var_d,0);

	auto poly_p = P(ctx, var_b) - P(ctx, 1);
	auto poly_q = P(ctx, var_b) + P(ctx, var_s) + (P(ctx, var_d) * P(ctx, var_p));

	auto constr_p = smtrat::cadcells::Constraint(poly_p, carl::Relation::EQ);
	auto constr_q = smtrat::cadcells::Constraint(poly_q, carl::Relation::EQ);
	std::vector<smtrat::cadcells::Atom> constrs;
	constrs.emplace_back(constr_p);
	constrs.emplace_back(constr_q);

	std::cout << "--- DEFAULT ---" << std::endl;
	auto res_default = smtrat::mcsat::onecell::onecell<smtrat::mcsat::onecell::LDBSettings>(constrs, ctx, ass);
	std::cout << res_default << std::endl;
	std::cout << "--- FILTERED ---" << std::endl;
	auto res_filtered = smtrat::mcsat::onecell::onecell<smtrat::mcsat::onecell::LDBFilteredAllSelectiveSettings>(constrs, ctx, ass);
	std::cout << res_filtered << std::endl;

	ASSERT_EQ(res_default, res_filtered);
}

struct OCSettings : smtrat::mcsat::onecell::BaseSettings {
	constexpr static auto cell_heuristic = smtrat::cadcells::representation::BIGGEST_CELL_FILTER;
	constexpr static auto covering_heuristic = smtrat::cadcells::representation::BIGGEST_CELL_COVERING_FILTER;
	constexpr static auto op = smtrat::cadcells::operators::op::mccallum_filtered_all;
};

TEST(smtrat_mcsat, onecell_filter_bug)
{
	if (!carl::logging::logger().has("stdout")) {
		carl::logging::logger().configure("stdout", std::cout);
	}
	carl::logging::logger().filter("stdout")
	 	("smtrat.cadcells", carl::logging::LogLevel::LVL_TRACE)
	 	("smtrat.mcsat.onecell", carl::logging::LogLevel::LVL_TRACE)
	;

	using P = smtrat::cadcells::Polynomial;

	// explanation: (g__AT0 <= 0 or (b.speed_y__AT1 > rootExpr(-2*b.y__AT0*g__AT0 + 2*b.y__AT1*g__AT0 + b.speed_y__AT1^2, 1, b.speed_y__AT1)) or b.y__AT0 + -1*b.y__AT1 <= 0 or -1*b.speed_y__AT4 + b.speed_y__AT1 < 0)
	// unsat cell [2: [1: (b.y__AT0 > rootExpr(1*b.y__AT0 + (-1*b.y__AT1), 1, b.y__AT0))], [1: (g__AT0 > rootExpr(1*g__AT0, 1, g__AT0))]]
	// of constraints [2: (b.speed_y__AT1 ! > rootExpr(1*b.speed_y__AT1^2 + ((-2*g__AT0)*b.y__AT0 + ((2*g__AT0)*b.y__AT1)), 1, b.speed_y__AT1)), -1*b.speed_y__AT1 + (1*b.speed_y__AT4) <= 0]
	// varorder [12: g__AT0, b.y__AT8, b.speed_y__AT8, b.speed_y__AT4, b.delta__AT4, b.delta__AT8, b.y__AT9, b.y__AT10, b.speed_y__AT5, b.y__AT1, b.y__AT0, b.speed_y__AT1]
	// assignment {b.speed_y__AT8 : 1, b.delta__AT8 : <8*x + (-1), (0, 1/4)>, b.y__AT9 : <16*x + (-1), (0, 1/4)>, b.y__AT0 : 10, b.y__AT10 : <16*x + (-1), (0, 1/4)>, b.speed_y__AT5 : 0, b.speed_y__AT4 : 1, b.y__AT1 : 5, b.y__AT8 : 0, b.delta__AT4 : <8*x + (-1), (0, 1/4)>, g__AT0 : 8}

	// x  g__AT0   8
	// y  b.speed_y__AT4   1
	// z  b.y__AT1   5
	// v  b.y__AT0   10
	// w  b.speed_y__AT1  unassigned
	auto x = carl::fresh_real_variable("x");
	auto y = carl::fresh_real_variable("y");
	auto z = carl::fresh_real_variable("z");
	auto v = carl::fresh_real_variable("v");
	auto w = carl::fresh_real_variable("w");

	smtrat::cadcells::VariableOrdering vrs({ x,y,z,v,w });
	smtrat::cadcells::Polynomial::ContextType ctx(vrs);

	smtrat::cadcells::Assignment ass;
	ass.emplace(x,8);
	ass.emplace(y,1);
	ass.emplace(z,5);
	ass.emplace(v,10);

	// (b.speed_y__AT1 ! > rootExpr(1*b.speed_y__AT1^2 + ((-2*g__AT0)*b.y__AT0 + ((2*g__AT0)*b.y__AT1)), 1, b.speed_y__AT1))
	// (w ! > rootExpr(1*w^2 + ((-2*x)*v + ((2*x)*z)), 1, w))
	auto poly_p = P(ctx, w) * P(ctx, w) - 2 * P(ctx, x) * P(ctx, v) + 2 * P(ctx, x) * P(ctx, z);
	auto mv_p = smtrat::cadcells::MultivariateRoot(poly_p, 1, w);
	auto varcomp_p = smtrat::cadcells::VariableComparison(w, mv_p, carl::Relation::GREATER, true);

	// -1*b.speed_y__AT1 + (1*b.speed_y__AT4) <= 0
	// -1*w + 1*y <=0
	auto poly_q = -1 * P(ctx, w) + P(ctx, y);
	auto constr_q = smtrat::cadcells::Constraint(poly_q, carl::Relation::LEQ);

	std::vector<smtrat::cadcells::Atom> constrs({varcomp_p, constr_q});

	auto res_filtered = smtrat::mcsat::onecell::onecell<OCSettings>(constrs, ctx, ass);
	std::cout << res_filtered << std::endl;
}

TEST(smtrat_mcsat, onecell_filter_bug_2)
{
	if (!carl::logging::logger().has("stdout")) {
		carl::logging::logger().configure("stdout", std::cout);
	}
	carl::logging::logger().filter("stdout")
	 	("smtrat.cadcells", carl::logging::LogLevel::LVL_TRACE)
	 	("smtrat.mcsat.onecell", carl::logging::LogLevel::LVL_TRACE)
	;

	using P = smtrat::cadcells::Polynomial;

	// explanation: (g__AT0 <= 0 or (b.delta__AT0 < rootExpr(-2*b.y__AT0 + 2*b.y__AT2 + b.delta__AT0^2*g__AT0, 2, b.delta__AT0)) or b.y__AT0 + -1*b.y__AT2 <= 0 or (b.delta__AT0 > rootExpr(b.speed_y__AT2 + b.delta__AT0*g__AT0, 1, b.delta__AT0)))
	// unsat cell [2: [1: (b.y__AT0 > rootExpr(1*b.y__AT0 + (-1*b.y__AT2), 1, b.y__AT0))], [1: (g__AT0 > rootExpr(1*g__AT0, 1, g__AT0))]]
	// of constraints [2: (b.delta__AT0 ! < rootExpr((1*g__AT0)*b.delta__AT0^2 + (-2*b.y__AT0 + (2*b.y__AT2)), 2, b.delta__AT0)), (b.delta__AT0 ! > rootExpr((1*g__AT0)*b.delta__AT0 + (1*b.speed_y__AT2), 1, b.delta__AT0))]
	// assignment {b.y__AT3 : 1, b.y__AT0 : 10, b.speed_y__AT3 : 0, b.delta__AT2 : 1/2, b.y__AT2 : 0, b.speed_y__AT2 : 4, g__AT0 : 8}
 	// varorder [8: b.y__AT2, g__AT0, b.y__AT3, b.speed_y__AT2, b.delta__AT2, b.speed_y__AT3, b.y__AT0, b.delta__AT0]


	// x  b.y__AT2        0
	// y  g__AT0          8
	// z  b.speed_y__AT2  4
	// v  b.y__AT0        10
	// w  b.delta__AT0    unassigned
	auto x = carl::fresh_real_variable("x");
	auto y = carl::fresh_real_variable("y");
	auto z = carl::fresh_real_variable("z");
	auto v = carl::fresh_real_variable("v");
	auto w = carl::fresh_real_variable("w");

	smtrat::cadcells::VariableOrdering vrs({ x,y,z,v,w });
	smtrat::cadcells::Polynomial::ContextType ctx(vrs);

	smtrat::cadcells::Assignment ass;
	ass.emplace(x,0);
	ass.emplace(y,8);
	ass.emplace(z,4);
	ass.emplace(v,10);

	// (b.delta__AT0 ! < rootExpr((1*g__AT0)*b.delta__AT0^2 + (-2*b.y__AT0 + (2*b.y__AT2)), 2, b.delta__AT0))
	// (w ! < rootExpr((1*y)*w^2 + (-2*v + (2*x)), 2, w))
	auto poly_p = P(ctx, y) * P(ctx, w) * P(ctx, w) - 2*P(ctx, v) + 2*P(ctx, x);
	auto mv_p = smtrat::cadcells::MultivariateRoot(poly_p, 2, w);
	// auto varcomp_p = smtrat::cadcells::VariableComparison(w, mv_p, carl::Relation::LESS, true);
	auto varcomp_p = smtrat::cadcells::VariableComparison(w, mv_p, carl::Relation::GEQ, false);

	// (b.delta__AT0 ! > rootExpr((1*g__AT0)*b.delta__AT0 + (1*b.speed_y__AT2), 1, b.delta__AT0))
	// (w ! > rootExpr((1*y)*w + (1*z), 1, w))
	auto poly_q = P(ctx, y) * P(ctx, w) + P(ctx, z);
	auto mv_q = smtrat::cadcells::MultivariateRoot(poly_q, 1, w);
	// auto varcomp_q = smtrat::cadcells::VariableComparison(w, mv_q, carl::Relation::GREATER, true);
	auto varcomp_q = smtrat::cadcells::VariableComparison(w, mv_q, carl::Relation::LEQ, false);

	std::vector<smtrat::cadcells::Atom> constrs({varcomp_p, varcomp_q});

	auto res_filtered = smtrat::mcsat::onecell::onecell<OCSettings>(constrs, ctx, ass);
	std::cout << res_filtered << std::endl;
	// wrong answer was: [2: [1: (v > rootExpr(1*v + (-1*x), 1, v))], [1: (y > rootExpr(1*y, 1, y))]]


	// x  0
	// y  1
	// z  -1
	// v  1/2
	// w  1
	smtrat::cadcells::Assignment ass2;
	ass2.emplace(x,0);
	ass2.emplace(y,1);
	ass2.emplace(z,-1);
	ass2.emplace(v,mpq_class(1)/2);
	//ass2.emplace(w,1);
	std::cout << carl::evaluate(mv_p, ass2) << std::endl;
	std::cout << carl::evaluate(mv_q, ass2) << std::endl;
}