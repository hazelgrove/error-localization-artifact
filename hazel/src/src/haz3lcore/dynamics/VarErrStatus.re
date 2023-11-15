module HoleReason = {
  /* Variable: reason */
  [@deriving (show({with_path: false}), sexp, yojson)]
  type t =
    | Free
    | ExpandingKeyword(ExpandingKeyword.t);
};

/* Variable: var_err */
[@deriving (show({with_path: false}), sexp, yojson)]
type t =
  | NotInVarHole
  | InVarHole(HoleReason.t, MetaVar.t);
