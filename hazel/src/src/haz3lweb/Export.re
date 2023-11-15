open Sexplib.Std;

[@deriving (show({with_path: false}), sexp, yojson)]
type all = {
  settings: string,
  langDocMessages: string,
  scratch: string,
  school: string,
  log: string,
};

// fallback for saved state prior to release of lang doc in 490F22
[@deriving (show({with_path: false}), sexp, yojson)]
type all_f22 = {
  settings: string,
  scratch: string,
  school: string,
  log: string,
};

let mk_all = (~instructor_mode) => {
  print_endline("Mk all");
  let settings = LocalStorage.Settings.export();
  print_endline("Settings OK");
  let langDocMessages = LocalStorage.LangDocMessages.export();
  print_endline("LangDocMessages OK");
  let scratch = LocalStorage.Scratch.export();
  print_endline("Scratch OK");
  let specs = School.exercises;
  let school = LocalStorage.School.export(~specs, ~instructor_mode);
  print_endline("School OK");
  let log = Log.export();
  {settings, langDocMessages, scratch, school, log};
};

let export_all = (~instructor_mode) => {
  mk_all(~instructor_mode) |> yojson_of_all;
};

let import_all = (data, ~specs) => {
  let all =
    try(data |> Yojson.Safe.from_string |> all_of_yojson) {
    | _ =>
      let all_f22 = data |> Yojson.Safe.from_string |> all_f22_of_yojson;
      {
        settings: all_f22.settings,
        scratch: all_f22.scratch,
        school: all_f22.school,
        log: all_f22.log,
        langDocMessages: "",
      };
    };
  let settings = LocalStorage.Settings.import(all.settings);
  LocalStorage.LangDocMessages.import(all.langDocMessages);
  let instructor_mode = settings.instructor_mode;
  LocalStorage.Scratch.import(all.scratch);
  LocalStorage.School.import(all.school, ~specs, ~instructor_mode);
  Log.import(all.log);
};
