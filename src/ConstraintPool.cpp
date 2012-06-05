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

#include "ConstraintPool.h"

using namespace std;
using namespace GiNaC;

namespace smtrat
{
    /**
     * Constructs a new constraint using its string representation.
     *
     * @param _stringrep    String representation of the constraint.
     * @param _infix        true, if the given representation is in infix notation and false, if it is in prefix notation
     * @param _polarity     The polarity of the constraint.
     *
     * @return A shared pointer to the constraint.
     */
    const Constraint* ConstraintPool::newConstraint( const string& _stringrep, const bool _infix, const bool _polarity )
    {
        /*
         * Read the given string representing the constraint.
         */
        string expression;
        if( _infix )
        {
            expression = _stringrep;
        }
        else
        {
            expression = prefixToInfix( _stringrep );
        }
        string::size_type   opPos;
        Constraint_Relation relation;
        unsigned            opSize = 0;
        opPos = expression.find( "=", 0 );
        if( opPos == string::npos )
        {
            opPos = expression.find( "<", 0 );
            if( opPos == string::npos )
            {
                opPos = expression.find( ">", 0 );

                assert( opPos != string::npos );

                if( _polarity )
                {
                    relation = CR_GREATER;
                }
                else
                {
                    relation = CR_LEQ;
                }
                opSize = 1;
            }
            else
            {
                if( _polarity )
                {
                    relation = CR_LESS;
                }
                else
                {
                    relation = CR_GEQ;
                }
                opSize = 1;
            }
        }
        else
        {
            string::size_type tempOpPos = opPos;
            opPos = expression.find( "<", 0 );
            if( opPos == string::npos )
            {
                opPos = expression.find( ">", 0 );
                if( opPos == string::npos )
                {
                    opPos = expression.find( "!", 0 );
                    if( opPos == string::npos )
                    {
                        opPos = tempOpPos;
                        if( _polarity )
                        {
                            relation = CR_EQ;
                        }
                        else
                        {
                            relation = CR_NEQ;
                        }
                        opSize = 1;
                    }
                    else
                    {
                        if( _polarity )
                        {
                            relation = CR_NEQ;
                        }
                        else
                        {
                            relation = CR_EQ;
                        }
                        opSize = 2;
                    }
                }
                else
                {
                    if( _polarity )
                    {
                        relation = CR_GEQ;
                    }
                    else
                    {
                        relation = CR_LESS;
                    }
                    opSize = 2;
                }
            }
            else
            {
                if( _polarity )
                {
                    relation = CR_LEQ;
                }
                else
                {
                    relation = CR_GREATER;
                }
                opSize = 2;
            }
        }

        /*
         * Parse the lefthand and righthand side and store their difference as
         * lefthand side of the constraint.
         */
        parser reader( mAllVariables );
        ex     lhs, rhs;
        string lhsString = expression.substr( 0, opPos );
        string rhsString = expression.substr( opPos + opSize );
        try
        {
            lhs = reader( lhsString );
            rhs = reader( rhsString );
        }
        catch( parse_error& err )
        {
            cerr << err.what() << endl;
        }

        /*
         * Collect the new variables in the constraint:
         */
        mAllVariables.insert( reader.get_syms().begin(), reader.get_syms().end() );
        return *mAllConstraints.insert( new Constraint( lhs, rhs, relation, mAllVariables ) ).first;
    }

    /**
     * Transforms the constraint in prefix notation to a constraint in infix notation.
     *
     * @param   _prefixRep  The prefix notation of the contraint to transform.
     *
     * @return The infix notation of the constraint.
     */
    string ConstraintPool::prefixToInfix( const string& _prefixRep )
    {
        assert( !_prefixRep.empty() );

        if( _prefixRep.at( 0 ) == '(' )
        {
            string op  = string( "" );
            string lhs = string( "" );
            string rhs = string( "" );
            unsigned pos               = 1;
            unsigned numOpeningBracket = 0;
            unsigned numClosingBracket = 0;
            while( pos < _prefixRep.length() && _prefixRep.at( pos ) != ' ' )
            {
                assert( _prefixRep.at( pos ) != '(' );
                assert( _prefixRep.at( pos ) != ')' );
                op += _prefixRep.at( pos );
                pos++;
            }

            assert( pos != _prefixRep.length() );
            pos++;

            while( pos < _prefixRep.length() )
            {
                if( _prefixRep.at( pos ) == '(' )
                {
                    numOpeningBracket++;
                    lhs += _prefixRep.at( pos );
                }
                else if( _prefixRep.at( pos ) == ')' && numOpeningBracket > numClosingBracket )
                {
                    numClosingBracket++;
                    lhs += _prefixRep.at( pos );
                }
                else if( (_prefixRep.at( pos ) == ' ' && numOpeningBracket == numClosingBracket)
                         || (_prefixRep.at( pos ) == ')' && numOpeningBracket == numClosingBracket) )
                {
                    break;
                }
                else
                {
                    lhs += _prefixRep.at( pos );
                }
                pos++;
            }

            assert( pos != _prefixRep.length() );

            if( _prefixRep.at( pos ) == ')' )
            {
                assert( op.compare( "-" ) == 0 );

                string result = "(-1)*(" + prefixToInfix( lhs ) + ")";
                return result;
            }
            string result = "(" + prefixToInfix( lhs ) + ")";
            while( _prefixRep.at( pos ) != ')' )
            {
                rhs = "";
                pos++;
                while( pos < _prefixRep.length() )
                {
                    if( _prefixRep.at( pos ) == '(' )
                    {
                        numOpeningBracket++;
                        rhs += _prefixRep.at( pos );
                    }
                    else if( _prefixRep.at( pos ) == ')' && numOpeningBracket > numClosingBracket )
                    {
                        numClosingBracket++;
                        rhs += _prefixRep.at( pos );
                    }
                    else if( (_prefixRep.at( pos ) == ' ' || _prefixRep.at( pos ) == ')') && numOpeningBracket == numClosingBracket )
                    {
                        break;
                    }
                    else
                    {
                        rhs += _prefixRep.at( pos );
                    }
                    pos++;
                }

                assert( pos != _prefixRep.length() );

                result += op + "(" + prefixToInfix( rhs ) + ")";
            }
            return result;
        }
        else
        {
            assert( _prefixRep.find( " ", 0 ) == string::npos );
            assert( _prefixRep.find( "(", 0 ) == string::npos );
            assert( _prefixRep.find( ")", 0 ) == string::npos );
            return _prefixRep;
        }
    }

}    // namespace smtrat
