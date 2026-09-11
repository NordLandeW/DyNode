import contextlib
import io
import json
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import Mock, patch

import requests

import pre_deploy


class TranslationTests(unittest.TestCase):
    source = "该版本包含一些错误修复。\n\n* 修复了内存泄漏问题。\n"

    def response(self, content, status="completed"):
        return Mock(json=lambda: {
            "status": status,
            "output": [
                {"type": "reasoning", "summary": []},
                {"type": "message", "content": content},
            ],
        })

    @patch.dict(os.environ, {"OPENROUTER_API_KEY": "test-key"}, clear=True)
    @patch("requests.post")
    def test_translates_response_text_and_preserves_output_contract(self, post):
        translation = "This version includes bug fixes.\n\n* Fixed memory leaks.\n"
        raw = json.dumps({"en-us": translation}, ensure_ascii=False)
        post.return_value = self.response([
            {"type": "output_text", "text": raw[:10]},
            {"type": "output_text", "text": raw[10:]},
        ])
        result = pre_deploy.translate_with_openrouter(self.source, ["zh-cn", "en-us"])
        self.assertEqual(result, {"en-us": translation})
        url = post.call_args.args[0]
        request = post.call_args.kwargs
        self.assertEqual(url, "https://openrouter.ai/api/v1/responses")
        self.assertEqual(request["headers"]["Authorization"], "Bearer test-key")
        self.assertEqual(request["json"]["model"], "google/gemini-3.8-flash")
        self.assertEqual(request["json"]["provider"], {
            "only": ["google-vertex/global"], "allow_fallbacks": False,
        })
        self.assertIn(self.source, request["json"]["input"])
        self.assertIn("Markdown", request["json"]["instructions"])
        self.assertFalse(request["json"]["store"])
        with tempfile.TemporaryDirectory() as tmp:
            changelog = Path(tmp) / "changelog.json"
            release = Path(tmp) / "releaselog.txt"
            with patch.object(pre_deploy, "OUTPUT_CHANGELOG", changelog), \
                 patch.object(pre_deploy, "OUTPUT_RELEASELOG", release), \
                 contextlib.redirect_stdout(io.StringIO()):
                pre_deploy.write_outputs("v-test", True, result, self.source)
            self.assertEqual(json.loads(changelog.read_text(encoding="utf-8")), {
                "version": "v-test", "zh-cn": self.source, "en-us": translation,
            })
            self.assertEqual(release.read_text(encoding="utf-8"),
                             f"{self.source}\n\n---------\n{translation}\n")

    @patch.dict(os.environ, {"OPENROUTER_API_KEY": "test-key"}, clear=True)
    @patch("requests.post")
    def test_invalid_or_missing_language_keeps_chinese_only_for_that_language(self, post):
        post.return_value = self.response([{"type": "output_text", "text": json.dumps({
            "en-us": "Fixed memory leaks.", "ja-jp": " ", "extra": "ignored",
        })}])
        result = pre_deploy.translate_with_openrouter(self.source, ["en-us", "ja-jp", "zh-tw"])
        self.assertEqual(result, {"en-us": "Fixed memory leaks.",
                                  "ja-jp": self.source, "zh-tw": self.source})

    @patch.dict(os.environ, {"OPENROUTER_API_KEY": "test-key"}, clear=True)
    @patch("requests.post")
    def test_failed_incomplete_or_invalid_response_keeps_chinese(self, post):
        cases = [
            self.response([], "incomplete"),
            self.response([]),
            self.response([{"type": "refusal", "refusal": "No translation"}]),
            self.response([{"type": "output_text", "text": "not JSON"}]),
            self.response([{"type": "output_text", "text": "[]"}]),
        ]
        http_error = self.response([])
        http_error.raise_for_status.side_effect = requests.HTTPError("503 Service Unavailable")
        cases.append(http_error)
        for response in cases:
            with self.subTest(response=response):
                post.return_value = response
                self.assertEqual(pre_deploy.translate_with_openrouter(self.source, ["en-us"]),
                                 {"en-us": self.source})

    @patch.dict(os.environ, {}, clear=True)
    @patch("requests.post")
    def test_missing_key_keeps_chinese_without_sending_request(self, post):
        self.assertEqual(pre_deploy.translate_with_openrouter(self.source, ["en-us"]),
                         {"en-us": self.source})
        post.assert_not_called()

    @patch("requests.post")
    def test_no_target_language_needs_no_request(self, post):
        self.assertEqual(pre_deploy.translate_with_openrouter(self.source, ["zh-cn"]), {})
        post.assert_not_called()


if __name__ == "__main__":
    unittest.main()
