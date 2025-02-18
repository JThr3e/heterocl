/*!
 *  Copyright (c) 2016 by Contributors
 * \file ir_visitor.h
 * \brief Visitor to quickly visit IR trees
 */
#ifndef TVM_IR_VISITOR_H_
#define TVM_IR_VISITOR_H_

#include <tvm/ir_functor.h>
#include "./ir.h"

namespace TVM {
namespace ir {

/*!
 * \brief a base class for visitor to iterative traverse the IR
 *
 *  This IRVisitor is implemented via IRFunctor
 *  This enables extensions of possible new Node.
 *
 * \sa ExprFunctor, StmtFunctor, PostOrderVisit
 *
 * \note If you need to return values during Visit:
 *  - If it is mutation of the IR, use IRMutator
 *  - If you want to return other things, consider use ExprFunctor/StmtFunctor
 *  - Watch out for possible bug pattern if you use IRVisitor to simulate returns.
 *
 * \code
 *
 * // This is an example code to show cases for traps in IRVisitor
 * // The use case is to count number of Variables in the ir tree.
 * class MyCounter : public IRVisitor {
 *  public:
 *   int Count(const NodeRef& n) {
 *     ret_ = 0;
 *     this->Visit(n);
 *     return ret_;
 *   }
 *   void Visit_(const Variable* op) final {
 *     ret_ = 1;
 *   }
 *   void Visit_(const Add* op) final {
 *     ret_ = count(op->a) + count(op->b);
 *   }

 *  private:
 *   int ret_;
 * };
 * MyCounter counter;
 * Var x("x");
 * // this returns 2
 * CHECK_EQ(counter.Count(x + x), 2);
 * // Think what is the result of the following count
 * counter.count(Max::make(x, x));
 * // The result is actually 1
 * // This is because Visit is not overriden for Max
 * // so it simply calls Visit for the left and right children
 * // and because Count is not called, ret_ is not cleared.
 * // There can also be cases where ret_ is forgetten to be set.
 *
 * // These traps may not happen if we program carefully
 * // But it is recommended to use ExprFunctor, which allows direct
 * // return the value, this helps us to avoid such problems.
 *
 * \endcode
 */
class TVM_DLL IRVisitor {
 public:
  /*!
   * \brief recursively visit an IR node
   */
  virtual void Visit(const NodeRef& node) {
    static const FVisit& f = vtable();
    if (node.defined()) f(node, this);
  }
  /*! \brief destructor */
  virtual ~IRVisitor() {}
  /*! \brief functor type of visitor */
  using FVisit = IRFunctor<void(const NodeRef&, IRVisitor*)>;
  /*! \return internal vtable*/
  static FVisit& vtable();
  // overloadable visit function.
  virtual void Visit_(const Variable* op);
  virtual void Visit_(const LetStmt* op);
  virtual void Visit_(const AttrStmt* op);
  virtual void Visit_(const IfThenElse* op);
  virtual void Visit_(const For* op);
  virtual void Visit_(const Allocate* op);
  virtual void Visit_(const Load* op);
  virtual void Visit_(const Store* op);
  virtual void Visit_(const Let* op);
  virtual void Visit_(const Free* op);
  virtual void Visit_(const Call* op);
  virtual void Visit_(const Add* op);
  virtual void Visit_(const Sub* op);
  virtual void Visit_(const Mul* op);
  virtual void Visit_(const Div* op);
  virtual void Visit_(const Mod* op);
  virtual void Visit_(const Min* op);
  virtual void Visit_(const Max* op);
  virtual void Visit_(const EQ* op);
  virtual void Visit_(const NE* op);
  virtual void Visit_(const LT* op);
  virtual void Visit_(const LE* op);
  virtual void Visit_(const GT* op);
  virtual void Visit_(const GE* op);
  virtual void Visit_(const And* op);
  virtual void Visit_(const Or* op);
  virtual void Visit_(const Reduce* op);
  virtual void Visit_(const Cast* op);
  virtual void Visit_(const Not* op);
  virtual void Visit_(const Select* op);
  virtual void Visit_(const Ramp* op);
  virtual void Visit_(const Broadcast* op);
  virtual void Visit_(const AssertStmt* op);
  virtual void Visit_(const ProducerConsumer* op);
  virtual void Visit_(const Provide* op);
  virtual void Visit_(const Realize* op);
  virtual void Visit_(const Prefetch* op);
  virtual void Visit_(const Block* op);
  virtual void Visit_(const Evaluate* op);
  virtual void Visit_(const IntImm* op);
  virtual void Visit_(const UIntImm* op);
  virtual void Visit_(const FloatImm* op);
  virtual void Visit_(const StringImm* op);
  virtual void Visit_(const GetBit* op);
  virtual void Visit_(const GetSlice* op);
  virtual void Visit_(const SetBit* op);
  virtual void Visit_(const SetSlice* op);
  virtual void Visit_(const Quantize* op);
  virtual void Visit_(const KernelDef* op);
  virtual void Visit_(const KernelExpr* op);
  virtual void Visit_(const KernelStmt* op);
  virtual void Visit_(const StreamExpr* op);
  virtual void Visit_(const StreamStmt* op);
  virtual void Visit_(const Return* op);
  virtual void Visit_(const Break* op);
  virtual void Visit_(const While* op);
  virtual void Visit_(const Reuse* op);
  virtual void Visit_(const Partition* op);
  virtual void Visit_(const Stencil* op);
};

/*!
 * \brief recursively visit the ir in post DFS order node, apply fvisit
 * Each node is guaranteed to be visited only once.
 * \param node The ir to be visited.
 * \param fvisit The visitor function to be applied.
 */
void PostOrderVisit(const NodeRef& node, std::function<void(const NodeRef&)> fvisit);

}  // namespace ir
}  // namespace TVM

#endif  // TVM_IR_VISITOR_H_
