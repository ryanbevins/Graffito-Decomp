"""Regression diagnostics must expose losses hidden by aggregate gains."""
import importlib.util
import io
from contextlib import redirect_stdout
from pathlib import Path
import unittest


spec = importlib.util.spec_from_file_location(
    "report_delta", Path(__file__).parents[1] / "agent" / "report_delta.py")
report_delta = importlib.util.module_from_spec(spec)
spec.loader.exec_module(report_delta)


def unit(**scores):
    return {"functions": [{"name": name, "fuzzy_match_percent": score}
                          for name, score in scores.items()]}


class FunctionDiagnostics(unittest.TestCase):
    def output(self, before, after, all_changes=False):
        with redirect_stdout(io.StringIO()) as output:
            report_delta.show_functions("test", before, after, all_changes)
        return output.getvalue()

    def test_loss_is_visible_despite_larger_sibling_gain(self):
        output = self.output(unit(a=80, b=99.7), unit(a=95, b=98.8))
        self.assertIn("Function regression: test/b: 99.700000 -> 98.800000", output)
        self.assertNotIn("test/a", output)

    def test_optional_gains_and_unchanged_functions(self):
        output = self.output(unit(a=80, b=100), unit(a=95, b=100), True)
        self.assertIn("Function gain: test/a: 80.000000 -> 95.000000", output)
        self.assertNotIn("test/b", output)

    def test_missing_or_unmatched_function_is_a_loss(self):
        for after in (unit(), {"functions": [{"name": "a"}]}):
            self.assertIn("100.000000 -> 0.000000",
                          self.output(unit(a=100), after))


if __name__ == "__main__":
    unittest.main()
