/**
  The result of a program evaluation. Includes the {!type:EvaluatorResult.t},
  the {!type:EvaluatorState}, and the tracked hole instance information
  ({!type:HoleInstanceInfo.t}). Constructed by {!val:Program.get_result}.
 */
[@deriving (show({with_path: false}), sexp, yojson)]
type t = (EvaluatorResult.t, EvaluatorState.t, HoleInstanceInfo.t);

/**
  [get_dhexp r] is the {!type:DHExp.t} in [r].
 */
let get_dhexp: t => DHExp.t;

let get_state: t => EvaluatorState.t;

/**
  [get_hii r] is the {!type:HoleInstanceInfo.t} in [r].
 */
let get_hii: t => HoleInstanceInfo.t;

/**
  [fast_equal (r1, hii1, _, _) (r2, hii2, _, _) ] is checks if [hii1] and
  [hii2] are equal and computes [EvaluatorResult.fast_equal r1 r2].
 */
let fast_equal: (t, t) => bool;
