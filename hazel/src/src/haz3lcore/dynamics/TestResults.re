open Sexplib.Std;

[@deriving (show({with_path: false}), sexp, yojson)]
type test_results = {
  test_map: TestMap.t,
  statuses: list(TestStatus.t),
  descriptions: list(string),
  total: int,
  passing: int,
  failing: int,
  unfinished: int,
};

let mk_results = (~descriptions=[], test_map: TestMap.t): test_results => {
  test_map,
  statuses: test_map |> List.map(r => r |> snd |> TestMap.joint_status),
  descriptions,
  total: TestMap.count(test_map),
  passing: TestMap.count_status(Pass, test_map),
  failing: TestMap.count_status(Fail, test_map),
  unfinished: TestMap.count_status(Indet, test_map),
};

let result_summary_str =
    (~n, ~p, ~q, ~n_str, ~ns_str, ~p_str, ~q_str, ~r_str): string => {
  let one_p = "one is " ++ p_str ++ " ";
  let one_q = "one is " ++ q_str ++ " ";
  let mny_p = Printf.sprintf("%d are %s ", p, p_str);
  let mny_q = Printf.sprintf("%d are %s ", q, q_str);
  let of_n = Printf.sprintf("Out of %d %s, ", n, ns_str);
  switch (n, p, q) {
  | (0, _, _) => "No " ++ ns_str ++ " available."
  | (_, 0, 0) => "All " ++ ns_str ++ " " ++ r_str ++ "! "
  | (n, _, c) when n == c => "All " ++ ns_str ++ " " ++ q_str ++ " "
  | (n, f, _) when n == f => "All " ++ ns_str ++ " " ++ p_str ++ " "
  | (1, 0, 1) => "One " ++ n_str ++ " " ++ q_str ++ " "
  | (1, 1, 0) => "One " ++ n_str ++ " " ++ p_str ++ " "
  | (2, 1, 1) =>
    "One " ++ n_str ++ " " ++ p_str ++ " and one " ++ q_str ++ " "
  | (_, 0, 1) => of_n ++ one_q
  | (_, 1, 0) => of_n ++ one_p
  | (_, 1, 1) => of_n ++ one_p ++ "and " ++ one_q
  | (_, 1, _) => of_n ++ one_p ++ "and " ++ mny_q
  | (_, _, 1) => of_n ++ mny_p ++ "and " ++ one_q
  | (_, 0, _) => of_n ++ mny_q
  | (_, _, 0) => of_n ++ mny_p
  | (_, _, _) => of_n ++ mny_p ++ "and " ++ mny_q
  };
};

/*
 let test_summary_str = (~test_map: TestMap.t): string => {
   let total = TestMap.count(test_map);
   let failing = TestMap.count_status(Fail, test_map);
   let unfinished = TestMap.count_status(Indet, test_map);
   let one_failing = "one is failing ";
   let one_unfinished = "one is unfinished ";
   let mny_failing = Printf.sprintf("%d are failing ", failing);
   let mny_unfinished = Printf.sprintf("%d are unfinished ", unfinished);
   let of_n_tests = Printf.sprintf("Out of %d tests, ", total);
   switch (total, failing, unfinished) {
   | (_, 0, 0) => "All tests passing! "
   | (n, _, c) when n == c => "All tests unfinished "
   | (n, f, _) when n == f => "All tests failing "
   | (1, 0, 1) => "One test unfinished "
   | (1, 1, 0) => "One test failing "
   | (2, 1, 1) => "One test failing and one unfinished "
   | (_, 0, 1) => of_n_tests ++ one_unfinished
   | (_, 1, 0) => of_n_tests ++ one_failing
   | (_, 1, 1) => of_n_tests ++ one_failing ++ "and " ++ one_unfinished
   | (_, 1, _) => of_n_tests ++ one_failing ++ "and " ++ mny_unfinished
   | (_, _, 1) => of_n_tests ++ mny_failing ++ "and " ++ one_unfinished
   | (_, 0, _) => of_n_tests ++ mny_unfinished
   | (_, _, 0) => of_n_tests ++ mny_failing
   | (_, _, _) => of_n_tests ++ mny_failing ++ "and " ++ mny_unfinished
   };
 };
 */

let test_summary_str = (test_results: test_results): string =>
  result_summary_str(
    ~n=test_results.total,
    ~p=test_results.failing,
    ~q=test_results.unfinished,
    ~n_str="test",
    ~ns_str="tests",
    ~p_str="failing",
    ~q_str="indeterminate",
    ~r_str="passing",
  );

type simple_data = {
  eval_result: DHExp.t,
  test_results,
};

type simple = option(simple_data);

let unwrap_test_results = (simple: simple): option(test_results) => {
  Option.map(simple_data => simple_data.test_results, simple);
};
