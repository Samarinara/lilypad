# Bugfix Requirements Document

## Introduction

The Lilypad browser (Qt6/QML, QtWebEngine/Chromium) has several performance defects that cause excessive memory consumption, unnecessary CPU work, UI freezes, and portability failures. These issues manifest across the tab management system, URL processing pipeline, and application startup. This document captures the defective behaviors, the correct behaviors that should replace them, and the existing behaviors that must be preserved unchanged.

---

## Bug Analysis

### Current Behavior (Defect)

**Issue 1 — All WebEngineViews are always alive (high memory cost)**

1.1 WHEN multiple tabs are open THEN the system instantiates one full `WebEngineView` (and its Chromium renderer process) per tab simultaneously, regardless of whether the tab is active or visible.

1.2 WHEN a background tab is not visible THEN the system keeps its `WebEngineView` fully alive in memory with no suspension or unloading.

**Issue 2 — Redundant `dataChanged` emission in `onFaviconChanged`**

1.3 WHEN a favicon URL is received for a tab THEN the system calls `entry->setUrl(entry->url())` — a no-op that re-sets the same URL — instead of storing the favicon URL, causing a spurious `dataChanged` signal to be emitted without any actual data change.

1.4 WHEN a favicon URL is received THEN the system does not store the favicon URL in the `TabEntry`, so the favicon is never propagated to the model or displayed.

**Issue 3 — `tabById` is O(n) and called on every hot-path event**

1.5 WHEN any tab event fires (`onTitleChanged`, `onUrlChanged`, `onLoadingStateChanged`, `onFaviconChanged`, `indexOfTab`) THEN the system performs a full linear scan of `m_tabs` to locate the tab, with no indexed lookup.

**Issue 4 — `closeAllTabs` blocks the event loop**

1.6 WHEN the application is closing and `closeAllTabs()` is called THEN the system spins a nested `QEventLoop` with a 5-second safety timeout, blocking the UI thread until all tabs emit close signals or the timeout expires.

1.7 WHEN WebEngineViews do not emit their close signals promptly THEN the system freezes the UI for up to 5 seconds before quitting.

**Issue 5 — Bang database path resolution is fragile and non-portable**

1.8 WHEN the application is run on any machine other than the developer's machine THEN the system fails to locate `bangs.db` because `initDatabase()` includes hardcoded absolute paths (`/home/samarinara/...`) that do not exist on other systems.

1.9 WHEN none of the hardcoded paths resolve to an existing file THEN the system silently falls back to no bang support without any user-visible indication.

**Issue 6 — Excessive `qDebug` logging in hot paths**

1.10 WHEN a page is loading THEN the system emits `qDebug` log messages on every `onUrlChanged`, `onTitleChanged`, `onLoadingStateChanged`, and `WebEngineView` signal invocation, adding overhead on every navigation event.

**Issue 7 — `QRegularExpression` compiled on every `process()` call**

1.11 WHEN `UrlProcessor::isUrl()` is called THEN the system constructs a new `QRegularExpression` object for `domainRegex` as a local variable, recompiling the pattern on every invocation.

1.12 WHEN `UrlProcessor::handleBangRouting()` is called THEN the system constructs a new `QRegularExpression` object for `bangRegex` as a local variable, recompiling the pattern on every invocation.

---

### Expected Behavior (Correct)

**Issue 1 — Lazy WebEngineView instantiation**

2.1 WHEN multiple tabs are open THEN the system SHALL only instantiate `WebEngineView` components for tabs that are active or have been recently visited, using a `Loader`-based approach or equivalent lazy instantiation strategy.

2.2 WHEN a background tab is not visible THEN the system SHALL either defer its `WebEngineView` creation until the tab is first activated, or suspend/unload it after it becomes inactive, reducing concurrent Chromium renderer process count.

**Issue 2 — Correct favicon handling**

2.3 WHEN a favicon URL is received for a tab THEN the system SHALL store the favicon URL in the `TabEntry` (e.g., as a `faviconUrl` string property) and emit `dataChanged` only when the stored value actually changes.

2.4 WHEN a favicon URL is stored in `TabEntry` THEN the system SHALL expose it via the model so QML can load and display the favicon image.

**Issue 3 — O(1) tab lookup**

2.5 WHEN any tab event fires THEN the system SHALL locate the corresponding `TabEntry` in O(1) time using a `QHash<int, TabEntry*>` keyed by `tabId`.

2.6 WHEN a tab is created or removed THEN the system SHALL keep the `QHash` index consistent with `m_tabs`.

**Issue 4 — Non-blocking application close**

2.7 WHEN `closeAllTabs()` is called THEN the system SHALL NOT spin a nested `QEventLoop` or block the UI thread.

2.8 WHEN the application is closing THEN the system SHALL release tab resources and exit without waiting for WebEngineView close signals, completing shutdown promptly.

**Issue 5 — Portable bang database resolution**

2.9 WHEN the application initializes the bang database THEN the system SHALL resolve `bangs.db` using only portable paths: the Qt resource system (`qrc:/`) or paths relative to the application binary (`QCoreApplication::applicationDirPath()`).

2.10 WHEN `bangs.db` cannot be found at any portable path THEN the system SHALL log a clear warning and continue without bang support, without relying on any hardcoded developer-machine absolute paths.

**Issue 6 — Gated debug logging**

2.11 WHEN URL changes, title changes, and loading state changes fire during page navigation THEN the system SHALL NOT emit unconditional `qDebug` messages in these hot-path handlers; any retained diagnostic logging SHALL be gated behind a compile-time or runtime debug flag.

**Issue 7 — Static regex compilation**

2.12 WHEN `UrlProcessor::isUrl()` is called THEN the system SHALL use a `static const QRegularExpression` for `domainRegex` so the pattern is compiled only once for the lifetime of the process.

2.13 WHEN `UrlProcessor::handleBangRouting()` is called THEN the system SHALL use a `static const QRegularExpression` for `bangRegex` so the pattern is compiled only once for the lifetime of the process.

---

### Unchanged Behavior (Regression Prevention)

3.1 WHEN a user switches to a tab THEN the system SHALL CONTINUE TO display that tab's web content correctly and update the URL bar with the tab's current URL.

3.2 WHEN a user opens a new tab THEN the system SHALL CONTINUE TO create a tab entry, navigate to the initial URL, and make the new tab active.

3.3 WHEN a user closes a tab THEN the system SHALL CONTINUE TO remove the tab from the list, switch to an adjacent tab if the closed tab was active, and free associated resources.

3.4 WHEN a page finishes loading THEN the system SHALL CONTINUE TO update the tab's title, URL, and loading indicator in the tab panel.

3.5 WHEN a user enters a bang query (e.g., `!g search term`) in the URL bar THEN the system SHALL CONTINUE TO route the query to the correct search engine URL.

3.6 WHEN a user enters a plain URL or domain in the URL bar THEN the system SHALL CONTINUE TO navigate to that URL, prepending `https://` if no scheme is present.

3.7 WHEN a user enters a search query with no bang THEN the system SHALL CONTINUE TO fall back to the default search engine.

3.8 WHEN the tab count reaches the maximum (50) THEN the system SHALL CONTINUE TO prevent new tab creation.

3.9 WHEN dark mode is toggled THEN the system SHALL CONTINUE TO apply the correct theme colors across the tab panel, URL bar, and top bar.

3.10 WHEN the application starts THEN the system SHALL CONTINUE TO open with a single tab navigated to the default home page.
