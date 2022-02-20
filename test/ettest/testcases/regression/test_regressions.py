"""
Regression tests for eastwood-tidy
Use for errors that crop up over time and bugs that occur to check that we do not
let them happen in the future
"""

from ettest.testcases.snippets.snippets import Snippet, Error
from ettest.fixtures import manager
from ettest.rule import Rule
from ettest.filetest import FileTest


def test_space_alignment_71(manager) -> None:
    """
    Test alignment of comments with multiple spaces

    https://github.com/novafacing/eastwood-tidy/issues/71
    """
    snippets = [
        # This should be allowed
        Snippet(
            [
                "  int something = 0;      // We align this comment",
                "  int something_else = 0; // This is a data item",
            ]
        ),
        Snippet(
            [
                "  int something = 0;  // We align this comment",
            ]
        ),
    ]
    for snip in snippets:
        res = manager.test_snippet(snip)
        assert not res.unexpected_errors
        assert not res.unseen_errors


def test_define_in_function_72(manager) -> None:
    """
    Test define inside function body

    https://github.com/novafacing/eastwood-tidy/issues/72
    """
    snippets = [
        # This should be allowed
        Snippet(
            [
                "#define SOMETHING (0)",
                "  int something = 0;",
            ]
        )
    ]
    for snip in snippets:
        res = manager.test_snippet(snip)
        assert not res.unexpected_errors
        assert not res.unseen_errors


def test_header_comments_73(manager) -> None:
    """
    Test header comment detection for bool returning functions

    https://github.com/novafacing/eastwood-tidy/issues/72
    """
    tests = [FileTest("header_comments_73.c", [])]
    for test in tests:
        res = manager.test_file(test)
        assert not res.unexpected_errors
        assert not res.unseen_errors
