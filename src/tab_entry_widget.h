#pragma once
#include <QFrame>
#include <QPixmap>

class QLabel;
class QPushButton;
class QMovie;
class QHBoxLayout;

class TabEntryWidget : public QFrame {
    Q_OBJECT
public:
    explicit TabEntryWidget(int tabId, QWidget* parent = nullptr);
    ~TabEntryWidget();

    int tabId() const { return m_tabId; }

    void setActive(bool active);
    void setTitle(const QString& title);
    void setFavicon(const QPixmap& pixmap);
    void setLoading(bool loading);
    void setUrl(const QString& url);  // used as fallback when title is empty

signals:
    void clicked(int tabId);
    void closeClicked(int tabId);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    int          m_tabId;
    QString      m_url;
    QString      m_title;
    bool         m_active  = false;
    bool         m_loading = false;

    QLabel*      m_faviconLabel;
    QLabel*      m_titleLabel;
    QPushButton* m_closeBtn;
    QMovie*      m_loadingSpinner = nullptr;

    // Cached favicon pixmap (set via setFavicon, restored after loading ends)
    QPixmap      m_faviconPixmap;

    void updateTitleDisplay();
    void updateBackground();
};
