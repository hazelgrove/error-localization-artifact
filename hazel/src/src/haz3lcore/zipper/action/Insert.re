open Zipper;
open Util;
open OptUtil.Syntax;

let barf = (d: Direction.t, (z, id_gen): state): option(state) => {
  /* Removes the d-neighboring tile and drops from backpack;
     precondition: the d-neighbor should be a monotile
     string-matching the dropping shard */
  let* z = delete(d, z);
  let+ z = put_down(d, z);
  (z, id_gen);
};

let delayed_expand =
    (t: Token.t, caret: Direction.t, (z, id_gen): state): option(state) => {
  /* Removes the d-neighboring tile and reconstructs it, triggering
     keyword-expansion; precondition: the d-neighbor should be a monotile
     string-matching a keyword of an expanding form */
  let (new_label, backpack) = Molds.delayed_expansion(t);
  let+ z = delete(caret, z);
  construct(~backpack, ~caret, new_label, z, id_gen);
};

let expand_or_barf_left_neighbor = ((z, _) as s: state): option(state) =>
  /* If left neighbor is a monotile (a) string-matching the shard at the
     top of the backpack, barf it, or (b) an expansing keyword, expand it. */
  switch (left_neighbor_monotile(z.relatives.siblings)) {
  | Some(t) when Backpack.will_barf(t, z.backpack) => barf(Left, s)
  | Some(t) when Molds.is_delayed(t) => delayed_expand(t, Left, s)
  | _ => Some(s)
  };

let expand_or_barf_right_neighbor = ((z, _) as s: state): option(state) =>
  /* If right neighbor is a monotile (a) string-matching the shard at the
     top of the backpack, barf it, or (b) an expansing keyword, expand it. */
  switch (right_neighbor_monotile(z.relatives.siblings)) {
  | Some(t) when Backpack.will_barf(t, z.backpack) => barf(Right, s)
  | Some(t) when Molds.is_delayed(t) => delayed_expand(t, Right, s)
  | _ => Some(s)
  };

let get_duo_shard = ({label, shards, _}: Tile.t) =>
  if (List.length(label) == 2 && List.length(shards) == 1) {
    List.nth_opt(label, List.hd(shards));
  } else {
    None;
  };

let neighbor_can_duomerge =
    (t: Token.t, s: Siblings.t): option((Label.t, Direction.t)) =>
  /* Checks if a neighbor, preferentially the left neighbor, is
     a shard of a duotile which can be merged to form a monotile.
     It returns the resulting (mono)label, and the direction of
     the relevant neighbor. */
  switch (Siblings.neighbors(s)) {
  | (Some(Tile(tile)), _) =>
    let* start = get_duo_shard(tile);
    let+ mono_lbl = Form.duomerges([start, t]);
    (mono_lbl, Direction.Left);
  | (_, Some(Tile(tile))) =>
    let* last = get_duo_shard(tile);
    let+ mono_lbl = Form.duomerges([t, last]);
    (mono_lbl, Direction.Right);
  | _ => None
  };

let make_new_tile = (t: Token.t, caret: Direction.t, z: t, id_gen): state =>
  /* Adds a new tile at the caret. If the new token matches the top
     of the backpack, the backpack shard is dropped. Otherwise, we
     construct a new tile, which may immediately expand. */
  Backpack.will_barf(t, z.backpack)
    ? switch (neighbor_can_duomerge(t, z.relatives.siblings)) {
      | Some((lbl, d)) =>
        Zipper.replace(~caret=d, ~backpack=d, lbl, (z, id_gen)) |> Option.get
      | None => (put_down(caret, z) |> Option.get, id_gen)
      }
    : {
      let (lbl, backpack) = Molds.instant_expansion(t);
      construct(~caret, ~backpack, lbl, z, id_gen);
    };

let expand_neighbors_and_make_new_tile =
    (char: Token.t, state: state): option(state) => {
  /* Trigger a token boundary event and create a new tile.
     This process potentially involves both neighboring tiles,
     potentially triggering up to 3 expansions or backpack barfs.
     In particular, both left and right neighboring monotiles may
     undergo delayed (aka keyword) expansion, and the newly-created
     single-character token may undergo instant expansion. Currently
     made the decision to expand or barf the neighbors before making
     the new tile because barfing is limited to the top of the backpack,
     and I wanted things like "if|then", when you enter a "(", to
     barf the "then", before it is buried by the ")" added to the BP.
     The order here could be revisited if barfing was more sophisticated.
     */
  let* (z, id_gen) = expand_or_barf_left_neighbor(state);
  //let (z, id_gen) = regrout(Left, z, id_gen);
  /* Note to david: I'm not sure why the above regrout is necessary.
     Without it, there is a Nonconvex segment error thrown in exactly
     one case, the double barf case: insert space on "if then|else" */
  let+ (z, id_gen) = expand_or_barf_right_neighbor((z, id_gen));
  make_new_tile(char, Left, z, id_gen);
};

let replace_tile =
    (t: Token.t, d: Direction.t, (z, id_gen): state): option(state) => {
  let+ z = delete(d, z);
  make_new_tile(t, d, z, id_gen);
};

[@deriving (show({with_path: false}), sexp, yojson)]
type appendability =
  | AppendLeft(Token.t)
  | AppendRight(Token.t)
  | MakeNew;

let sibling_appendability: (string, Siblings.t) => appendability =
  (char, siblings) =>
    switch (neighbor_monotiles(siblings)) {
    | (Some(t), _) when Form.is_valid_token(t ++ char) =>
      AppendLeft(t ++ char)
    | (_, Some(t))
        when Form.is_valid_token(char ++ t) && !Form.is_comment_delim(char) =>
      AppendRight(char ++ t)
    | _ => MakeNew
    };

let insert_outer = (char: string, (z, _) as state: state): option(state) =>
  switch (sibling_appendability(char, z.relatives.siblings)) {
  | MakeNew => expand_neighbors_and_make_new_tile(char, state)
  | AppendLeft(t) => replace_tile(t, Left, state)
  | AppendRight(t) => replace_tile(t, Right, state)
  };

let insert_duo = (id_gen, lbl: Label.t, z: option(t)): option(state) =>
  z
  |> Option.map(z =>
       Zipper.construct(~caret=Left, ~backpack=Left, lbl, z, id_gen)
     )
  |> OptUtil.and_then(((z, id_gen)) =>
       Zipper.put_down(Left, z)  //TODO(andrew): direction?
       |> OptUtil.and_then(Zipper.move(Left))
       |> Option.map(z => (z, id_gen))
     );

let insert_monos =
    (id_gen, l: Token.t, r: Token.t, z: option(t)): option(state) =>
  z
  |> Option.map(z => Zipper.construct_mono(Right, r, z, id_gen))
  |> Option.map(((z, id_gen)) => Zipper.construct_mono(Left, l, z, id_gen));

let split =
    ((z, id_gen): state, char: string, idx: int, t: Token.t): option(state) => {
  let (l, r) = Token.split_nth(idx, t);
  z
  |> Zipper.set_caret(Outer)
  |> Zipper.select(Right)
  |> (
    /* overwrite selection */
    switch (Form.duomerges([l, r])) {
    | Some(_) => insert_duo(id_gen, [l, r])
    | None => insert_monos(id_gen, l, r)
    }
  )
  |> OptUtil.and_then(expand_neighbors_and_make_new_tile(char));
};

let opt_regrold = d =>
  Option.map(((z, id_gen)) => remold_regrout(d, z, id_gen));

let move_into_if_stringlit_or_comment = (char, z) =>
  /* This is special-case logic for advancing the caret to position between the quotes
     in newly-created stringlits. The main stringlit special-case is in Zipper.constuct
     and ideally this logic would be located there as well, but both regrouting and
     subsequent caret position logic at this function's callsites dicate that this
     be done after. Not too happy about this tbh. */
  Form.is_string_delim(char) || Form.is_comment_delim(char)
    ? switch (move(Left, z)) {
      | None => z
      | Some(z) => z |> set_caret(Inner(0, 0))
      }
    : z;

let go =
    (
      char: string,
      ({caret, relatives: {siblings, _}, _} as z, id_gen): state,
    )
    : option(state) => {
  /* If there's a selection, delete it before proceeding */
  let z = z.selection.content != [] ? Zipper.destruct(z) : z;
  switch (caret, neighbor_monotiles(siblings)) {
  /* Special cases for insertion of quotes when the caret is
     in or is adjacent to a string/comment. This is necessary to
     avoid breaking paste. */
  | (_, (_, Some(t)))
      when
        Form.is_string(t)
        && Form.is_string_delim(char)
        || Form.is_comment(t)
        && Form.is_comment_delim(char) =>
    z
    |> Zipper.set_caret(Outer)
    |> Zipper.move(Right)
    |> Option.map(z => (z, id_gen))
  | (Outer, (Some(t), _))
      when
        Form.is_string(t)
        && Form.is_string_delim(char)
        || Form.is_comment(t)
        && Form.is_comment_delim(char) =>
    Some((z, id_gen))
  | (Inner(d_idx, n), (_, Some(t))) =>
    let idx = n + 1;
    let new_t = Token.insert_nth(idx, char, t);
    /* If inserting wouldn't produce a valid token, split */
    Form.is_valid_token(new_t)
      ? z
        |> Zipper.set_caret(Inner(d_idx, idx))
        |> (z => Zipper.replace_mono(Right, new_t, (z, id_gen)))
        |> opt_regrold(Left)
      : split((z, id_gen), char, idx, t) |> opt_regrold(Right);
  /* Can't insert inside delimiter */
  | (Inner(_, _), (_, None)) => None
  | (Outer, (_, Some(_))) =>
    let caret: Zipper.Caret.t =
      /* If we're adding to the right, move caret inside right nhbr */
      switch (sibling_appendability(char, siblings)) {
      | AppendRight(_) => Inner(0, 0) //Note: assumption of monotile
      | MakeNew
      | AppendLeft(_) => Outer
      };
    (z, id_gen)
    |> insert_outer(char)
    |> Option.map(((z, id_gen)) => (Zipper.set_caret(caret, z), id_gen))
    |> opt_regrold(Left)
    |> Option.map(((z, id_gen)) =>
         (move_into_if_stringlit_or_comment(char, z), id_gen)
       );
  | (Outer, (_, None)) =>
    insert_outer(char, (z, id_gen))
    |> opt_regrold(Left)
    |> Option.map(((z, id_gen)) =>
         (move_into_if_stringlit_or_comment(char, z), id_gen)
       )
  };
};
