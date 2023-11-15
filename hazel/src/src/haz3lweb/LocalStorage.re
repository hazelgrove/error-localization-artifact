open Sexplib.Std;

// Settings serialization
module Settings = {
  let save_settings_key: string = "SETTINGS";

  let serialize = settings =>
    settings |> ModelSettings.sexp_of_t |> Sexplib.Sexp.to_string;

  let deserialize = data =>
    try(
      data
      |> Sexplib.Sexp.of_string
      |> ModelSettings.t_of_sexp
      |> ModelSettings.fix_instructor_mode
    ) {
    | _ =>
      print_endline("Could not deserialize settings.");
      ModelSettings.init;
    };

  let save = (settings: ModelSettings.t): unit =>
    JsUtil.set_localstore(save_settings_key, serialize(settings));

  let init = () => {
    JsUtil.set_localstore(save_settings_key, serialize(ModelSettings.init));
    ModelSettings.init;
  };

  let load = (): ModelSettings.t =>
    switch (JsUtil.get_localstore(save_settings_key)) {
    | None => init()
    | Some(data) => deserialize(data)
    };

  let export = () => Option.get(JsUtil.get_localstore(save_settings_key));
  let import = data => {
    let settings = deserialize(data);
    save(settings);
    settings;
  };
};

// LangDocMessages serialization
module LangDocMessages = {
  let save_langDocMessages_key: string = "LANGDOCMESSAGES";

  let serialize = langDocMessages =>
    LangDocMessages.serialize(langDocMessages);

  let deserialize = data =>
    try(LangDocMessages.deserialize(data)) {
    | _ =>
      print_endline("Could not deserialize langDocMessages.");
      LangDocMessages.init;
    };

  let save = (langDocMessages: LangDocMessages.t): unit =>
    JsUtil.set_localstore(
      save_langDocMessages_key,
      serialize(langDocMessages),
    );

  let init = () => {
    JsUtil.set_localstore(
      save_langDocMessages_key,
      serialize(LangDocMessages.init),
    );
    LangDocMessages.init;
  };

  let load = (): LangDocMessages.t =>
    switch (JsUtil.get_localstore(save_langDocMessages_key)) {
    | None => init()
    | Some(data) => deserialize(data)
    };

  let rec export = () =>
    switch (JsUtil.get_localstore(save_langDocMessages_key)) {
    | None =>
      let _ = init();
      export();
    | Some(data) => data
    };

  let import = data => {
    let langDocMessages = deserialize(data);
    save(langDocMessages);
  };
};

// Scratch mode serialization
module Scratch = {
  let save_scratch_key: string = "SAVE_SCRATCH";

  [@deriving (show({with_path: false}), sexp, yojson)]
  type persistent = (int, list(ScratchSlide.persistent_state));

  let to_persistent = ((idx, slides): Editors.scratch): persistent => (
    idx,
    List.map(ScratchSlide.persist, slides),
  );

  let of_persistent = ((idx, slides): persistent): Editors.scratch => {
    (idx, List.map(ScratchSlide.unpersist, slides));
  };

  let serialize = scratch => {
    scratch |> to_persistent |> sexp_of_persistent |> Sexplib.Sexp.to_string;
  };

  let deserialize = data => {
    data |> Sexplib.Sexp.of_string |> persistent_of_sexp |> of_persistent;
  };

  let save = (scratch: Editors.scratch): unit => {
    JsUtil.set_localstore(save_scratch_key, serialize(scratch));
  };

  let init = () => {
    let scratch = ScratchSlidesInit.init();
    save(scratch);
    scratch;
  };

  let load = () =>
    switch (JsUtil.get_localstore(save_scratch_key)) {
    | None => init()
    | Some(data) =>
      try(deserialize(data)) {
      | _ => init()
      }
    };

  let export = () => serialize(load());
  let import = data => save(deserialize(data));
};

module School = {
  open SchoolExercise;

  let cur_exercise_key = "CUR_EXERCISE";

  let keystring_of_key = key => {
    key |> sexp_of_key |> Sexplib.Sexp.to_string;
  };

  let keystring_of = p => {
    key_of(p) |> keystring_of_key;
  };

  let key_of_keystring = keystring => {
    keystring |> Sexplib.Sexp.of_string |> key_of_sexp;
  };

  let save_exercise_key = key => {
    JsUtil.set_localstore(cur_exercise_key, keystring_of_key(key));
  };

  let save_exercise = (exercise, ~instructor_mode) => {
    let key = SchoolExercise.key_of_state(exercise);
    let keystring = keystring_of_key(key);
    let value = SchoolExercise.serialize_exercise(exercise, ~instructor_mode);
    JsUtil.set_localstore(keystring, value);
  };

  let init_exercise = (spec, ~instructor_mode) => {
    let key = SchoolExercise.key_of(spec);
    let keystring = keystring_of_key(key);
    let exercise = SchoolExercise.state_of_spec(spec, ~instructor_mode);
    save_exercise(exercise, ~instructor_mode);
    JsUtil.set_localstore(cur_exercise_key, keystring);
    exercise;
  };

  let load_exercise = (key, spec, ~instructor_mode): SchoolExercise.state => {
    let keystring = keystring_of_key(key);
    switch (JsUtil.get_localstore(keystring)) {
    | Some(data) =>
      let exercise =
        try(
          SchoolExercise.deserialize_exercise(data, ~spec, ~instructor_mode)
        ) {
        | _ => init_exercise(spec, ~instructor_mode)
        };
      JsUtil.set_localstore(cur_exercise_key, keystring);
      exercise;
    | None => init_exercise(spec, ~instructor_mode)
    };
  };

  let save = ((n, specs, exercise), ~instructor_mode) => {
    let key = key_of(List.nth(specs, n));
    let keystring = keystring_of_key(key);
    save_exercise(exercise, ~instructor_mode);
    JsUtil.set_localstore(cur_exercise_key, keystring);
  };

  let init = (~instructor_mode) => {
    let school = School.init(~instructor_mode);
    save(school, ~instructor_mode);
    school;
  };

  let load = (~specs, ~instructor_mode): Editors.school => {
    switch (JsUtil.get_localstore(cur_exercise_key)) {
    | Some(keystring) =>
      let key = key_of_keystring(keystring);
      switch (SchoolExercise.find_key_opt(key, specs)) {
      | Some((n, spec)) =>
        switch (JsUtil.get_localstore(keystring)) {
        | Some(data) =>
          let exercise =
            try(deserialize_exercise(data, ~spec, ~instructor_mode)) {
            | _ => init_exercise(spec, ~instructor_mode)
            };
          (n, specs, exercise);
        | None =>
          // initialize exercise from spec
          let exercise = SchoolExercise.state_of_spec(spec, ~instructor_mode);
          save_exercise(exercise, ~instructor_mode);
          (n, specs, exercise);
        }
      | None =>
        // invalid current exercise key saved, load the first exercise
        let first_spec = List.nth(specs, 0);
        let first_key = SchoolExercise.key_of(first_spec);
        (0, specs, load_exercise(first_key, first_spec, ~instructor_mode));
      };
    | None => init(~instructor_mode)
    };
  };

  type school_export = SchoolExercise.school_export;

  let prep_school_export = (~specs, ~instructor_mode) => {
    {
      cur_exercise:
        key_of_keystring(
          Option.get(JsUtil.get_localstore(cur_exercise_key)),
        ),
      exercise_data:
        specs
        |> List.map(spec => {
             let key = SchoolExercise.key_of(spec);
             let exercise =
               load_exercise(key, spec, ~instructor_mode)
               |> SchoolExercise.persistent_state_of_state(~instructor_mode);
             (key, exercise);
           }),
    };
  };

  let serialize_school_export = (~specs, ~instructor_mode) => {
    prep_school_export(~specs, ~instructor_mode)
    |> sexp_of_school_export
    |> Sexplib.Sexp.to_string;
  };

  let export = (~specs, ~instructor_mode) => {
    serialize_school_export(~specs, ~instructor_mode);
  };

  let import = (data, ~specs, ~instructor_mode) => {
    let school_export = data |> deserialize_school_export;
    save_exercise_key(school_export.cur_exercise);
    school_export.exercise_data
    |> List.iter(((key, persistent_state)) => {
         let spec = SchoolExercise.find_key_opt(key, specs);
         switch (spec) {
         | None =>
           print_endline("Warning: saved key does not correspond to exercise")
         | Some((_, spec)) =>
           save_exercise(
             SchoolExercise.unpersist_state(
               persistent_state,
               ~spec,
               ~instructor_mode,
             ),
             ~instructor_mode,
           )
         };
       });
  };
};
