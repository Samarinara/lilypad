// Unit tests and property-based tests for TabEntryWidget.
//
// Unit tests (task 6.1):
//   - setActive(true) applies highlight styling
//   - setActive(false) removes highlight styling
//   - setLoading(true) shows spinner and hides favicon
//   - setLoading(false) shows favicon and hides spinner
//   - setTitle("") with a URL set displays the URL
//
// Property-based tests:
//   Property 13 — Title/URL fallback (task 6.2)
//     Validates: Requirements 2.3
//   Property 14 — Loading state display round-trip (task 6.3)
//     Validates: Requirements 7.3, 7.4

#include <QtTest/QtTest>
#include <QApplication>
#include <QLabel>
#include <QPushButton>
#include <rapidcheck.h>

#include "tab_entry_widget.h"

// ---------------------------------------------------------------------------
// Helper: retrieve the private QLabel children by position in the layout.
// The layout order is: faviconLabel (index 0), titleLabel (index 1),
// closeBtn (index 2).
// ---------------------------------------------------------------------------
static QLabel* faviconLabel(TabEntryWidget& w)
{
    // First QLabel child
    QList<QLabel*> labels = w.findChildren<QLabel*>();
    return labels.isEmpty() ? nullptr : labels.at(0);
}

static QLabel* titleLabel(TabEntryWidget& w)
{
    QList<QLabel*> labels = w.findChildren<QLabel*>();
    return (labels.size() >= 2) ? labels.at(1) : nullptr;
}

// ---------------------------------------------------------------------------
// Test class
// ---------------------------------------------------------------------------

class TestTabEntryWidget : public QObject {
    Q_OBJECT

private slots:
    // -----------------------------------------------------------------------
    // Unit tests (task 6.1)
    // -----------------------------------------------------------------------

    // setActive(true) should enable autoFillBackground and use highlight color
    void testSetActiveTrue();

    // setActive(false) should disable autoFillBackground
    void testSetActiveFalse();

    // setLoading(true) should show "⟳" text in favicon label
    void testSetLoadingTrue();

    // setLoading(false) should clear the spinner text and restore pixmap/placeholder
    void testSetLoadingFalse();

    // setTitle("") with a URL set should display the URL in the title label
    void testEmptyTitleDisplaysUrl();

    // -----------------------------------------------------------------------
    // Property-based tests
    // -----------------------------------------------------------------------

    // Property 13: Title/URL fallback
    // Validates: Requirements 2.3
    void prop13_emptyTitleDisplaysUrl();

    // Property 14: Loading state display round-trip
    // Validates: Requirements 7.3, 7.4
    void prop14_loadingStateDisplayRoundTrip();
};

// ---------------------------------------------------------------------------
// Unit test: setActive(true) applies highlight styling
// ---------------------------------------------------------------------------
void TestTabEntryWidget::testSetActiveTrue()
{
    TabEntryWidget w(1);
    w.show();  // ensure the widget is realized

    w.setActive(true);

    QVERIFY(w.autoFillBackground() == true);
    // The window color should be the highlight color
    QColor windowColor = w.palette().color(QPalette::Window);
    QColor highlightColor = w.palette().color(QPalette::Highlight);
    QCOMPARE(windowColor, highlightColor);
}

// ---------------------------------------------------------------------------
// Unit test: setActive(false) removes highlight styling
// ---------------------------------------------------------------------------
void TestTabEntryWidget::testSetActiveFalse()
{
    TabEntryWidget w(1);
    w.show();

    // First activate, then deactivate
    w.setActive(true);
    QVERIFY(w.autoFillBackground() == true);

    w.setActive(false);
    QVERIFY(w.autoFillBackground() == false);
}

// ---------------------------------------------------------------------------
// Unit test: setLoading(true) shows spinner text in favicon label
// ---------------------------------------------------------------------------
void TestTabEntryWidget::testSetLoadingTrue()
{
    TabEntryWidget w(1);
    w.show();

    w.setLoading(true);

    QLabel* fav = faviconLabel(w);
    QVERIFY(fav != nullptr);
    // The spinner is shown as "⟳" text
    QCOMPARE(fav->text(), QStringLiteral("⟳"));
    // The pixmap should be null/empty when spinner is shown
    QVERIFY(fav->pixmap().isNull());
}

// ---------------------------------------------------------------------------
// Unit test: setLoading(false) clears spinner and restores favicon/placeholder
// ---------------------------------------------------------------------------
void TestTabEntryWidget::testSetLoadingFalse()
{
    TabEntryWidget w(1);
    w.show();

    // Start loading
    w.setLoading(true);
    QLabel* fav = faviconLabel(w);
    QVERIFY(fav != nullptr);
    QCOMPARE(fav->text(), QStringLiteral("⟳"));

    // Stop loading
    w.setLoading(false);

    // Spinner text should be cleared
    QVERIFY(fav->text().isEmpty());
    // A placeholder pixmap should be shown (not null)
    QVERIFY(!fav->pixmap().isNull());
}

// ---------------------------------------------------------------------------
// Unit test: setTitle("") with a URL set displays the URL
// ---------------------------------------------------------------------------
void TestTabEntryWidget::testEmptyTitleDisplaysUrl()
{
    TabEntryWidget w(1);
    w.show();
    // Force a known size so elision is deterministic
    w.resize(300, 36);

    const QString url = QStringLiteral("https://example.com/page");
    w.setUrl(url);
    w.setTitle(QString());  // empty title

    QLabel* title = titleLabel(w);
    QVERIFY(title != nullptr);

    // The displayed text should contain the URL (possibly elided)
    // At minimum it should not be empty and should start with "https"
    QVERIFY(!title->text().isEmpty());
    QVERIFY(title->text().startsWith(QStringLiteral("https")) ||
            title->text() == QStringLiteral("…") ||
            url.startsWith(title->text().left(5)));
}

// ---------------------------------------------------------------------------
// Property 13: Title/URL fallback
//
// For any TabEntryWidget whose title is the empty string, the display text
// shown shall equal the tab's URL string (possibly elided).
//
// Validates: Requirements 2.3
// ---------------------------------------------------------------------------
void TestTabEntryWidget::prop13_emptyTitleDisplaysUrl()
{
    QVERIFY(rc::check(
        "Property 13: empty title displays URL in TabEntryWidget",
        []() {
            // Generate a non-empty printable ASCII string (simulating a URL).
            // We restrict to printable ASCII [0x20, 0x7E] to avoid encoding
            // ambiguities when converting between std::string and QString.
            auto asciiChar = rc::gen::inRange<char>(0x20, 0x7F);
            const std::vector<char> chars =
                *rc::gen::nonEmpty(rc::gen::container<std::vector<char>>(asciiChar));
            const QString url = QString::fromLatin1(chars.data(), static_cast<int>(chars.size()));

            TabEntryWidget w(42);
            // Give the widget a fixed size so QFontMetrics has a stable width
            w.resize(300, 36);
            w.show();

            w.setUrl(url);
            w.setTitle(QString());  // empty title — should fall back to URL

            QList<QLabel*> labels = w.findChildren<QLabel*>();
            RC_ASSERT(labels.size() >= 2);
            QLabel* title = labels.at(1);
            RC_ASSERT(title != nullptr);

            // The displayed text must not be empty
            RC_ASSERT(!title->text().isEmpty());

            // The displayed text must be a prefix of the URL (elided) or the full URL.
            // QFontMetrics::elidedText either returns the full string or a truncated
            // version ending with the ellipsis character (U+2026).
            const QString displayed = title->text();
            // Strip trailing ellipsis if present
            const QString displayedBase = displayed.endsWith(QChar(0x2026))
                ? displayed.left(displayed.length() - 1)
                : displayed;

            RC_ASSERT(url.startsWith(displayedBase) || displayed == url);
        }
    ));
}

// ---------------------------------------------------------------------------
// Property 14: Loading state display round-trip
//
// After setLoading(true) → loading indicator visible (spinner text "⟳").
// After setLoading(false) → loading indicator hidden (spinner text empty,
// pixmap non-null).
//
// Validates: Requirements 7.3, 7.4
// ---------------------------------------------------------------------------
void TestTabEntryWidget::prop14_loadingStateDisplayRoundTrip()
{
    QVERIFY(rc::check(
        "Property 14: loading indicator shown during load, replaced by favicon after load",
        []() {
            // Generate a tab ID
            const int tabId = *rc::gen::inRange(1, 1000);

            TabEntryWidget w(tabId);
            w.resize(300, 36);
            w.show();

            QList<QLabel*> labels = w.findChildren<QLabel*>();
            RC_ASSERT(labels.size() >= 1);
            QLabel* fav = labels.at(0);

            // --- Loading ON ---
            w.setLoading(true);
            RC_ASSERT(fav->text() == QStringLiteral("⟳"));
            RC_ASSERT(fav->pixmap().isNull());

            // --- Loading OFF ---
            w.setLoading(false);
            RC_ASSERT(fav->text().isEmpty());
            RC_ASSERT(!fav->pixmap().isNull());
        }
    ));
}

QTEST_MAIN(TestTabEntryWidget)
#include "test_tab_entry_widget.moc"
