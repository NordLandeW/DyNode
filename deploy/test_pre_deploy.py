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

    def response(self, parts, finish_reason="STOP"):
        return Mock(json=lambda: {
            "candidates": [{
                "content": {"role": "model", "parts": parts},
                "finishReason": finish_reason,
            }],
        })

    @patch.dict(os.environ, {"GEMINI_API_KEY": "test-key"}, clear=True)
    @patch("requests.post")
    def test_translates_visible_parts_and_preserves_output_contract(self, post):
        translation = "This version includes bug fixes.\n\n* Fixed memory leaks.\n"
        raw = json.dumps({"en-us": translation}, ensure_ascii=False)
        post.return_value = self.response([
            {"thought": True, "text": "Internal translation reasoning."},
            {"text": raw[:10]},
            {"text": raw[10:], "thought": False},
        ])
        result = pre_deploy.translate_with_gemini(self.source, ["zh-cn", "en-us"])
        self.assertEqual(result, {"en-us": translation})
        url = post.call_args.args[0]
        request = post.call_args.kwargs
        self.assertEqual(url, "https://generativelanguage.googleapis.com/v1beta/models/gemini-3.8-flash:generateContent")
        self.assertEqual(request["headers"]["x-goog-api-key"], "test-key")
        self.assertEqual(request["json"]["contents"][0]["role"], "user")
        self.assertIn(self.source, request["json"]["contents"][0]["parts"][0]["text"])
        self.assertIn("Markdown", request["json"]["systemInstruction"]["parts"][0]["text"])
        self.assertEqual(request["json"]["generationConfig"], {"responseMimeType": "application/json"})
        self.assertEqual(request["timeout"], (15, 120))
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

    @patch.dict(os.environ, {"GEMINI_API_KEY": "test-key"}, clear=True)
    @patch("requests.post")
    def test_invalid_or_missing_language_keeps_chinese_only_for_that_language(self, post):
        post.return_value = self.response([{"text": json.dumps({
            "en-us": "Fixed memory leaks.", "ja-jp": " ", "extra": "ignored",
        })}])
        result = pre_deploy.translate_with_gemini(self.source, ["en-us", "ja-jp", "zh-tw"])
        self.assertEqual(result, {"en-us": "Fixed memory leaks.",
                                  "ja-jp": self.source, "zh-tw": self.source})

    @patch.dict(os.environ, {"GEMINI_API_KEY": "test-key"}, clear=True)
    @patch("requests.post")
    def test_failed_incomplete_or_invalid_response_keeps_chinese(self, post):
        cases = [
            self.response([{"text": '{"en-us":"partial"}'}], "MAX_TOKENS"),
            self.response([{"text": '{"en-us":"blocked"}'}], "SAFETY"),
            self.response([], None),
            self.response([]),
            self.response([{"thought": True, "text": '{"en-us":"internal thought"}'}]),
            self.response([{"functionCall": {"name": "translate", "args": {}}}]),
            self.response([{"text": "not JSON"}]),
            self.response([{"text": "[]"}]),
            Mock(json=lambda: {"promptFeedback": {"blockReason": "SAFETY"}}),
            Mock(json=lambda: {"candidates": []}),
        ]
        http_error = self.response([])
        http_error.raise_for_status.side_effect = requests.HTTPError("503 Service Unavailable")
        cases.append(http_error)
        for response in cases:
            with self.subTest(response=response):
                post.return_value = response
                self.assertEqual(pre_deploy.translate_with_gemini(self.source, ["en-us"]),
                                 {"en-us": self.source})

    @patch.dict(os.environ, {}, clear=True)
    @patch("requests.post")
    def test_missing_key_keeps_chinese_without_sending_request(self, post):
        self.assertEqual(pre_deploy.translate_with_gemini(self.source, ["en-us"]),
                         {"en-us": self.source})
        post.assert_not_called()

    @patch("requests.post")
    def test_no_target_language_needs_no_request(self, post):
        self.assertEqual(pre_deploy.translate_with_gemini(self.source, ["zh-cn"]), {})
        post.assert_not_called()


if __name__ == "__main__":
    unittest.main()
