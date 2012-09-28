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
 * @file LRAModule.h
 * @author name surname <emailadress>
 *
 * @version 2012-04-05
 * Created on April 5th, 2012, 3:22 PM
 */

#ifndef LRAMODULE_H
#define LRAMODULE_H

#include "../Module.h"
#include "LRAModule/Value.h"
#include "LRAModule/Variable.h"
#include "LRAModule/Bound.h"
#include "LRAModule/Tableau.h"
#include <stdio.h>

#define LRA_SIMPLE_CONFLICT_SEARCH

namespace smtrat
{
    class LRAModule:
        public Module
    {
        public:
            struct exPointerComp
            {
                bool operator ()( const GiNaC::ex* const pExA, const GiNaC::ex* const pExB ) const
                {
                    return GiNaC::ex_is_less()( *pExA, *pExB );
                }
            };
            struct constraintPointerComp
            {
                bool operator ()( const Constraint* const pConstraintA, const Constraint* const pConstraintB ) const
                {
                    return (*pConstraintA) < (*pConstraintB);
                }
            };
            typedef std::map<const GiNaC::ex*, lra::Variable*, exPointerComp>                    ExVariableMap;
            typedef std::pair<const Constraint* const , const lra::Bound* >                      ConstraintBoundPair;
            typedef std::map<const Constraint* const , const lra::Bound*, constraintPointerComp> ConstraintBoundMap;

        private:

            /**
             * Members:
             */
            bool                        mInitialized;
            lra::Tableau                mTableau;
            std::set<const Constraint*, constraintPointerComp > mLinearConstraints;
            std::set<const Constraint*, constraintPointerComp > mNonlinearConstraints;
            ExVariableMap               mExistingVars;
            ConstraintBoundMap          mConstraintToBound;

        public:

            /**
             * Constructors:
             */
            LRAModule( Manager* const _tsManager, const Formula* const _formula );

            /**
             * Destructor:
             */
            virtual ~LRAModule();

            /**
             * Methods:
             */

            // Interfaces.
            bool inform( const Constraint* const );
            bool assertSubformula( Formula::const_iterator );
            void removeSubformula( Formula::const_iterator );
            Answer isConsistent();

        private:
            /**
             * Methods:
             */
            #ifdef LRA_REFINEMENT
            void learnRefinements();
            #endif
            bool checkAssignmentForNonlinearConstraint() const;
            bool activateBound( const lra::Bound*, std::set<const Formula*>& );
            void setBound( lra::Variable&, const Constraint_Relation&, bool, const GiNaC::numeric&, const Constraint* );
            #ifdef LRA_SIMPLE_CONFLICT_SEARCH
            void findSimpleConflicts( const lra::Bound& );
            #endif
            void initialize();
    };

}    // namespace smtrat

#endif   /* LRAMODULE_H */
