/**
 * @file PolyTree.h
 * @author Andreas Krueger <andreas.krueger@rwth-aachen.de>
 */

#pragma once

#include <boost/optional.hpp>
#include "../../Common.h"

namespace smtrat
{
    // forward declaration
    class PolyTreeContent;

    // forward declaration
    class PolyTreePool;

    class PolyTree
    {
    private:
        const PolyTreeContent* mpContent;

    public:
        enum class Type : unsigned { VARIABLE, CONSTANT, SUM, PRODUCT };

        PolyTree(const Poly& _poly);

        const PolyTree& left() const;
        const PolyTree& right() const;
        carl::Variable::Arg variable() const;
        const Integer& constant() const;
        Type type() const;
        const Poly& poly() const;
    };

    class PolyTreeContent
    {
        friend class PolyTree;

    private:
        Poly mPoly;
        PolyTree::Type mType;
        union
        {
            carl::Variable mVariable;
            Integer mConstant;
        };
        boost::optional<PolyTree> mLeft;
        boost::optional<PolyTree> mRight;

    public:
        PolyTreeContent(const Poly& _poly, PolyTree::Type _type, const PolyTree& _left, const PolyTree& _right) :
        mPoly(_poly), mType(_type), mVariable(), mLeft(_left), mRight(_right)
        {
            assert(_type == PolyTree::Type::SUM || _type == PolyTree::Type::PRODUCT);
        }

        PolyTreeContent(carl::Variable::Arg _variable) :
        mPoly(Poly(_variable)), mType(PolyTree::Type::VARIABLE), mLeft(), mRight()
        { }

        PolyTreeContent(Integer _constant) :
        mPoly(Poly(_constant)), mType(PolyTree::Type::CONSTANT), mLeft(), mRight()
        { }

        ~PolyTreeContent()
        {
            if(mType == PolyTree::Type::CONSTANT) {
                mConstant.~Integer();
            } else {
                mVariable.~Variable();
            }
        }

        const Poly& poly() const
        {
            return mPoly;
        }

        bool operator==(const PolyTreeContent& _other) const
        {
            return mPoly == _other.mPoly;
        }

    };
} // namespace smtrat
