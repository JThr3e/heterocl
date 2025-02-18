/*!
 *  Copyright (c) 2016 by Contributors
 * \file schedule.h
 * \brief Define a schedule.
 */
#ifndef TVM_SCHEDULE_H_
#define TVM_SCHEDULE_H_

#include <string>
#include <vector>
#include "./base.h"
#include "./expr.h"
#include "./tensor.h"
#include "./tensor_intrin.h"
#include "./ir.h"

namespace TVM {

// Node container for Stage
class StageNode;
// Node container for Schedule
class ScheduleNode;
// Node container for IterVarRelation
class IterVarRelationNode;
// Attribute of itervar.
class IterVarAttrNode;

/*! \brief the attachment type */
enum AttachType : int {
  kGroupRoot = 1,
  kInline = 2,
  kInlinedAlready = 3,
  kScope = 4,
  kScanUpdate = 5
};

/*! \brief Stage, contains scheduling for a stage of computation. */
class Stage : public NodeRef {
 public:
  Stage() {}
  explicit Stage(std::shared_ptr<Node> n) : NodeRef(n) {}
  /*!
   * \brief create a new schedule for op.
   * \param op The operator in the schedule
   */
  explicit Stage(Operation op);
  /*!
   * \brief access the internal node container
   * \return the pointer to the internal node container
   */
  inline const StageNode* operator->() const;
  /*!
   * \brief access the internal node container
   * \return the pointer to the internal node container
   */
  inline StageNode* operator->();
  /*!
   * \brief set the memory scope of the stage
   * \param scope The memory scope.
   */
  EXPORT Stage& set_scope(std::string scope);  // NOLINT(*)
  /*!
   * \brief specify the schedule to be computed at the parent schedule's scope.
   * \param parent The parent schedule.
   * \param scope The iteration point to carry the schedule.
   * \return reference to self.
   */
  EXPORT Stage& compute_at(Stage parent, IterVar scope);   // NOLINT(*)
  /*!
   * \brief Compute the function inline.
   * \return reference to self.
   */
  EXPORT Stage& compute_inline();   // NOLINT(*)
  /*!
   * \brief Compute the function at group root.
   * \return reference to self.
   */
  EXPORT Stage& compute_root();  // NOLINT(*)
  /*!
   * \brief Bind the ivar to thread index.
   *
   * \param ivar The IterVar to be binded.
   * \param thread_ivar The thread axis to be binded.
   * \return reference to self.
   */
  EXPORT Stage& bind(IterVar ivar, IterVar thread_ivar);
  /*!
   * \brief Set predicate under which store to the array can be performed.
   *  Use this when there are duplicated threads doing the same store and we only
   *  need one of them to do the store.
   *
   * \note This is a dangerous scheduling primitive that can change behavior of program.
   *    Only do when we are certain that thare are duplicated store.
   * \param predicate The condition to be checked.
   * \return reference to self.
   */
  EXPORT Stage& set_store_predicate(Expr predicate);
  /*!
   * \brief Specify environment threads that launched around the group's scope.
   *  This can only be used in group stage.
   * \param threads The threads to be launched around the scope.
   * \note Each thread can only appear in one env_threads.
   *    This is a beta feature.
   * \return reference to self.
   */
  EXPORT Stage& env_threads(Array<IterVar> threads);
  /*!
   * \brief Split the parent by factor.
   * \param parent The parent iteration domain.
   * \param factor The split factor of the loop.
   * \param p_outer The result outer domain
   * \param p_inner The result inner domain.
   * \return reference to self.
   */
  EXPORT Stage& split(IterVar parent, Expr factor, IterVar* p_outer, IterVar* p_inner);  // NOLINT(*)
  /*!
   * \brief Split the iteration with given number of parts.
   * \param parent The parent iteration domain.
   * \param nparts The number of parts in the outer domain.
   * \param p_outer The result outer domain.
   * \param p_inner The result inner domain.
   * \return reference to self.
   */
  EXPORT Stage& split_by_nparts(IterVar parent, Expr nparts, IterVar* p_outer, IterVar* p_inner);   // NOLINT(*)
  /*!
   * \brief Split the iteration by annotation without changing the IR structure.
   * \param parent The iteration domain.
   * \param factor The split factor of the loop.
   * \return reference to self.
   */
  EXPORT Stage& split_annotate(IterVar parent, Expr factor);  // NOLINT(*)
  /*!
   * \brief Split the iteration by annotation without changing the IR structure.
   * \param parent The iteration domain.
   * \param nparts The number of parts in the outer domain.
   * \return reference to self.
   */
  EXPORT Stage& split_by_nparts_annotate(IterVar parent, Expr nparts);   // NOLINT(*)
  /*!
   * \brief Fuse the inner outer domain to the target
   * \param outer The outer domain to be fused.
   * \param inner The inner domain to be fused
   * \param p_target The result target domain.
   * \return reference to self.
   */
  EXPORT Stage& fuse(IterVar outer, IterVar inner, IterVar* p_target);  // NOLINT(*)
  /*!
   * \brief Reorder the iteration
   * \param order The order of iteration variable.
   * \return reference to self.
   */
  EXPORT Stage& reorder(const Array<IterVar>& order);   // NOLINT(*)
  /*!
   * \brief Perform tiling on two dimensions
   *  The final loop order from outmost to inner most are
   *  [x_outer, y_outer, x_inner, y_inner]
   *
   * \param x_parent The original x dimension
   * \param y_parent The original y dimension
   * \param x_factor The stride factor on x axis
   * \param y_factor The stride factor on y axis
   * \param p_x_outer Outer axis of x dimension
   * \param p_y_outer Outer axis of y dimension
   * \param p_x_inner Inner axis of x dimension
   * \param p_y_inner Inner axis of y dimension
   * \return reference to self.
   */
  EXPORT Stage& tile(IterVar x_parent, IterVar y_parent,   // NOLINT(*)
              Expr x_factor, Expr y_factor,
              IterVar* p_x_outer, IterVar* p_y_outer,
              IterVar* p_x_inner, IterVar* p_y_inner);
  /*!
   * \brief Vectorize iteration.
   * \param var The axis to be vectorized.
   * \return reference to self.
   */
  EXPORT Stage& vectorize(IterVar var);   // NOLINT(*)
  /*!
   * \brief Replace computation of the current stage by tensor intrinsic f.
   * \param var The axis marks beginning of tensorization.
   *  Every operations inside the axis(include axis itself is tensorized).
   * \param f The Tensor compute intrinsics.
   * \return reference to self.
   */
  EXPORT Stage& tensorize(IterVar var, TensorIntrin f);   // NOLINT(*)
  /*!
   * \brief Unroll iteration.
   * \param var The axis to be unrolled.
   * \return reference to self.
   */
  EXPORT Stage& unroll(IterVar var);   // NOLINT(*)
  /*!
   * \brief Unroll iteration.
   * \param var The axis to be unrolled.
   * \param factor Unroll factor.
   * \return reference to self.
   */
  EXPORT Stage& unroll(IterVar var, const Expr& factor);   // NOLINT(*)
  /*!
   * \brief Parallelize iteration.
   * \param var The axis to be parallelized.
   * \return reference to self.
   */
  EXPORT Stage& parallel(IterVar var);   // NOLINT(*)
  /*!
   * \brief Pipeline iteration.
   * \param var The axis to be pipelined.
   * \param initiation_interval The initiation interval.
   * \return reference to self.
   */
  EXPORT Stage& pipeline(IterVar var, const Expr& initiation_interval);   // NOLINT(*)
  /*!
   * \brief create stream data channel.
   * \param target The data streaming consumer.
   * \param stream_type The data streaming channel type.
   * \param depth The channel depth.
   * \return reference to self.
   */
  EXPORT Stage& stream(Stage dest, Stage source, 
                       ir::StreamType stream_type, int depth);   // NOLINT(*)

  EXPORT Stage& stencil(int burst_width, int unroll_factor, int num_iteration);   // NOLINT(*)
  /*!
   * \brief Annotate the iteration with pragma
   *
   * \param var The axis to be parallelized.
   * \param pragma_type The pragma type.
   *
   * \return reference to self.
   */
  EXPORT Stage& pragma(IterVar var, const std::string& pragma_type);   // NOLINT(*)
  /*!
   * \brief Fetch data in advance.
   * \param domain the tensor to be prefetched
   * \param var the iteration point at which to apply prefetching
   * \param offset the number of iterations be to fetched in advance
   * \return reference to self
   */
  EXPORT Stage& prefetch(const Tensor &domain, IterVar var, Expr offset); //NOLINT(*)
  /*!
   * \brief Set alignment requirement for specific dimension.
   *
   *  Such that stride[axis] == k * factor + offset for some k.
   *
   * \param axis The dimension to be specified for alignment.
   * \param factor The factor multiple of alignment
   * \param offset The required offset factor.
   * \return reference to self
   */
  EXPORT Stage& storage_align(IterVar axis, int factor, int offset); //NOLINT(*)
  /*!
   * \brief Compute current stage with double buffering.
   * \return reference to self.
   */
  EXPORT Stage& double_buffer();   // NOLINT(*)
  /*!
   * \brief Schedule for OpenGL fragment shader.
   * \return reference to self.
   */
  Stage& opengl(); // NOLINT(*)
  /*!
   * \brief whether the stage has been scheduled.
   * \return whether the stage has been scheduled.
   */
  bool is_scheduled() const;
  /*!
   * \brief Get attachment spec of current stage.
   *  If the stage compute at Group root, this function
   *  will traverse the group function to get the
   *  final spec from the group.
   * \return A stage representing the attach spec of the group.
   */
  Stage GetAttachSpec() const;
  // declare container type
  using ContainerType = StageNode;
};

/*!
 * \brief Global schedule container
 *  For operations and all the operations they depend on.
 *  The schedule per Operation is named as stage.
 */
class Schedule : public NodeRef {
 public:
  Schedule() {}
  explicit Schedule(std::shared_ptr<Node> n) : NodeRef(n) {}
  /*!
   * \brief Get a copy of current schedule.
   * \return The copied schedule.
   */
  Schedule copy() const;
  /*!
   * \brief Get the stage corresponds to the op
   * \param op The operation.
   */
  EXPORT Stage operator[](const Operation& op);
  /*!
   * \brief Short hand for getting the stage of tensor's operation.
   * \param tensor The tensor
   * \return The stage corresponding to the tensor's op
   */
  EXPORT Stage operator[](const Tensor& tensor) {
    return this->operator[](tensor->op);
  }
  /*!
   * \brief Create a new stage group for all intermediate
   *  operations between inputs and outputs.
   *
   * \param outputs The output boundary of the group.
   * \param inputs The input boundary of the group.
   * \param include_inputs Whether include inputs if they are reachable from outputs.
   * \return The new grouped stage.
   */
  EXPORT Stage create_group(const Array<Tensor>& outputs,
                     const Array<Tensor>& inputs,
                     bool include_inputs = false);
  /*!
   * \brief create a cache read of original tensor for readers.
   *  This will mutate the body of the readers.
   *  A new stage will be created for the tensor.
   * \param tensor The tensor cached.
   * \param scope The scope of the cache.
   * \param readers The readers to redirect to the tensor.
   * \return The created tensor.
   */
  EXPORT Tensor cache_read(const Tensor& tensor,
                    const std::string& scope,
                    const Array<Operation>& readers);
  /*!
   * \brief Create a cache write tensor for producing tensor.
   *  The the tensor will take over body of original tensor op.
   *
   *  This function can be used to do data layout transformation.
   *  If there is a split/fuse/reorder on the data parallel axis of tensor
   *  before cache_write is called. The intermediate cache stores
   *  the data in the layout as the iteration order of leave axis.
   *  The data will be transformed back to the original layout in the original tensor.
   *  User can further call compute_inline to inline the original layout and keep
   *  the data stored in the transformed layout.
   *
   * \param tensor The tensor to be produced.
   * \param scope The scope of the storage.
   * \return The created tensor.
   */
  EXPORT Tensor cache_write(const Tensor& tensor, const std::string& scope);
  /*!
   * \brief Factor a reduction axis in tensor's schedule to be an explicit axis.
   * This will create a new stage that generated the new tensor with axis
   * as the first dimension. The tensor's body will be rewritten as a reduction
   * over the factored tensor.
   *
   *  P. Suriana, A. Adams and S. Kamil. Parallel associative reductions in halide. CGO'17
   *
   * \param tensor The tensor to be factored.
   * \param axis The reduction axis in tensor's schedule to be factored.
   * \param factor_axis The position where the new axis is placed.
   * \return The created factored tensors.
   */
  EXPORT Array<Tensor> rfactor(const Tensor& tensor,
                        const IterVar& axis,
                        int factor_axis = 0);

  EXPORT Tensor reuse_at(const Tensor& target, 
      Stage parent, 
      IterVar axis,
      std::string name);

  EXPORT Tensor partition(const Tensor& target, int dim, int factor,
                          ir::PartitionType partition_type);

  EXPORT void reshape(const Tensor& target, Array<Expr> new_shape);
  /*!
   * \brief Normalize the schedule.
   *  This is needed before bound inference.
   *  Insert necessary RebaseNode to make sure all leaf_iter_vars
   *  are in form [0, extent)
   *
   * \return A normalized schedule, can be same as current one.
   */
  Schedule normalize();
  /*!
   * \brief access the internal node container
   * \return the pointer to the internal node container
   */
  inline const ScheduleNode* operator->() const;
  /*!
   * \brief access the internal node container
   * \return the pointer to the internal node container
   */
  inline ScheduleNode* operator->();
  // declare container type
  using ContainerType = ScheduleNode;
};

/*!
 * \brief The schedule relation between IterVars
 *  can be Split, Fuse.
 */
class IterVarRelation : public NodeRef {
 public:
  IterVarRelation() {}
  explicit IterVarRelation(std::shared_ptr<Node> n) : NodeRef(n) {}
  /*!
   * \brief access the internal node container
   * \return the pointer to the internal node container
   */
  inline const IterVarRelationNode* operator->() const;
};

/*!
 * \brief Additional scheduable attributes about IterVar.
 */
class IterVarAttr : public NodeRef {
 public:
  IterVarAttr() {}
  explicit IterVarAttr(std::shared_ptr<Node> n) : NodeRef(n) {}
  /*!
   * \brief access the internal node container
   * \return the pointer to the internal node container
   */
  inline const IterVarAttrNode* operator->() const;
};

/*!
 * \brief represents a stage.
 *
 *  relations form a Directed acylic hypergraph in bipartite manner.
 *  With each node is represented by a IterVar,
 *  and each hyper-edge is represented by a IterVarRelation.
 *  The relations connects the IterVars in the graph.
 *
 *  Besides typical stage that corresponds to operations.
 *  There is also group stage, which groups stages together.
 *  Each stage's group(given by group) represent an constraint,
 *  the stage can only be attached to stages within the group.
 *
 *  The group stage node can be attached to IterVars as in normal stage.
 */
class StageNode : public Node {
 public:
  /*!
   * \brief The operation of stage, can be different from original op.
   *  If it is null, then this stage is a group stage.
   */
  Operation op;
  /*!
   * \brief The original operator.
   *  The op field can change during schedule to alternate the dataflow,
   *  while origin_op remains fixed.
   */
  Operation origin_op;
  /*! \brief All the nodes in the iter var */
  Array<IterVar> all_iter_vars;
  /*! \brief The current active leaf iter vars in the stage. */
  Array<IterVar> leaf_iter_vars;
  /*! \brief The iter var exprs before reorder in the stage. */
  std::vector<Expr> iter_var_exprs_before_reorder;
  /*! \brief The iter var exprs after reorder in the stage. */
  std::vector<Expr> iter_var_exprs_after_reorder;
  /*!
   * \brief Specify threads to be launched at the stage.
   *  This is only valid for composite ops such as Scan.
   * \note Experimental primitive: used for thread persistence.
   */
  Array<IterVar> env_threads;
  /*!
   * \brief The predicate under which store can happen
   *  Use this when there can be duplicated threads doing the same store.
   * \note Experimental primitive: used by cross thread-reduction.
   */
  Expr store_predicate;
  /*! \brief The relation bwteen of IterVars */
  Array<IterVarRelation> relations;
  /*! \brief additional attributes about iter var. */
  Map<IterVar, IterVarAttr> iter_var_attrs;
  /*! \brief The attachment type of the schedule */
  AttachType attach_type{kGroupRoot};
  /*! \brief The attach point of this schedule. */
  IterVar attach_ivar;
  size_t attach_level;
  IterVar origin_attach_ivar;
  /*! \brief Stages that are attached to the current stage. */
  std::vector<Stage> attached_stages;
  /*! \brief The stage this node attaches to */
  Stage attach_stage;
  /*! \brief The thread storage scope level of the stage */
  std::string scope;
  /*! \brief Whether this is an output stage */
  bool is_output{false};
  /*! \brief Whether this is an OpenGL stage */
  bool is_opengl{false};
  /*! \brief Whether apply double buffer optimization to this stage */
  bool double_buffer{false};
  /*!
   * \brief The parent group of the current stage.
   *  The stage cannot be assigned to stages outside the group.
   */
  Stage group;
  /*! \brief Number of direct child stages, only used for group stage.*/
  int num_child_stages{0};

  void VisitAttrs(AttrVisitor* v) final {
    v->Visit("op", &op);
    v->Visit("origin_op", &origin_op);
    v->Visit("all_iter_vars", &all_iter_vars);
    v->Visit("leaf_iter_vars", &leaf_iter_vars);
    v->Visit("env_threads", &env_threads);
    v->Visit("relations", &relations);
    v->Visit("iter_var_attrs", &iter_var_attrs);
    v->Visit("attach_type", &attach_type);
    v->Visit("attach_ivar", &attach_ivar);
    v->Visit("attach_stage", &attach_stage);
    v->Visit("scope", &scope);
    v->Visit("is_output", &is_output);
    v->Visit("is_opengl", &is_opengl);
    v->Visit("double_buffer", &double_buffer);
    v->Visit("group", &group);
    v->Visit("num_child_stages", &num_child_stages);
  }

  static constexpr const char* _type_key = "Stage";
  TVM_DECLARE_NODE_TYPE_INFO(StageNode, Node);
};

/*! \brief node container for schedule */
class ScheduleNode : public Node {
 public:
  /*! \brief The output operations in original data flow graph */
  Array<Operation> outputs;
  /*!
   * \brief list of all stages for ops.
   * The stages are sorted in dependency order.
   */
  Array<Stage> stages;
  /*!
   * \brief List of all stage groups.
   */
  Array<Stage> groups;
  /*! \brief map of original operation to the stages */
  Map<Operation, Stage> stage_map;
  /*! \brief map of output buffer to the stages */
  Map<NodeRef, Stage> stage_buff_map;
  /*!
   * \brief Internal stage map to map internal ops to stages.
   *  This is created on demand and can be invalidated.
   */
  std::unordered_map<const Node*, Stage> op2stage_cache_;

  std::unordered_map<IterVar, IterVar> extern_itervar_map;

  void VisitAttrs(AttrVisitor* v) final {
    v->Visit("outputs", &outputs);
    v->Visit("stages", &stages);
    v->Visit("groups", &groups);
    v->Visit("stage_map", &stage_map);
    v->Visit("stage_buff_map", &stage_buff_map);
  }

  /*! \brief Initialize temp cache. */
  void InitCache();
  /*! \brief Invalidate temp cache. */
  void InvalidateCache();

  /*!
   * \brief Create a schedule for array of ops(and their dependencies).
   * \param ops The ops to be scheduled.
   * \return sch The created Schedule.
   */
  EXPORT static Schedule make(Array<Operation> ops);

  static constexpr const char* _type_key = "Schedule";
  TVM_DECLARE_NODE_TYPE_INFO(ScheduleNode, Node);
};

/*!
 * \brief Create a schedule for array of ops(and their dependencies).
 * \param ops The ops to be scheduled.
 * \return sch The created Schedule.
 */
inline Schedule create_schedule(Array<Operation> ops) {
  return ScheduleNode::make(ops);
}

/*! \brief node container for IterVar attr */
class IterVarAttrNode : public Node {
 public:
  /*! \brief The iteration type. */
  IterVarType iter_type{kDataPar};
  /*! \brief The thread this iter Var binds, can be null */
  IterVar bind_thread;
  /*! \brief List of tensor to be prefetched in this loop */
  Array<Tensor> prefetch_data;
  /*! \brief The offset used in each prefetch */
  Array<Expr> prefetch_offset;
  /*!
   * \brief Tensor intrinsic used in tensorization,
   *   when the axis is marked as Tensorized
   */
  TensorIntrin tensor_intrin;
  /*! \brief Alignment factor of buffer dimension */
  int dim_align_factor{0};
  /*! \brief Alignment offset of buffer dimension */
  int dim_align_offset{0};
  /*!
   * \brief Additional pragmas, array of StringImm
   */
  Array<Expr> pragmas;
  /*!
   * \brief Annotate keys used in for loop, array of StringImm
   */
  Array<Expr> for_loop_annotate_keys;
  /*!
   * \brief Annotate values used in for loop, array of StringImm
   */
  Array<Expr> for_loop_annotate_values;

  void VisitAttrs(AttrVisitor* v) final {
    v->Visit("iter_type", &iter_type);
    v->Visit("bind_thread", &bind_thread);
    v->Visit("prefetch_data", &prefetch_data);
    v->Visit("prefetch_offset", &prefetch_offset);
    v->Visit("tensor_intrin", &tensor_intrin);
    v->Visit("dim_align_factor", &dim_align_factor);
    v->Visit("dim_align_offset", &dim_align_offset);
    v->Visit("pragmas", &pragmas);
    v->Visit("for_loop_annotate_keys", &for_loop_annotate_keys);
    v->Visit("for_loop_annotate_values", &for_loop_annotate_values);
  }

  static constexpr const char* _type_key = "IterVarAttr";
  TVM_DECLARE_NODE_TYPE_INFO(IterVarAttrNode, Node);
};

/*! \brief base node of iteration var */
class IterVarRelationNode : public Node {
 public:
  static constexpr const char* _type_key = "IterVarRelation";
  TVM_DECLARE_BASE_NODE_INFO(IterVarRelationNode, Node);
};

/*!
 * \brief Split the parent domain into product of
 *  outer and iter.
 */
class SplitNode : public IterVarRelationNode {
 public:
  /*! \brief The parent domain */
  IterVar parent;
  /*! \brief The outer domain */
  IterVar outer;
  /*! \brief The inner domain */
  IterVar inner;
  /*! \brief The split factor */
  Expr factor;
  /*! \brief Number of parts, only factor or nparts can be given */
  Expr nparts;

  void VisitAttrs(AttrVisitor* v) final {
    v->Visit("parent", &parent);
    v->Visit("outer", &outer);
    v->Visit("inner", &inner);
    v->Visit("factor", &factor);
    v->Visit("nparts", &nparts);
  }

  static IterVarRelation make(IterVar parent,
                              IterVar outer,
                              IterVar inner,
                              Expr factor,
                              Expr nparts);

  static constexpr const char* _type_key = "Split";
  TVM_DECLARE_NODE_TYPE_INFO(SplitNode, IterVarRelationNode);
};

/*!
 * \brief Fuse two domains into one domain.
 */
class FuseNode : public IterVarRelationNode {
 public:
  /*! \brief The outer domain */
  IterVar outer;
  /*! \brief The inner domain */
  IterVar inner;
  /*! \brief The target domain */
  IterVar fused;

  void VisitAttrs(AttrVisitor* v) final {
    v->Visit("outer", &outer);
    v->Visit("inner", &inner);
    v->Visit("fused", &fused);
  }

  static IterVarRelation make(
      IterVar outer, IterVar inner, IterVar fused);

  static constexpr const char* _type_key = "Fuse";
  TVM_DECLARE_NODE_TYPE_INFO(FuseNode, IterVarRelationNode);
};

/*!
 * \brief Reorder.
 */
class ReorderNode : public IterVarRelationNode {
 public:
  /*! \brief The order */
  Array<IterVar> order;

  void VisitAttrs(AttrVisitor* v) final {
    v->Visit("order", &order);
  }

  static IterVarRelation make(const Array<IterVar>& order);

  static constexpr const char* _type_key = "Reorder";
  TVM_DECLARE_NODE_TYPE_INFO(ReorderNode, IterVarRelationNode);
};

/*!
 * \brief Rebase the iteration to make min to be 0.
 *  This is useful to normalize the Schedule
 *  to make every leaf variable's min to be 0.
 */
class RebaseNode : public IterVarRelationNode {
 public:
  /*! \brief The parent domain */
  IterVar parent;
  /*! \brief The inner domain */
  IterVar rebased;

  void VisitAttrs(AttrVisitor* v) final {
    v->Visit("parent", &parent);
    v->Visit("rebased", &rebased);
  }

  static IterVarRelation make(IterVar parent, IterVar rebased);

  static constexpr const char* _type_key = "Rebase";
  TVM_DECLARE_NODE_TYPE_INFO(RebaseNode, IterVarRelationNode);
};


// implementations
inline const StageNode* Stage::operator->() const {
  return static_cast<const StageNode*>(node_.get());
}
inline StageNode* Stage::operator->() {
  return static_cast<StageNode*>(node_.get());
}

inline const ScheduleNode* Schedule::operator->() const {
  return static_cast<const ScheduleNode*>(node_.get());
}
inline ScheduleNode* Schedule::operator->() {
  return static_cast<ScheduleNode*>(node_.get());
}

inline const IterVarRelationNode* IterVarRelation::operator->() const {
  return static_cast<const IterVarRelationNode*>(node_.get());
}

inline const IterVarAttrNode* IterVarAttr::operator->() const {
  return static_cast<const IterVarAttrNode*>(node_.get());
}
}  // namespace TVM
#endif  // TVM_SCHEDULE_H_
