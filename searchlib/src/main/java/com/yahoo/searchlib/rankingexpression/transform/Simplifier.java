// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.searchlib.rankingexpression.transform;

import com.yahoo.document.update.ArithmeticValueUpdate;
import com.yahoo.searchlib.rankingexpression.evaluation.DoubleValue;
import com.yahoo.searchlib.rankingexpression.evaluation.Value;
import com.yahoo.searchlib.rankingexpression.rule.ArithmeticNode;
import com.yahoo.searchlib.rankingexpression.rule.ArithmeticOperator;
import com.yahoo.searchlib.rankingexpression.rule.ComparisonNode;
import com.yahoo.searchlib.rankingexpression.rule.CompositeNode;
import com.yahoo.searchlib.rankingexpression.rule.ConstantNode;
import com.yahoo.searchlib.rankingexpression.rule.EmbracedNode;
import com.yahoo.searchlib.rankingexpression.rule.ExpressionNode;
import com.yahoo.searchlib.rankingexpression.rule.IfNode;
import com.yahoo.searchlib.rankingexpression.rule.ReferenceNode;
import com.yahoo.searchlib.rankingexpression.rule.TensorFunctionNode;
import com.yahoo.tensor.functions.TensorFunction;

import java.util.ArrayList;
import java.util.List;

/**
 * Performs simple algebraic simplification of expressions
 *
 * @author bratseth
 */
public class Simplifier extends ExpressionTransformer<TransformContext> {

    @Override
    public ExpressionNode transform(ExpressionNode node, TransformContext context) {
        if (node instanceof CompositeNode)
            node = transformChildren((CompositeNode) node, context); // depth first
        if (node instanceof IfNode)
            node = transformIf((IfNode) node);
        if (node instanceof EmbracedNode && hasSingleUndividableChild((EmbracedNode)node))
            node = ((EmbracedNode)node).children().get(0);
        if (node instanceof ArithmeticNode)
            node = transformArithmetic((ArithmeticNode) node);
        return node;
    }

    private boolean hasSingleUndividableChild(EmbracedNode node) {
        if (node.children().size() > 1) return false;
        if (node.children().get(0) instanceof ArithmeticNode) return false;
        if (node.children().get(0) instanceof ComparisonNode) return false;
        return true;
    }

    private ExpressionNode transformArithmetic(ArithmeticNode node) {
        // Fold the subset of expressions that are constant (such that in "1 + 2 + var")
        if (node.children().size() > 1) {
            List<ExpressionNode> children = new ArrayList<>(node.children());
            List<ArithmeticOperator> operators = new ArrayList<>(node.operators());
            for (ArithmeticOperator operator : ArithmeticOperator.operatorsByPrecedence)
                transform(operator, children, operators);
            node = new ArithmeticNode(children, operators);
        }

        if (isConstant(node) && ! node.evaluate(null).isNaN())
            return new ConstantNode(node.evaluate(null));
        else if (allMultiplicationOrDivision(node) && hasZero(node) && ! hasDivisionByZero(node))
            return new ConstantNode(new DoubleValue(0));
        else
            return node;
    }

    private void transform(ArithmeticOperator operatorToTransform,
                           List<ExpressionNode> children, List<ArithmeticOperator> operators) {
        int i = 0;
        while (i < children.size()-1) {
            boolean transformed = false;
            if ( operators.get(i).equals(operatorToTransform)) {
                ExpressionNode child1 = children.get(i);
                ExpressionNode child2 = children.get(i + 1);
                if (isConstant(child1) && isConstant(child2) && hasPrecedence(operators, i)) {
                    Value evaluated = new ArithmeticNode(child1, operators.get(i), child2).evaluate(null);
                    if ( ! evaluated.isNaN()) { // Don't replace by NaN
                        operators.remove(i);
                        children.set(i, new ConstantNode(evaluated.freeze()));
                        children.remove(i + 1);
                        transformed = true;
                    }
                }
            }
            if ( ! transformed) // try the next index
                i++;
        }
    }

    /**
     * Returns true if the operator at i binds at least as strongly as the neighbouring operators on each side (if any).
     * This check works because we simplify by decreasing precedence, so neighbours will either be single constant values
     * or a more complex expression that can't be simplified and hence also prevents the simplification in question here.
     */
    private boolean hasPrecedence(List<ArithmeticOperator> operators, int i) {
        if (i > 0 && operators.get(i-1).hasPrecedenceOver(operators.get(i))) return false;
        if (i < operators.size()-1 && operators.get(i+1).hasPrecedenceOver(operators.get(i))) return false;
        return true;
    }

    private ExpressionNode transformIf(IfNode node) {
        if ( ! isConstant(node.getCondition())) return node;

        if ((node.getCondition().evaluate(null)).asBoolean())
            return node.getTrueExpression();
        else
            return node.getFalseExpression();
    }

    private boolean allMultiplicationOrDivision(ArithmeticNode node) {
        for (ArithmeticOperator o : node.operators())
            if (o != ArithmeticOperator.MULTIPLY && o != ArithmeticOperator.DIVIDE)
                return false;
        return true;
    }

    private boolean hasZero(ArithmeticNode node) {
        for (ExpressionNode child : node.children()) {
            if (isZero(child))
                return true;
        }
        return false;
    }

    private boolean hasDivisionByZero(ArithmeticNode node) {
        for (int i = 1; i < node.children().size(); i++) {
            if ( node.operators().get(i - 1) == ArithmeticOperator.DIVIDE && isZero(node.children().get(i)))
                return true;
        }
        return false;
    }

    private boolean isZero(ExpressionNode node) {
        if ( ! (node instanceof ConstantNode)) return false;
        ConstantNode constant = (ConstantNode)node;
        if ( ! constant.getValue().hasDouble()) return false;
        return constant.getValue().asDouble() == 0.0;
    }

    private boolean isConstant(ExpressionNode node) {
        if (node instanceof ConstantNode) return true;
        if (node instanceof ReferenceNode) return false;
        if (node instanceof TensorFunctionNode) return false; // TODO: We could support asking it if it is constant
        if ( ! (node instanceof CompositeNode)) return false;
        for (ExpressionNode child : ((CompositeNode)node).children()) {
            if ( ! isConstant(child)) return false;
        }
        return true;
    }

}
