# usage: python3 -m unittest discover -s tools/tests
"""Relocation identity must come from objdiff, not exported symbol indices."""
import importlib.util
from pathlib import Path
import unittest


def load(name):
    path = Path(__file__).resolve().parents[1] / (name + '.py')
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


noise = load('check-diff-noise')
diff = load('decomp-diff')


def entry(op='lfs', register='f2', target='@7226', changed=False):
    operands = [] if op == 'bl' else [{'opaque': register}]
    operands.append({'reloc': True})
    return {
        'diff_kind': 'DIFF_ARG_MISMATCH',
        'arg_diff': [{} for _ in operands[:-1]] +
                    ([{'diff_index': 0}] if changed else [{}]),
        'instruction': {
            'parts': [{'opcode': {'mnemonic': op}}] +
                     [{'arg': operand} for operand in operands],
            'formatted': op + ' ' + (register + ', ' if op != 'bl' else '') + target,
            'relocation': {'target_symbol': 0},
        },
    }


class RelocationTests(unittest.TestCase):
    def test_changed_literal(self):
        self.assertEqual(noise.classify_pair(entry(changed=True), entry(target='@5063')),
                         'structural:relocation')

    def test_changed_callee(self):
        self.assertEqual(noise.classify_pair(entry(op='bl', target='old'),
                                            entry(op='bl', target='new', changed=True)),
                         'structural:relocation')

    def test_register_and_relocation(self):
        self.assertEqual(noise.classify_pair(entry(), entry(register='f3', changed=True)),
                         'structural:relocation')

    def test_equal_anonymous_literals(self):
        self.assertEqual(noise.classify_pair(entry(), entry(target='@other')), 'exact')

    def test_register_only(self):
        self.assertEqual(noise.classify_pair(entry(), entry(register='f3')),
                         'ignored:register')

    def test_renderer_uses_resolved_text(self):
        inst = entry(target='real+0x4@sda21')
        self.assertEqual(diff.render_instruction(inst, [{'name': 'wrong'}]),
                         'lfs f2, real+0x4@sda21')
        inst['arg_diff'][-1] = {'diff_index': 0}
        self.assertEqual(diff.render_instruction(inst, [], True),
                         '{lfs f2, real+0x4@sda21}')


if __name__ == '__main__':
    unittest.main()
