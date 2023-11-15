[@deriving (show({with_path: false}), sexp, yojson)]
type t = (EvaluatorResult.t, EvaluatorState.t, HoleInstanceInfo.t);

let get_dhexp = ((r, _, _): t) => EvaluatorResult.unbox(r);
let get_state = ((_, es, _): t) => es;
let get_hii = ((_, _, hii): t) => hii;

let fast_equal_hii = (hii1, hii2) => {
  let fast_equal_his = (his1, his2) =>
    List.equal(
      ((sigma1, _), (sigma2, _)) =>
        ClosureEnvironment.id_equal(sigma1, sigma2)
        /* Check that variable mappings in ClosureEnvironment are equal */
        && List.equal(
             ((x1, d1), (x2, d2)) => x1 == x2 && DHExp.fast_equal(d1, d2),
             ClosureEnvironment.to_list(sigma1),
             ClosureEnvironment.to_list(sigma2),
           ),
      his1,
      his2,
    );

  MetaVarMap.equal(fast_equal_his, hii1, hii2);
};

let fast_equal = ((r1, _, hii1): t, (r2, _, hii2): t): bool =>
  fast_equal_hii(hii1, hii2) && EvaluatorResult.fast_equal(r1, r2);
