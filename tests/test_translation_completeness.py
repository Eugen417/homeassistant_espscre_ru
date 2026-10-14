"""Release checks reject missing full-language keys but allow regional overrides."""
from contextlib import redirect_stdout
import importlib.util
import io
from pathlib import Path
import unittest
from unittest.mock import patch


class TranslationCompletenessTests(unittest.TestCase):
    def test_missing_editor_or_firmware_translation_fails(self):
        path = Path(__file__).resolve().parents[1] / 'tools/i18n.py'
        spec = importlib.util.spec_from_file_location('translation_check', path)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        with redirect_stdout(io.StringIO()):
            self.assertEqual(module.check(), 0)  # en-GB inherits untranslated keys from en.
        for group, key in [('editor.pages', 'title'), ('screen.status', 'configuration_problem')]:
            languages = module.languages()
            target = languages['de']
            for part in group.split('.'): target = target[part]
            del target[key]
            output = io.StringIO()
            with patch.object(module, 'languages', return_value=languages), redirect_stdout(output):
                self.assertEqual(module.check(), 1)
            self.assertIn(f'{group}.{key}', output.getvalue())
