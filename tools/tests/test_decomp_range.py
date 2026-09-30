# usage: python3 -m unittest discover -s tools/tests
"""Ranged diffs must not hide insertions at a different object address."""
from contextlib import redirect_stdout
from io import StringIO
from types import SimpleNamespace
import unittest

from test_diff_relocations import diff


def instruction(address, text, kind=''):
    return {'instruction': {'address': str(address),
                            'parts': [{'opcode': {'mnemonic': text}}]},
            'diff_kind': kind}


class RangeTests(unittest.TestCase):
    def render(self, left, right, selected):
        data = {
            'left': {'symbols': [{'name': 'test', 'target_symbol': 1,
                                  'instructions': left}]},
            'right': {'symbols': [{'name': 'test', 'instructions': right}]},
        }
        args = SimpleNamespace(range=selected, context=3, no_collapse=True)
        output = StringIO()
        with redirect_stdout(output):
            diff.build_diff(data, 'test', args)
        return output.getvalue()

    def test_insertion_uses_preceding_target_offset(self):
        output = self.render(
            [instruction(0x100, 'before'), {}, instruction(0x104, 'after')],
            [instruction(0x200, 'before'),
             instruction(0x204, 'inserted', 'DIFF_INSERT'),
             instruction(0x208, 'after')], '100-100')
        self.assertIn('inserted', output)
        self.assertNotIn('after', output)

    def test_leading_insertion_uses_first_target_offset(self):
        output = self.render(
            [{}, instruction(0, 'first')],
            [instruction(0x200, 'inserted', 'DIFF_INSERT'),
             instruction(0x204, 'first')], '0-0')
        self.assertIn('inserted', output)
        self.assertIn('first', output)

    def test_current_only_function_uses_current_offsets(self):
        output = self.render([], [instruction(0x200, 'inserted', 'DIFF_INSERT')],
                             '200-200')
        self.assertIn('inserted', output)


if __name__ == '__main__':
    unittest.main()
