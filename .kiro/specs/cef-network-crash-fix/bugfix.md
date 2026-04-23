# Bugfix Requirements Document

## Introduction

The Lilypad CEF+Qt6 desktop browser app suffers from four interrelated startup bugs that together prevent the application from functioning. The network service crashes in a tight restart loop, the target page never loads, a spurious second blank window appears, and a misconfiguration warning fires repeatedly. All four issues stem from incorrect CEF initialization order and missing settings in `main.cpp` and `mainwindow.cpp`.

## Bug Analysis

### Current Behavior (Defect)

1.1 WHEN the application launches THEN the system emits `[ERROR:...network_service_instance_impl.cc:608] Network service crashed or was terminated, restarting service.` repeatedly every ~1 second indefinitely.

1.2 WHEN the application launches THEN the system never renders the page at `https://polli.page`; the CEF container remains blank.

1.3 WHEN the application launches THEN the system opens a second blank native window in addition to the intended `MainWindow`.

1.4 WHEN the application launches THEN the system emits `[WARNING:...resource_util.cc:83] Please customize CefSettings.root_cache_path for your application. Use of the default value may lead to unintended process singleton behavior.` repeatedly.

### Expected Behavior (Correct)

2.1 WHEN the application launches THEN the system SHALL start the CEF network service once and keep it running without crashing or restarting.

2.2 WHEN the application launches THEN the system SHALL successfully load and render `https://polli.page` inside the CEF container embedded in `MainWindow`.

2.3 WHEN the application launches THEN the system SHALL open exactly one window (the `MainWindow`); no additional blank windows SHALL appear.

2.4 WHEN the application launches THEN the system SHALL NOT emit the `root_cache_path` warning; `CefSettings.root_cache_path` SHALL be set to an application-specific directory.

### Unchanged Behavior (Regression Prevention)

3.1 WHEN the user types a URL in the address bar and presses Enter THEN the system SHALL CONTINUE TO navigate the embedded CEF browser to that URL.

3.2 WHEN the application is closed THEN the system SHALL CONTINUE TO shut down CEF cleanly via `CefShutdown()` without crashes or resource leaks.

3.3 WHEN the application is running THEN the system SHALL CONTINUE TO pump the CEF message loop via the Qt timer at the configured interval so that CEF remains responsive.

3.4 WHEN the CEF browser is created THEN the system SHALL CONTINUE TO embed it as a child of the native Qt container widget at the correct geometry.
