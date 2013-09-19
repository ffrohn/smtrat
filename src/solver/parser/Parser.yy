/*
 *  SMT-RAT - Satisfiability-Modulo-Theories Real Algebra Toolbox
 * Copyright (C) 2012 Florian Corzilius, Ulrich Loup, Erika Abraham, Sebastian Junges
 *
 * This file is part of SMT-RAT.
 *
 * SMT-RAT is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SMT-RAT is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SMT-RAT.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


/**
 * @file Parser.yy
 *
 * @author Florian Corzilius
 * @author Ulrich Loup
 * @since 2012-03-19
 * @version 2013-05-05
 */

/* TODOS: some setter and getter of the smtlib language miss.
 *        resolve the shift/reduce
 *        use unordered_maps where possible (watch out for the stored iterators)
 *        remove useless code in the driver
 *        shorten code in the Parser.yy by outsourcing in the driver
 *        rename the class Driver
 *        there is some kind of minus and plus where I didn't expect it to be (TM/p6-zenonumeric_s9.smt2)
 */
%{ /* C/C++ Declarations */

#include <stdio.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <ginac/ginac.h>
#include <lib/Formula.h>

%}

/*
 * yacc/bison Declarations
 */

// Require bison 2.3 or later
%require "2.3"

// Add debug output code to generated parser. disable this for release versions.
%debug

// Start symbol is named "start"
%start start

// write out a header file containing the token defines
%defines

// use newer C++ skeleton file
%skeleton "lalr1.cc"

// namespace to enclose parser in
%name-prefix="smtrat"

// Set the parser's class identifier
%define "parser_class_name" "Parser"

// Keep track of the current position within the input
%locations
%initial-action
{
    // initialize the initial location object
    @$.begin.filename = @$.end.filename = dv.pStreamname();
    struct YYLTYPE
    {
        int first_line;
        int first_column;
        int last_line;
        int last_column;
    };
};

/*
 * The driver is passed by reference to the parser and to the scanner. This
 * provides a simple but effective pure interface, not relying on global
 * variables.
 */
%parse-param { class Driver& dv }

// Verbose error messages
%error-verbose

// Symbols.

%union
{
   unsigned                                              eval;
   std::string*                                          sval;
   std::vector< std::string* >*                          vsval;
   std::vector< std::pair< std::string, std::string > >* vspval;
   class Formula*                                        fval;
   std::vector< class Formula* >*                        vfval;
   std::pair< GiNaC::ex, std::vector< std::unordered_map< std::string, std::pair< std::string, GiNaC::ex > >::const_iterator > >* pval;
}

%token END	0	"end of file"
%token ASSERT CHECK_SAT PUSH POP
%token PLUS MINUS TIMES DIV
%token EQ LEQ GEQ LESS GREATER NEQ
%token AND OR NOT IFF XOR IMPLIES ITE LET AS TRUE FALSE
%token DECLARE_CONST DECLARE_FUN DECLARE_SORT
%token DEFINE_CONST DEFINE_FUN DEFINE_SORT
%token SET_INFO SET_OPTION SET_LOGIC
%token GET_INFO GET_OPTION
%token GET_VALUE GET_ASSIGNMENT GET_PROOF GET_UNSAT_CORE GET_ASSERTIONS
%token EXIT
%token <sval> SYM THEORY_VAR BOOLEAN_VAR NUM DEC KEY BIT HEX BIN

%token OB CB DB

%type <sval>   value
%type <pval>   poly polylistPlus polylistMinus polylistTimes polyOp
%type <fval>   form equation bind
%type <vfval>  formlist bindlist
%type <vsval>  symlist
%type <vspval> varlist
%type <eval>   relation
%type <eval>   unaryOp binaryOp naryOp

%{

#include "Driver.h"
#include "Scanner.h"

#undef yylex
#define yylex dv.pLexer()->lex

%}

%% /* SMT_RAT Grammar Rules */

start:
	commandlist

commandlist:
		commandlist command
	|	command

command:
		OB ASSERT form CB                       { dv.add( $3 ); }
	|	OB CHECK_SAT CB                         { dv.check(); }
	|	OB PUSH NUM CB                          { dv.push( *$3 ); delete $3; }
	|	OB POP NUM CB                           { dv.pop( *$3 ); delete $3; }
	| 	OB SET_LOGIC SYM CB                     { dv.setLogic( *$3 ); delete $3; }
	|	OB SET_INFO KEY value CB                { dv.setInfo( *$3, *$4 ); delete $4; delete $3; }
	|	OB GET_INFO KEY CB                      { dv.getInfo( *$3 ); delete $3; }
	|	OB SET_OPTION KEY value CB              { dv.setOption( *$3, *$4 ); delete $4; delete $3; }
	|	OB GET_OPTION KEY CB                    { dv.getOption( *$3 ); delete $3; }
	|	OB GET_VALUE OB varlist CB CB           { dv.getValue( $4 ); }
	|	OB GET_ASSIGNMENT CB                    { dv.getAssignment(); }
	|	OB GET_PROOF CB                         { dv.getProof(); }
	|	OB GET_UNSAT_CORE CB                    { dv.getUnsatCore(); }
	|	OB GET_ASSERTIONS CB                    { dv.getAssertions(); }
	|	OB DECLARE_CONST SYM SYM CB             { dv.addVariable( yyloc, *$3, *$4 ); delete $3; delete $4; }
	| 	OB DECLARE_FUN SYM OB CB SYM CB         { dv.addVariable( yyloc, *$3, *$6 ); delete $3; delete $6; }
	| 	OB DECLARE_FUN SYM OB symlist CB SYM CB { error( yyloc, "Declaration of function with arguments is not allowed in supported logics!" );
                                                  delete $3; delete $7; dv.free( $5 ); }
    |   OB DECLARE_SORT SYM NUM CB              { error( yyloc, "Declaration of types are not allowed in supported logics!" );
                                                  delete $3; delete $4; }
	| 	OB DEFINE_FUN SYM OB CB SYM CB          { error( yyloc, "Definition of functions are not allowed in supported logics!" );
                                                  delete $3; delete $6; }
	| 	OB DEFINE_FUN SYM OB symlist CB SYM CB  { error( yyloc, "Definition of functions are not allowed in supported logics!" );
                                                  delete $3; delete $7; dv.free( $5 ); }
	| 	OB DEFINE_SORT SYM OB CB SYM CB         { error( yyloc, "Definition of types are not allowed in supported logics!" );
                                                  delete $3; delete $6; }
	| 	OB DEFINE_SORT SYM OB symlist CB SYM CB { error( yyloc, "Definition of types are not allowed in supported logics!" );
                                                  delete $3; delete $7; dv.free( $5 ); }
	|	OB EXIT CB

symlist:
        SYM         { vector< string* >* syms = new vector< string* >(); syms->push_back( $1 ); $$ = syms; }
    |   symlist SYM { $1->push_back( $2 ); $$ = $1; }
    
varlist:
        THEORY_VAR          { vector< pair< string, string > >* vars = new vector< pair< string, string > >();
                              vars->push_back( pair< string, string >( *$1, "Theory" ) ); delete $1; $$ = vars; }
    |   BOOLEAN_VAR         { vector< pair< string, string > >* vars = new vector< pair< string, string > >();
                              vars->push_back( pair< string, string >( *$1, "Boolean" ) ); delete $1; $$ = vars; }
    |   varlist THEORY_VAR  { $1->push_back( pair< string, string >( *$2, "Theory" )  ); delete $2; $$ = $1; }
    |   varlist BOOLEAN_VAR { $1->push_back( pair< string, string >( *$2, "Boolean" )  ); delete $2; $$ = $1; }

value:
        SYM   { $$ = $1; }
    |   NUM   { $$ = $1; }
    |   DEC   { $$ = $1; }
    |   TRUE  { $$ = new string( "true" ); }
    |   FALSE { $$ = new string( "false" ); }

form:
        BOOLEAN_VAR                   { $$ = new Formula( dv.getBooleanVariable( yyloc, *$1 ) ); delete $1; }
    |   TRUE                          { $$ = new Formula( smtrat::TTRUE ); }
    |   FALSE                         { $$ = new Formula( smtrat::FFALSE ); }
    |   equation                      { $$ = $1; }
    |   OB relation poly poly CB      { $$ = dv.mkConstraint( *$3, *$4, $2 ); delete $3; delete $4; }
    |   OB AS SYM SYM CB              { error( yyloc, "\"as\" is not allowed in supported logics!" ); }
	|	OB unaryOp form CB            { $$ = dv.mkFormula( $2, $3 ); }
	|	OB binaryOp form form CB      { $$ = dv.mkFormula( $2, $3, $4 ); }
	|	OB naryOp formlist CB         { $$ = dv.mkFormula( $2, *$3 ); delete $3; }
    |   OB let OB bindlist CB form CB { $$ = dv.appendBindings( *$4, $6 ); delete $4; dv.popVariableStack(); }
    |   OB ITE form form form CB      { $$ = dv.mkIteInFormula( $3, $4, $5 ); }

formlist :
		form          { $$ = new vector< Formula* >( 1, $1 ); }
    |	formlist form { $1->push_back( $2 ); $$ = $1; }

equation:
       OB EQ form form CB { $$ = dv.mkFormula( smtrat::IFF, $3, $4 ); }
    |  OB EQ poly poly CB { $$ = dv.mkConstraint( *$3, *$4, CR_EQ ); delete $3; delete $4; }


relation :
		LEQ     { $$ = CR_LEQ; }
    |	GEQ     { $$ = CR_GEQ; }
    |	LESS    { $$ = CR_LESS; }
    |	GREATER { $$ = CR_GREATER; }
    |	NEQ     { $$ = CR_NEQ; }

unaryOp :
		NOT { $$ = smtrat::NOT; }

binaryOp :
		IMPLIES { $$ = smtrat::IMPLIES; }
    |	IFF     { $$ = smtrat::IFF; }
    |	XOR     { $$ = smtrat::XOR; }

naryOp :
		AND { $$ = smtrat::AND; }
    |	OR  { $$ = smtrat::OR; }

let :
        LET { dv.pushVariableStack(); }

bindlist :
		bind          { $$ = new vector< smtrat::Formula* >(); if( $1 != NULL ) { $$->push_back( $1 ); } }
	|	bind bindlist { $$ = $2; if( $1 != NULL ) { $$->push_back( $1 ); } }

bind :
        OB SYM poly CB { dv.addTheoryBinding( yyloc, *$2, $3 ); $$ = NULL; delete $3; delete $2; }
	|	OB SYM form CB { $$ = dv.booleanBinding( yyloc, *$2, $3 ); delete $2; }

poly :
        THEORY_VAR               { $$ = dv.mkPolynomial( yyloc, *$1 ); delete $1; }
    |   DEC                      { numeric* num = dv.getNumeric( *$1 ); delete $1;
                                   $$ = new PolyVarsPair( ex( *num ), TheoryVarVec() ); delete num; }
    | 	NUM                      { $$ = new PolyVarsPair( ex( numeric( $1->c_str() ) ), TheoryVarVec() ); delete $1; }
    |  	polyOp                   { $$ = $1; }
    |   OB ITE form poly poly CB { $$ = dv.mkPolynomial( yyloc, *dv.mkIteInExpr( yyloc, $3, *$4, *$5 ) ); }

polyOp :
		OB DIV poly poly CB       { $3->second.insert( $3->second.end(), $4->second.begin(), $4->second.end() ); $3->first /= $4->first; delete $4; $$ = $3; }
	|	OB MINUS poly CB          { $3->first *= -1; $$ = $3; }
	|	OB PLUS polylistPlus CB   { $$ = $3; }
	|	OB MINUS polylistMinus CB { $$ = $3; }
	|	OB TIMES polylistTimes CB { $$ = $3; }

polylistPlus :
		poly polylistPlus { $1->second.insert( $1->second.end(), $2->second.begin(), $2->second.end() );
                            $1->first += $2->first; $$ = $1; delete $2; }
	|	poly poly         { $1->second.insert( $1->second.end(), $2->second.begin(), $2->second.end() );
                            $1->first += $2->first; $$ = $1; delete $2; }

polylistMinus :
		poly polylistMinus { $1->second.insert( $1->second.end(), $2->second.begin(), $2->second.end() );
                             $1->first -= $2->first; $$ = $1; delete $2; }
	|	poly poly          { $1->second.insert( $1->second.end(), $2->second.begin(), $2->second.end() );
                             $1->first -= $2->first; $$ = $1; delete $2; }

polylistTimes :
		poly polylistTimes  { $1->second.insert( $1->second.end(), $2->second.begin(), $2->second.end() );
                              $1->first *= $2->first; $$ = $1; delete $2; }
	|	poly poly           { $1->second.insert( $1->second.end(), $2->second.begin(), $2->second.end() );
                              $1->first *= $2->first; $$ = $1; delete $2; }

%% /* Additional Code */

void smtrat::Parser::error( const Parser::location_type& _loc, const std::string& _message )
{
    dv.error( _loc, _message );
}
