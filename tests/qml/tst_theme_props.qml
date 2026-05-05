// Tests for Theme property values
// Validates: Requirements 14.1, 14.2, 14.3

import QtQuick 2.15
import QtTest 1.0
import LilypadTheme 1.0

TestCase {
    name: "ThemeProps"

    // Req 14.1: Theme.mutedBalticBlue === "#0f4070"
    function test_mutedBalticBlue_value() {
        compare(Theme.mutedBalticBlue.toString().toLowerCase(), "#0f4070")
    }

    // Req 14.2: Theme.darkUrlBar === Theme.shadowGrey
    function test_darkUrlBar_equals_shadowGrey() {
        compare(Theme.darkUrlBar.toString().toLowerCase(),
                Theme.shadowGrey.toString().toLowerCase())
    }

    // Req 14.3: Theme.darkHover === Theme.mutedBalticBlue
    function test_darkHover_equals_mutedBalticBlue() {
        compare(Theme.darkHover.toString().toLowerCase(),
                Theme.mutedBalticBlue.toString().toLowerCase())
    }
}
